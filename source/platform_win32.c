/* Scansort 1.81 - platform specific stuff for Windows
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

#include "scansort.h"

#define  WIN32_EXTRA_LEAN
#define  WIN32_LEAN_AND_MEAN
#include <windows.h>


char PC = '\\';
char WINPC = '\\';  // still needed for E-CSVs
char * RT = "rt";		// file modes for fopen 
char * WT = "wt";
char * WB = "wb";
char * AT = "at";
char alternate_switch_char = '/';

// Alignment (0: 8 Bit, 1: 16 Bit, 3: 32 Bit)
// This is important for non-Intel CPUs

int align = 0;

void p_init(void)
{
  BigEndian = 0;
  m_stricmp_init();
}

void * p_alloc (int size)
{
  return GlobalAlloc(GMEM_FIXED, size);
}
  
int    p_alloc_size (void * p)
{
  return GlobalSize(p);
}

void   p_alloc_free (void * p)
{
  GlobalFree(p);
}


// adjust target path for a E-CSV (dummy for Windows)
void   p_adjust_ecsvpath (char *s)
{
}

// move a file to a new path, return 0 on error
int    p_MoveFile (char * source, char * dest)
{
  return MoveFile (source, dest);
}

// BOOL WINAPI     CopyFile(LPCSTR szSrc, LPCSTR szDst, BOOL fFailIfExists);
// copy a file to a new path, return 0 on error
int    p_CopyFile (char * source, char * dest)
{
  return CopyFile (source, dest, 0);
}

// create full pathname to a filename which can be relative to current directory
void p_GetFullPathName (char * filename, int bufferlen, char * buffer)
{
  GetFullPathName (filename, bufferlen, buffer, NULL);
}

// Get current Directory
void p_GetCurrentDirectory (char * p)
{
  GetCurrentDirectory (MAXPATH, p);
}

// Delete File, return 0 on error
int p_DeleteFile (char * name)
{
  SetFileAttributes (name, FILE_ATTRIBUTE_NORMAL);
  return DeleteFile (name);
}

// Set file attributes to hidden (no equivalent for this under Unix)
void p_MakeFileHidden (char * name)
{
  SetFileAttributes(name, FILE_ATTRIBUTE_HIDDEN);
}


// create a directory, return 0 on error

int p_CreateDirectory (char * p)
{
  return CreateDirectory (p, NULL);
}

// Check if file is open

int p_File_is_open (char * fname)
{
  HANDLE handle;
  int inuse = 0;

  handle = CreateFile (fname, GENERIC_READ, 0,
  		       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE) // used by another process
     inuse = 1;
  else
     CloseHandle (handle);
  return inuse;
}


/* File time handling
   Scansort uses a 32 Bit file time. This can be the DOS date/time, the Unix
   system time or whatever, but has to be 32 Bit */

/* compare filetimes:
ret: -1  t1 older than t2
      0  same
      1  t1 newer than t2 */
int p_CompareFileTime (p_FILETIME t1, p_FILETIME t2)
{
  if (t1 < t2)
     return -1;
  else if (t1 > t2)
     return 1;
  else   
     return 0;
}

/* Convert System Filetime to ScanSort Filetime. Since the datatype for System Filetime
   is only defined in this module this function is static */

static p_FILETIME p_FileTime_Systo32 (FILETIME * ts)
{
  p_FILETIME pf;

  FileTimeToDosDateTime (ts, ((short *) & pf) + 1, (short *) & pf);
  return pf;
}

// ... and back. This returns always the same pointer, so be careful !

static FILETIME * p_FileTime_32toSys (p_FILETIME pf)
{
  static FILETIME ft;

  DosDateTimeToFileTime( (short)(pf >> 16), (short)pf, & ft);
  return &ft;
}


/*
typedef struct _SYSTEMTIME {  // st 
    WORD wYear; 
    WORD wMonth; 
    WORD wDayOfWeek; 
    WORD wDay; 
    WORD wHour; 
    WORD wMinute; 
    WORD wSecond; 
    WORD wMilliseconds; 
} SYSTEMTIME;
*/

