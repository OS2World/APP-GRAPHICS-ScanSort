/* Scansort 1.81 - some file functions
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
#include <stdarg.h>

extern char wastepath[];
extern int wastepathlength;
extern int files_deleted, picbufferlen;
extern FILE * logfile;
extern FILE * extralogfile;
extern char * endung;

// switches
extern int verbose, debug, to_upper, delete_to_waste;


// create bak-file
void backup (char * pfad)
{
    char bak[MAXPATH];

    if (exists_length (pfad, 0))
    {	
       strcpy (bak, pfad);
       strcpy (bak + strlen(bak) - 3, "bak");
       p_CopyFile (pfad, bak); 
    }
}

void delete_bad (char * fname)
{
    lprintf (1, "Deleting bad file %s\n", fname);

    if (!DeleteWaste (fname))
      lprintf (2,"Error deleting %s: %s\n", fname, system_error());
    else
      files_deleted++;
}

void GetUnusedFilename (char * name)
{
  int i = 0;
  char ending[5], *c, *c1 = NULL;

  while (exists_length(name, 0))
  {
      if (!c1)
      {
        c = strrchr (name, '.');
        if (c)
        {
          m_strncpy (ending, c+1, 3);
          *c = 0;
          c1 = c;
        }
        else
        {
          c1 = name + strlen(name);
          ending[0] = 0;
        }  
      }
      sprintf (c1, "_%d.%s", ++i, ending);
  }
}

int DeleteWaste (char * name)
{
  static int firstcall = 1;

  if (!exists_length(name, 0))
     return 1;

  if (delete_to_waste)
  {
    if (firstcall)
    {
       wastepathlength = strlen(wastepath);
       check_make_directory (wastepath);
       firstcall = 0;
    }
    strcpy (wastepath+wastepathlength, name_from_path(name));
    GetUnusedFilename (wastepath);
    
    if (!p_CopyFile(name, wastepath)) 
       lprintf (2, "Warning: can't copy %s to wastebasket\n", name);
  }
  return p_DeleteFile(name);
}  


// check and eventually create target path (recursive)
// create == 1: create
// colnr == -1: no collection

int checktargetpath (char * p, int create, int colnr)  
{
  char p1[MAXPATH], *c;
  if (*p && (colnr < 0 || collections[colnr]->targetexists == 0))
  {
    if (p[strlen(p) - 1] == PC)
       sprintf (p1,"%s.",p);
    else
       sprintf (p1,"%s%c.",p,PC);
    if (!exists_length(p1, 0) && p1[strlen(p1) - 3] != ':')   // strange:  D:\. cannot be found !
    {
      if (create == 0) 
        return 0;
      p1[strlen(p1) - 2] = 0;
      if ((c = strrchr (p1, PC)) && c > p1 && c[-1] != ':')
      {
        *c = 0;
        checktargetpath (p1, 1, -1);
      }
      lprintf(2, "Making Directory %s\n", p);
      if (! p_CreateDirectory (p))
        error ("Cannot make directory %s : %s", p, system_error() );
    }
    if (colnr >= 0)
       collections[colnr]->targetexists = 1;  
  }
  return 1;
}

// check / create directory

void check_make_directory (char * dir)
{
  checktargetpath (dir, 1, -1);
}

int copy_pic (char * source, char * target)
{
  lprintf (1, "Copy %s to %s\n", source, target);
  return p_CopyFile(source, target);
}

int readbuffer(char * name)
{
  return readfile (name, (char**) &picbuffer, &picbufferlen);
}  

/*  level :
   0: logfile only
   1: screen if verbose
   2: always
   3: only screen if verbose
   4: only extra logfile
   5: screen only
*/
void lprintf ( int level, char *format, ...)
{
  va_list apo;
  char pp_buf[1000];

  va_start (apo, format);
  vsprintf (pp_buf, format, apo);
  va_end   (apo);

  if (logfile && level < 3)
  {
    fprintf(logfile, "%s", pp_buf);
    if (debug) fflush (logfile);
  }
  
  if (extralogfile)
  {
    fprintf(extralogfile, "%s", pp_buf);
  }
  
  if (level == 2 || level == 5 || (verbose && (level == 1 || level == 3)))
  {
    printf("%s", pp_buf);
    if (debug) fflush(stdout);
  }  
}
 
void * error ( char *format, ...)
{
  va_list apo;
  char pp_buf[1000];
  int  err = 0;

  va_start (apo, format);
  vsprintf (pp_buf, format, apo);
  va_end   (apo);

  if (logfile)
  {
    if (pp_buf[0])
      fprintf(logfile, "ERROR: %s\n", pp_buf);
    fclose (logfile);
  }  
  if (pp_buf[0])
  {
    printf("ERROR: %s\n", pp_buf);
    err = 1;
  }
  p_Exit (err);
  return NULL;     // dummy, just to allow p = mymalloc (x) || error ("");
}

char * get_home_directory (char * s)
{
  char * home;

  if (!s)
     s = "";
  if (!(home = getenv ("HOME")))
     error ("Can't resolve directory name %s - HOME not set ?", s);
  return home;
}

// Save fopen

FILE * my_fopen (char * s, char * mode)
{
  FILE * f = fopen(s, mode);
  if (!f)
    error ("Can't open %s for %s: %s", s,
          *mode == 'r' ? "reading" : "writing", system_error());
  return f;    
}

