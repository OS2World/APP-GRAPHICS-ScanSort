/* Scansort 1.81 - platform specific stuff for Unix
   Copyright (C) 1999 Stuart Redman
   sturedman@hotmail.com
   Latest version available at http://www.geocities.com/SouthBeach/Pier/3193/

This file is part of Scansort.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* The Unix port was done partly by Kees de Bruin, rest by me (Stu Redman). 

   This is not tested very well. I'm always open for suggestions
   from all you Unix Gurus out there ! ;-)  */


#include "scansort.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <errno.h>
#include <utime.h>
#include <dirent.h>

char PC = '/';
char WINPC = '\\';		/* still needed for E-CSVs */
char * RT = "r";		// file modes for fopen 
char * WT = "w";
char * WB = "w";
char * AT = "a";
char alternate_switch_char = '-';
extern char **environment;

/* Alignment (0: 8 Bit, 1: 16 Bit, 3: 32 Bit) This is important for non-Intel CPUs */

int align = ALIGN;

static int Umask;

void p_init(void)
{
  int u;
  char cb[4];
  volatile int *ip = (int *) cb;

  u = umask (0);	// read umask
  umask (u);		// and set it back to original value
  Umask = (u & 0777) ^ 0666;
  // printf ("umask is %o\n",Umask); 

  // Detect Endian
  *ip = 0;
  cb[0] = 1;
  BigEndian = *ip != 1;
  m_stricmp_init();
}

void * 	p_alloc (int size)
{
  return malloc (size);
}
  
int    	p_alloc_size (void * p)
{
  /* Is this possible? - Probably not. So we have to work around it.
   */
  return 0;
}

void   	p_alloc_free (void * p)
{
  if (p)
     free (p);
}

// adjust target path for a E-CSV
void   p_adjust_ecsvpath (char *s)
{
  while (*s)
  {
    if (*s == WINPC)
       *s = PC;
    s++;   
  }
}

/* create full pathname to a filename which can be relative to current directory
 */
void	p_GetFullPathName (char * filename, int bufferlen, char * buffer)
{
  int	l;

  if (filename[0] == PC)
  {
    /* absolute path
     */
    strcpy(buffer, filename);
  }
  else
  {
    /* relative path
     */
    getcwd(buffer, bufferlen);
    l = strlen(buffer);
    if (buffer[l - 1] != PC)
    {
      buffer[l] = PC;
      buffer[l + 1] = 0;
    }
    strcat(buffer, filename);
  }
}

/* get current Directory
 */
void	p_GetCurrentDirectory (char * p)
{
  getcwd(p, MAXPATH);
}

/* delete File, return 0 on error
 */
int	p_DeleteFile (char * name)
{
  int	rv;

//  chmod(name, 0664);    for deletion file modes are irrelevant
  rv = remove(name);
  return (rv == 0);
}

/* set file attributes to hidden (not available in Unix)
 */
void	p_MakeFileHidden (char * name)
{
  return;
}

/* create a directory, return 0 on error
 */
int	p_CreateDirectory (char * p)
{
  int	rv;

  rv = mkdir(p, 0775);				/* u+rwx, g+rwx, o+rx */
  return (rv == 0);
}

/* check if file is open - Can this be done ?
 */
int	p_File_is_open (char * fname)
{
  return 0;
}

/* File time handling
 *
 * Scansort uses a 32 Bit file time. This can be the DOS date/time, the Unix system time or whatever, but has
 * to be 32 Bit
 */

/* compare filetimes:
 * ret: -1  t1 older than t2
 *       0  same
 *       1  t1 newer than t2
 */
int p_CompareFileTime (p_FILETIME t1, p_FILETIME t2)
{
  if (t1 < t2)
    return -1;
  else if (t1 > t2)
    return 1;
  else   
    return 0;
}

/* Format a date string.
 */
char * timestr( p_FILETIME pf )
{
  static char buf[50];
  struct tm *tm;

  tm = localtime (&pf);
  strftime (buf, 48, "%d.%m.%Y %H:%M", tm);

  return buf;
}