// Format a date string. This is really yuck-yuck in Win32... :-(
// (there's no way to convert Windows time to Unix time... )
char * timestr( p_FILETIME pf )
{
  static char buf[50];
  FILETIME *t, lt;
  SYSTEMTIME st;

  t = p_FileTime_32toSys (pf);
  FileTimeToLocalFileTime (t, &lt);
  FileTimeToSystemTime(&lt, &st);  
  sprintf (buf, "%02d.%02d.%d %02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
  return buf;
}




// Read file into Buffer
// return size if o.k.,   0 Fehler
// bufferlen = NULL: allocate exact (+10) allokieren, otherwise increase by +64k if too small

int readfile (char * name, char ** buffer, int * bufferlen)
{
  HANDLE handle;
  int size;
  int gelesen;
  
  handle = CreateFile (name, GENERIC_READ, FILE_SHARE_READ,
  		       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE)
  {
     lprintf (2, "Can't open file %s for reading: %s\n", name, system_error());
     return 0;
  }
  size = GetFileSize (handle, NULL);
  if (size == -1)
  {
     lprintf (2, "Can't get filesize of %s: %s", name, system_error());
     return 0;
  }   
  if (bufferlen == NULL)
     *buffer = mymalloc(size + 10);
  else
     if (*bufferlen < size + 10)
     {
       if (*bufferlen > 0)
         p_alloc_free (*buffer);
       while (*bufferlen < size + 10)
         *bufferlen += 0x10000;
       *buffer = mymalloc(*bufferlen);  
     }
  ReadFile (handle, *buffer, size, &gelesen, NULL);
  CloseHandle (handle);
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

// write pic from buffer to disk

int p_write_picbuffer (char * name, int size, p_FILETIME time)
{
  HANDLE handle;
  int wsize;
  extern int touch_pics;
  FILETIME *ft;

        DeleteWaste (name);
        handle = CreateFile (name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  	if (handle == INVALID_HANDLE_VALUE)
  	{
      	  lprintf (2, "Can't open file %s for writing: %s\n", name, system_error());
     	  return 0;
  	}
  	if (WriteFile(handle, picbuffer, size, &wsize, NULL) == FALSE || wsize != size)
  	{
  	  lprintf (2, "Write error to %s : %d bytes instead of %d : %s\n",
  	      name, wsize, size, system_error());
  	  return 0;
  	}
  	if (!touch_pics)
  	{
  	  ft = p_FileTime_32toSys (time);
  	  SetFileTime (handle, ft, ft, ft);  // restore original file date/time
  	}  
  	if (CloseHandle (handle) == FALSE)
  	{
      	  lprintf (2, "Can't close file %s : %s\n", name, system_error());
     	  return 0;
  	}
	return 1;
}	

// check if file exists, return 1 if exist (wildcards in name allowed here, but never used)
// save filetime of last found (yeah, that's a dirty hack, I know)
// size == 0: ignore size

p_FILETIME exists_length_last_filetime;

int exists_length (char * name, int size)
{
  HANDLE dirhandle;
  WIN32_FIND_DATA info;
  int count = 0;

  if ((dirhandle = FindFirstFile(name, &info)) == INVALID_HANDLE_VALUE) 
    return 0;
  FindClose (dirhandle);
  if (size == 0 || size == (int) info.nFileSizeLow)
  {
    exists_length_last_filetime = p_FileTime_Systo32(&info.ftLastWriteTime);
    return 1;
  }
  else
    return 0;
}

// return last error message from operating system

char * system_error (void)
{
  static char * lpMsgBuf = NULL;

  if (lpMsgBuf)
    LocalFree( lpMsgBuf );

  FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
  );
// Display the string.
//MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

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

int expand_wildcard (mstring * ms, char * wild, int return_fileinfos)
{
  fileinfo fi, *fi1;
  dirhandle * dh;
  int found = 0;
  char buf[MAXPATH];

  if (wild[0] == '~')
     sprintf (buf, "%s%s", get_home_directory(wild), wild+1);
  else
     strcpy (buf, wild);
  if (dh = p_opendir (buf, &fi, 0, 1, 0)) 	// = is correct
    do {
      if (return_fileinfos)
      {
        fi1 = (fileinfo *) ms_npush (ms, &fi, sizeof fi);
        fi1->name = name_from_path (fi1->fullname);
      }
      else
        ms_push (ms, fi.fullname);
      found++;
    } while (p_nextdir (dh, &fi));
  return found;
}  

// Directory handling 

typedef struct {
  HANDLE  dir_handle;
  mstring * paths;
  int current_path;
  int recurse;		// 1 = process subdirectories
  int allow_dirs;	// 1 = return directories as well (only if not recurse)
  int add_wild;		// 1 = arguments are directories, so wildcard is necessary (required for recurse)
} dirstruct;


int p_nextdir (dirhandle dirh, fileinfo * fi)
{
  dirstruct * d = (dirstruct *) dirh;
  WIN32_FIND_DATA info;
  int found = 0, finished = 0, isdir;
  char fname[MAXPATH];
  char *s = info.cFileName;

  do
  {
    if (d->dir_handle != INVALID_HANDLE_VALUE && FindNextFile(d->dir_handle, &info))
      found = 1;
    else
    {
      if (d->dir_handle != INVALID_HANDLE_VALUE)
	FindClose (d->dir_handle);
      d->current_path++;
      if (d->paths->n == d->current_path)
      {
        finished = 1;
        mm_free (d->paths->m);
      }  
      else
      {
        char *s1 = d->paths->s[d->current_path];
        if (d->add_wild)
           sprintf (fname, "%s%c*.*", s1, PC);
        else
           strcpy (fname, s1);
        if ((d->dir_handle = FindFirstFile(fname, &info)) != INVALID_HANDLE_VALUE)
           found = 1;
      }
    }
    if (found)
    {
      if (d->add_wild)
         sprintf (fname, "%s%c%s", d->paths->s[d->current_path], PC, s);
      else
      {
         strcpy (fname, d->paths->s[d->current_path]);
         strcpy (name_from_path (fname), s);
      }   
      isdir = 0;
      if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        if (d->allow_dirs)
          isdir = 1;
        else  
        {
          found = 0;
          if (d->recurse && s[0] != '.')
  	     ms_push (d->paths, fname);
        }
    }
    if (found)
    {
        strcpy (fi->fullname, fname);
        fi->name = name_from_path (fi->fullname);
        fi->time = p_FileTime_Systo32 (&info.ftLastWriteTime);
        fi->size = info.nFileSizeLow;
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

  m = mm_init (2048, 256);
  d = mm_alloc (m, sizeof (*d));
  d->paths = ms_init (m, 1000);
  d->current_path = -1;
  d->recurse = recurse;
  d->allow_dirs = allow_dirs;
  d->add_wild = add_wild;
  d->dir_handle = INVALID_HANDLE_VALUE;
  
  if (dirname[l] == PC)
    dirname[l] = 0;
  ms_push (d->paths, dirname);

  return p_nextdir (d, fi) ? d : NULL;
}  

void p_Exit (int err)
{
  _flushall();		// flush all streams/open files
  ExitProcess (err);   
}


/* And now the great code for running Zip for trading.
   Zip is started in parallel, and a pipe connected to it so it can
   read the files to zip via stdin.
   I'm quite sure this is MUCH easier in Unix...
   However I suggest you leave this out first.    */

typedef struct {
   int busy;
   PROCESS_INFORMATION piProcInfo; 
   STARTUPINFO siStartInfo; 
} proc_info;

proc_info proc_infos[ZIP_MULTI];
int ziperrors = 0;

void InitZip (void)
{
  int i;
  for (i = ZIP_MULTI; i--; )
          proc_infos[i].busy = 0;
}

void WaitZip (int nr)
{
   proc_info * pi = proc_infos + nr;
   int procstatus;

   if (pi->busy)
   {
      while (GetExitCodeProcess(pi->piProcInfo.hProcess, &procstatus)
	    && procstatus == STILL_ACTIVE)
      	 Sleep(50);
      if (procstatus)
      {
      	 lprintf (2, "Error: Zip returned status %d\n", procstatus);
      	 ziperrors++;
      }   
      pi->busy = 0;
   }
}

HANDLE hChildStdinWrDup; 

void RunZip(char * zipname, int Ecsv) 
{ 
   static int akt_zip = 0;
   proc_info * pi = proc_infos + akt_zip; 
   char cmd[100];
   HANDLE hChildStdinRd, hChildStdinWr, hSaveStdin; 
   SECURITY_ATTRIBUTES saAttr;
   extern int verbose;

// Set the bInheritHandle flag so pipe handles are inherited. 
 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

		// Save the handle to the current STDIN. 

   		hSaveStdin = GetStdHandle(STD_INPUT_HANDLE); 
 
		// Create a pipe for the child's STDIN. 
 
   		if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 10000)) 
      		  error("Stdin pipe creation failed: %s", system_error()); 
 
		// Set a read handle to the pipe to be STDIN. 
 
   		if (! SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd)) 
      		  error("Redirecting Stdin failed: %s", system_error()); 
 
		// Duplicate the write handle to the pipe so it is not inherited. 
 
   		if (! DuplicateHandle(GetCurrentProcess(), hChildStdinWr, 
      		  		GetCurrentProcess(), &hChildStdinWrDup, 0, 
      				FALSE,                  // not inherited 
      				DUPLICATE_SAME_ACCESS)) 
      		  error("DuplicateHandle failed: %s", system_error()); 
 
   		CloseHandle(hChildStdinWr);

		// Now create the child process. 


   WaitZip (akt_zip);
   pi->busy = 1;
   akt_zip = (akt_zip + 1) % ZIP_MULTI;

   sprintf (cmd, "zip %s-@ %s%s", Ecsv ? "" : "-j ", verbose ? "" : "-q ", zipname); 
   lprintf (1, "Making Zipfile: %s\n", cmd);
   DeleteFile (zipname);

// Set up members of STARTUPINFO structure. 
 
   pi->siStartInfo.cb = sizeof(STARTUPINFO); 
   pi->siStartInfo.lpReserved = NULL; 
   pi->siStartInfo.lpReserved2 = NULL; 
   pi->siStartInfo.cbReserved2 = 0; 
   pi->siStartInfo.lpDesktop = NULL;  
   pi->siStartInfo.dwFlags = 0; 
 
// Create the child process. 
 
   if (! CreateProcess(NULL, 
      cmd,           // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &pi->siStartInfo,  // STARTUPINFO pointer 
      &pi->piProcInfo))  // receives PROCESS_INFORMATION 
    error ("Could not run %s: %s", cmd, system_error());

		// After process creation, restore the saved STDIN and STDOUT. 
 
   		if (! SetStdHandle(STD_INPUT_HANDLE, hSaveStdin)) 
      		  error("Re-redirecting Stdin failed: %s", system_error()); 
 
}

// close Handle to Zip so it starts zipping, return 0 on error

int FlushZip (void)
{
  return CloseHandle(hChildStdinWrDup);
}

// send a filename to zip

void WriteZip (char *name)
{
  int written;
  WriteFile(hChildStdinWrDup, name, strlen (name), &written, NULL);
}