/* Read file into buffer. Return size if o.k., 0 fehler. When bufferlen == NULL allocate exact (+10) size,
 * otherwise increase by +64k if too small
 */
p_FILETIME last_read_time;
 
int readfile (char * name, char ** buffer, int * bufferlen)
{
  int handle;
  int size;
  int gelesen;
  struct stat s_buf;

  handle = open(name, O_RDONLY);
  if (handle == -1)
  {
    lprintf (2, "Can't open file %s for reading: %s\n", name, system_error());
    return 0;
  }
  if (fstat(handle, &s_buf) == -1)
  {
    lprintf (2, "Can't get filesize of %s: %s", name, system_error());
    return 0;
  }
  size = s_buf.st_size;
  last_read_time = s_buf.st_mtime;
  if (bufferlen == NULL)
  {
    *buffer = mymalloc(size + 10);
  }
  else if (*bufferlen < size + 10)
  {
    if (*bufferlen > 0)
      p_alloc_free (*buffer);
    while (*bufferlen < size + 10)
      *bufferlen += 0x10000;
    *buffer = mymalloc(*bufferlen);  
  }
  gelesen = read(handle, *buffer, size);
  close(handle);
  if (gelesen != size)
  {
    lprintf (2, "Error reading %s : only %d bytes instead of %d : %s\n",
	     name, gelesen, size, system_error());
    return 0;
  }
  (*buffer)[size] = (*buffer)[size+1] = (*buffer)[size+2] = '\n';
  (*buffer)[size+3] = 0;
  return size;
}

/* Write pic from buffer to disk
 */
static int _p_write_picbuffer (char * name, int size, p_FILETIME time, char * buffer, int touch)
{
  int handle;
  int wsize;

  DeleteWaste (name);
  handle = open (name, O_WRONLY | O_CREAT | O_TRUNC, Umask);
  if (handle == -1)
  {
    lprintf (2, "Can't open file %s for writing: %s\n", name, system_error());
    return 0;
  }
  if ((wsize = write (handle, buffer, size)) != size)
  {
    lprintf (2, "Write error to %s : %d bytes instead of %d : %s\n",
	     name, wsize, size, system_error());
    return 0;
  }
  if (close (handle) == -1)
  {
    lprintf (2, "Can't close file %s : %s\n", name, system_error());
    return 0;
  }
  if (!touch)
  {
    struct utimbuf times;
    times.actime = times.modtime = time;
    utime (name, &times);  // restore original file date/time
  }
  return 1;
}	

int p_write_picbuffer (char * name, int size, p_FILETIME time)
{
  extern int touch_pics;
  return _p_write_picbuffer (name, size, time, picbuffer, touch_pics);
}

// copy a file to a new path, return 0 on error

int	p_CopyFile (char * source, char * dest)
{
  static char * cbuf;		// use an own buffer to prevent nasty side effects
  static int  cblen = 0;
  int len;

  len = readfile (source, &cbuf, &cblen);
  if (!len)
     return 0;
  return _p_write_picbuffer (dest, len, last_read_time, cbuf, 0);     
}

// move a file to a new path, return 0 on error

int	p_MoveFile (char * source, char * dest)
{
  int i;

  if (rename(source, dest))
  {           // error, try copy/delete
    if (!(i = p_CopyFile(source, dest)))
    {
       lprintf (2, "Can't copy %s tp %s : %s\n", source, dest, system_error());
       return 0;
    }
    else
       return p_DeleteFile(source);
  }
  else
    return 1;
}


/* Check if file exists, return 1 if exist (wildcards not allowed). Save filetime of last found (yeah,
 * that's a dirty hack, I know). When size == 0 ignore size.
 *
 */
p_FILETIME exists_length_last_filetime;

int exists_length (char * name, int size)
{
  struct stat sbuf;

  if (!stat(name, &sbuf) && (size == 0 || size == (int) sbuf.st_size))
  {
    exists_length_last_filetime = sbuf.st_mtime; /* might use st_atime instead!! */
    return 1;
  }
  else
    return 0;
}

// return last error message from operating system

char * system_error (void)
{
  static char lpMsgBuf[200];

  strcpy (lpMsgBuf, strerror(errno));

  return lpMsgBuf;
} 


// for benchmarking (currently not used)

void stopit(void)
{
  static clock_t T = 0, t;

  if (!T)
     T = clock();
  else
  {
     t = clock();
     lprintf (2, "STOP: %d\n", t-T);
     T = t;
  }
}

// expand a wildcard
// * We use the glob function here. I don't know if this is very portable, but it is POSIX.

int expand_wildcard (mstring * ms, char * wild, int return_fileinfos)
{
  fileinfo fi, *fi1;
  int i, found = 0;
  glob_t pglob;
  struct stat sinfo;
  char buf[MAXPATH];

  if (wild[0] == '~')
     sprintf (buf, "%s%s", get_home_directory(wild), wild+1);
  else
     strcpy (buf, wild);
  glob (buf, 0, NULL, &pglob);

  for (i = 0; i < pglob.gl_pathc; i++)
  {
      p_GetFullPathName (pglob.gl_pathv[i], MAXPATH, fi.fullname);
      if (return_fileinfos)
      {
        if (stat (fi.fullname, &sinfo))
           continue;	// error
        if ((sinfo.st_mode & S_IFMT) == S_IFDIR)
           fi.isdir = 1;
        else if ((sinfo.st_mode & S_IFMT) == S_IFREG)
           fi.isdir = 0;
        else continue;	// no ordinary file
        fi.time = sinfo.st_mtime;
        fi.size = sinfo.st_size;
        fi1 = (fileinfo *) ms_npush (ms, &fi, sizeof fi);
        fi1->name = name_from_path (fi1->fullname);
        found++;
      }
      else
      {
        ms_push (ms, fi.fullname);
        found++;
      }  
  }
  globfree(&pglob);

  return found;
}  

// Directory handling 

typedef struct {
  DIR * dir_handle;
  mstring * paths;
  int current_path;
  int recurse;		// 1 = process subdirectories
  int allow_dirs;	// 1 = return directories as well (only if not recurse)
//  int add_wild;		// 1 = arguments are directories, so wildcard is necessary (required for recurse)
                        // this is only used in the Windows version  
} dirstruct;


int p_nextdir (dirhandle dirh, fileinfo * fi)
{
  dirstruct * d = (dirstruct *) dirh;
  struct dirent * info = NULL;
  int found = 0, finished = 0, isdir = 0;
  char fname[MAXPATH];
  char *s;
  struct stat sinfo;

  do
  {
    if (d->dir_handle && (info = readdir(d->dir_handle)))
      found = 1;
    else
    {
      if (d->dir_handle != NULL)
	closedir (d->dir_handle);
      d->current_path++;
      if (d->paths->n == d->current_path)
      {
        finished = 1;
        mm_free (d->paths->m);
      }  
      else
      {
        strcpy (fname, d->paths->s[d->current_path]);
        if ((d->dir_handle = opendir(fname)) != NULL 
	     && (info = readdir(d->dir_handle)))
           found = 1;
      }
    }
    if (found)
    {
      s = info->d_name;
      sprintf (fname, "%s%c%s", d->paths->s[d->current_path], PC, s);
      if (stat (fname, &sinfo))
         found = 0;	// error
      else
      {   
       isdir = 0;
       if ((sinfo.st_mode & S_IFMT) == S_IFDIR)
       {
        if (d->allow_dirs)
          isdir = 1;
        else  
        {
          found = 0;
          if (d->recurse && s[0] != '.')
  	     ms_push (d->paths, fname);
        }
       }
       else if ((sinfo.st_mode & S_IFMT) != S_IFREG)
         found = 0;	// no ordinary file
      }  
    }
    if (found)
    {
        strcpy (fi->fullname, fname);
        fi->name = name_from_path (fi->fullname);
        fi->time = sinfo.st_mtime;
        fi->size = sinfo.st_size;
        fi->isdir = isdir;
    }
  } while (!found && !finished);
  return found;
}

dirhandle p_opendir (char *dirname, fileinfo * fi, int recurse, int allow_dirs, int add_wild)
{
  minimem_handle * m;
  dirstruct * d;
  int l = strlen (dirname) - 1;

  m = mm_init  (2048, 256);
  d = mm_alloc (m, sizeof (*d));
  d->paths = ms_init (m, 1000);
  d->current_path = -1;
  d->recurse = recurse;
  d->allow_dirs = allow_dirs;
  d->dir_handle = NULL;
  
  if (dirname[l] == PC)
    dirname[l] = 0;
  ms_push (d->paths, dirname);

  return p_nextdir (d, fi) ? d : NULL;
}  

void p_Exit (int err)
{
//  _flushall(); // flush all streams/open files (probably done by Unix automatically)
  printf ("\n"); // for the help functions
  exit (err);   
}


/* And now the code for running Zip for trading.
   The Windows version uses multitasking and pipe interprocess communication.
   This here is the simple version (which WON'T work for Windows because
   of the crippled commandline interface...) */

void InitZip (void)
{
}

void WaitZip (int nr)
{
}

static mstring * zms;

// prepare zip for running

void RunZip(char * zipname, int Ecsv) 
{ 
   extern int verbose;
   minimem_handle * m1;

   m1 = mm_init (0x4000, 0x400);
   zms = ms_init (m1, 100);

   ms_push (zms, "zip");
   if (!Ecsv)
      ms_push (zms, "-j");
   if (!verbose)
      ms_push (zms, "-q");
   ms_push (zms, zipname);
      
   p_DeleteFile (zipname);
 }

// send a filename to zip

void WriteZip (char *name)
{
   name [strlen(name) - 1] = 0;		// remove trailing cr
   ms_push (zms, name);
}

// run Zip

int FlushZip (void)
{
  int i, l = 0;
  char * cmd;
  
  for (i = 0; i < zms->n; i++)
    l += strlen (zms->s[i]) + 1;
  cmd = mm_alloc (zms->m, l + 5);
  *cmd = 0;
  for (i = 0; i < zms->n; i++)
  {
    strcat (cmd, zms->s[i]);
    strcat (cmd, " ");
  }
  lprintf (0, "Command: %s\n", cmd);
  i = system (cmd);
  mm_free (zms->m);
  return i;
}


#if 0
 obsolete stuff

void RunZip(char * zipname, int Ecsv) 
{ 
   char cmd[100];
   extern int verbose;
   minimem_handle * m1;
   char * shell = getenv ("SHELL");

   if (!shell)
      shell = "/bin/sh";

   lprintf (2, "Shell: %s\n", shell);

   m1  = mm_init (0x4000, 0x400);
   zms = ms_init (m1, 100);

   ms_push (zms, shell);
//   ms_push (zms, "-c zip");
   ms_push (zms, "-c");
   ms_push (zms, "args");
   if (!Ecsv)
      ms_push (zms, "-j");
   if (!verbose)
      ms_push (zms, "-q");
   ms_push (zms, zipname);
      
   p_DeleteFile (zipname);
 }

int FlushZip (void)
{
  int i;

  ms_push (zms, "");
  zms->s[zms->n - 1] = NULL;

  for (i = 0; i < zms->n - 1; i++)
    lprintf (2, ">>%s<<\n", zms->s[i]);
  
  p_exec (zms->s);
  mm_free (zms->m);
}

/* execute a command
 */
int	p_exec(char * argv[])
{
  int	pid;
  int	status;

  pid = fork();
printf ("P_EXEC: Pid %d\n", pid);  
  if (pid == -1)
  {
    return -1;
  }
  if (pid == 0)
  {
//    execve(argv[0], argv, environment);
    execv(argv[0], argv);
    error ("Exec Error: %s", system_error()); // shouldn't return
  }
  do
  {
    if (waitpid(pid, &status, 0) == -1)
    {
      if (errno != EINTR)
      {
	return -1;
      }
    }
    else
    {
      return status;
    }
  } while (1);
}

int stricmp (char * s1, char *s2)
{
  return strcasecmp (s1,s2);
}

#endif
