/* Scansort 1.81 - platform specific stuff for OS/2
   Copyright (C) 2001 Dmitry A.Steklenev
   glassman_ru@geocities.com
   glass@cholod.ru
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

#define  INCL_DOS
#define  INCL_DOSFILEMGR
#define  INCL_ERRORS
#include <os2.h>

#define  HF_STDIN  0

char  PC    = '\\';
char  WINPC = '\\'; // still needed for E-CSVs
char* RT    = "r";  // file modes for fopen
char* WT    = "w";
char* WB    = "wb";
char* AT    = "a";
char  alternate_switch_char = '/';

APIRET rc;          // last return code

typedef struct _FDATETIME
{
  FDATE date;
  FTIME time;

} FDATETIME;

// Alignment (0: 8 Bit, 1: 16 Bit, 3: 32 Bit)
// This is important for non-Intel CPUs

int align = 0;

void p_init(void)
{
  BigEndian = 0;
  m_stricmp_init();
}

void* p_alloc( int size )
{
  void * pb;
  rc = DosAllocMem( &pb, size, PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_COMMIT );

  if( rc != NO_ERROR )
    return 0;
  else
    return pb;
}

int p_alloc_size( void * p )
{
  ULONG  size  = -1,
         flags =  0;

  rc = DosQueryMem( p, &size, &flags );
  return size;
}

void p_alloc_free( void * p )
{
  rc = DosFreeMem(p);
}

// adjust target path for a E-CSV (dummy for OS/2)
void p_adjust_ecsvpath( char *s )
{}

// move a file to a new path, return 0 on error
int p_MoveFile( char* source, char* dest )
{
  rc = DosMove( source, dest );
  return rc == NO_ERROR;
}

// copy a file to a new path, return 0 on error
int p_CopyFile( char* source, char* dest )
{
  rc = DosCopy( source, dest, 0 );
  return rc == NO_ERROR;
}

// create full pathname to a filename which can be relative to current directory
void p_GetFullPathName( char * filename, int bufferlen, char* buffer )
{
  rc = DosQueryPathInfo( filename, FIL_QUERYFULLNAME, buffer, bufferlen );
}

// Get current Directory
void p_GetCurrentDirectory( char* p )
{
  rc = DosQueryCurrentDir( 0, p, MAXPATH );
}

// Delete File, return 0 on error
int p_DeleteFile( char* name )
{
  FILESTATUS3 status;

  DosQueryPathInfo( name, FIL_STANDARD, &status, sizeof(status));
  status.attrFile = FILE_NORMAL;
  DosSetPathInfo  ( name, FIL_STANDARD, &status, sizeof(status), 0 );
  rc = DosDelete(name);

  return rc == NO_ERROR;
}

// Set file attributes to hidden (no equivalent for this under Unix)
void p_MakeFileHidden( char * name )
{
  FILESTATUS3 status;
  DosQueryPathInfo( name, FIL_STANDARD, &status, sizeof(status));
  status.attrFile |= FILE_HIDDEN;
  DosSetPathInfo  ( name, FIL_STANDARD, &status, sizeof(status), 0 );
}

// create a directory, return 0 on error
int p_CreateDirectory( char* p )
{
  int   len  = strlen(p);
  char* name = strdup(p);

  // Remove trailed slash
  if( len && name[len-1] == PC )
    name[len-1] = 0;

  rc = DosCreateDir( name, NULL );
  free( name );

  return rc == NO_ERROR;
}

// Check if file is open
int p_File_is_open( char * fname )
{
  return 0;
}

/* File time handling
   Scansort uses a 32 Bit file time. This can be the DOS date/time, the Unix
   system time or whatever, but has to be 32 Bit */

/* compare filetimes:
ret: -1  t1 older than t2
      0  same
      1  t1 newer than t2 */
int p_CompareFileTime( p_FILETIME t1, p_FILETIME t2 )
{
  if( t1 < t2 )
     return -1;
  else if (t1 > t2)
     return 1;
  else
     return 0;
}

// Format a date string.
char* timestr( p_FILETIME pf )
{
  static char buf[50];

  FDATETIME tm = *((FDATETIME*)&pf);
  sprintf( buf, "%d.%d.%d %d:%d", tm.date.day,   tm.date.month, tm.date.year+1980,
                                  tm.time.hours, tm.time.minutes );
  return buf;
}

// Read file into Buffer
// return size if o.k., 0 Fehler
// bufferlen = NULL: allocate exact (+10) allokieren, otherwise increase by +64k if too small
int readfile( char* name, char** buffer, int* bufferlen )
{
  HFILE       handle;
  ULONG       action;
  FILESTATUS3 status;
  int         gelesen;

  rc = DosOpen( name, &handle, &action, 0,
                FILE_NORMAL | FILE_ARCHIVED,
                OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE, NULL );

  if( rc != NO_ERROR )
  {
    lprintf( 2, "Can't open file '%s' for reading.\n%s\n", name, system_error());
    return 0;
  }

  rc = DosQueryFileInfo( handle, FIL_STANDARD, &status, sizeof(status));

  if( rc != NO_ERROR )
  {
    lprintf( 2, "Can't get filesize of '%s'\n%s", name, system_error());
    return 0;
  }

  if( bufferlen == NULL)
     *buffer = mymalloc( status.cbFile + 10 );
  else
     if(*bufferlen < status.cbFile + 10)
     {
       if(*bufferlen > 0)
         p_alloc_free(*buffer);

       while(*bufferlen < status.cbFile + 10)
         *bufferlen += 0x10000;

       *buffer = mymalloc(*bufferlen);
     }

  rc = DosRead( handle, *buffer, status.cbFile, &gelesen );

  if( rc != NO_ERROR )
  {
    lprintf( 2, "Error reading '%s'\n%s\n", name, system_error());
    DosClose(handle);
    return 0;
  }

  rc = DosClose(handle);

  (*buffer)[status.cbFile  ] = (*buffer)[status.cbFile+1] = (*buffer)[status.cbFile+2] = '\n';
  (*buffer)[status.cbFile+3] = 0;
  return gelesen;
}

// write pic from buffer to disk
int p_write_picbuffer( char* name, int size, p_FILETIME time )
{
  HFILE       handle;
  ULONG       action;
  ULONG       wsize;
  FILESTATUS3 status;

  extern int touch_pics;

  p_DeleteFile( name );
  rc = DosOpen( name, &handle, &action, 0, FILE_NORMAL,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE, NULL );

  if( rc != NO_ERROR )
  {
    lprintf( 2, "Can't open file '%s' for reading\n%s\n", name, system_error());
    return 0;
  }

  rc = DosWrite( handle, picbuffer, size, &wsize );
  if( rc != NO_ERROR )
  {
    lprintf( 2, "Write error '%s'\n%s\n", name, system_error());
    DosClose(handle);
    return 0;
  }

  if( !touch_pics )
  {
    DosQueryFileInfo( handle, FIL_STANDARD, &status, sizeof(status));
    status.fdateLastWrite = ((FDATETIME*)&time)->date;
    status.ftimeLastWrite = ((FDATETIME*)&time)->time;
    DosSetFileInfo  ( handle, FIL_STANDARD, &status, sizeof(status));
  }

  rc = DosClose( handle );
  if( rc != NO_ERROR )
  {
    lprintf( 2, "Can't close file '%s'\n%s\n", name, system_error());
    return 0;
  }

  return 1;
}

// check if file exists, return 1 if exist (wildcards in name allowed here, but never used)
// save filetime of last found (yeah, that's a dirty hack, I know)
// size == 0: ignore size

p_FILETIME exists_length_last_filetime;

int exists_length( char* name, int size )
{
  FILESTATUS3 status;
  rc = DosQueryPathInfo( name, FIL_STANDARD, &status, sizeof(status));

  if( rc == NO_ERROR && (size == 0 || size == (int)status.cbFile ))
  {
    ((FDATETIME*)&exists_length_last_filetime)->date = status.fdateLastWrite;
    ((FDATETIME*)&exists_length_last_filetime)->time = status.ftimeLastWrite;
    return 1;
  }
  else
    return 0;
}

// return last error message from operating system
char* system_error( void )
{
  char   szErrInfo[2048];
  ULONG  ulMessageLength = 0;
  APIRET get_rc;

  get_rc = DosGetMessage( 0, 0, (PCHAR)szErrInfo, sizeof(szErrInfo),
                          rc, (PSZ)"OSO001.MSG", &ulMessageLength );

  szErrInfo[ulMessageLength] = 0;

  if( szErrInfo[ulMessageLength-1] == '\n' )
      szErrInfo[ulMessageLength-1] = 0;

  return get_rc ? "No error text is available" : szErrInfo;
}

// Directory handling

typedef struct {

  HDIR     dir_handle;
  mstring* paths;
  int      current_path;
  int      recurse;      // 1 = process subdirectories
  int      allow_dirs;   // 1 = return directories as well (only if not recurse)
  int      add_wild;     // 1 = arguments are directories, so wildcard is
                         // necessary (required for recurse)
} dirstruct;

dirhandle p_opendir (char* dirname, fileinfo* fi, int recurse, int allow_dirs, int add_wild )
{
  minimem_handle* m;
  dirstruct*      d;
  int l = strlen( dirname ) - 1;

  m = mm_init ( 2048, 256    );
  d = mm_alloc( m, sizeof(*d));

  d->paths = ms_init( m, 1000 );
  d->current_path = -1;
  d->recurse = recurse;
  d->allow_dirs = allow_dirs;
  d->add_wild = add_wild;
  d->dir_handle = HDIR_CREATE;

  if( dirname[l] == PC )
      dirname[l] = 0;

  ms_push( d->paths, dirname );
  return p_nextdir( d, fi ) ? d : NULL;
}

int p_nextdir( dirhandle dirh, fileinfo* fi )
{
  dirstruct*d = (dirstruct *)dirh;
  FILEFINDBUF3 info;

  char  fname[MAXPATH];
  int   found = 1, finished = 0, isdir;
  char* s = info.achName;

  do
  {
    found = 1;

    if( d->dir_handle == HDIR_CREATE ||
        DosFindNext( d->dir_handle, &info, sizeof(info), &found ) != NO_ERROR )
    {
      if( d->dir_handle != HDIR_CREATE )
      {
        DosFindClose(d->dir_handle);
        d->dir_handle = HDIR_CREATE;
      }

      d->current_path++;

      if( d->paths->n == d->current_path )
      {
        finished = 1;
        found = 0;
        mm_free( d->paths->m );
      }
      else
      {
        char *s1 = d->paths->s[d->current_path];
        if( d->add_wild )
            sprintf( fname, "%s%c*.*", s1, PC);
        else
            strcpy ( fname, s1 );

        found = 1;
        DosFindFirst( fname, &d->dir_handle, FILE_NORMAL | FILE_DIRECTORY, &info, sizeof(info), &found, FIL_STANDARD );
      }
    }

    if( found )
    {
      if( d->add_wild )
        sprintf( fname, "%s%c%s", d->paths->s[d->current_path], PC, s );
      else
      {
        strcpy( fname, d->paths->s[d->current_path]);
        strcpy( name_from_path(fname), s );
      }

      isdir = 0;

      if( info.attrFile & FILE_DIRECTORY )
      {
        if( d->allow_dirs )
          isdir = 1;
        else
        {
          found = 0;
          if( d->recurse && strcmp( s , "." ) && strcmp( s , ".." ))
            ms_push( d->paths, fname );
        }
      }
    }

    if( found )
    {
      strcpy( fi->fullname, fname );
      fi->name  = name_from_path( fi->fullname );
      fi->size  = info.cbFile;
      fi->isdir = isdir;

      ((FDATETIME*)&fi->time)->date = info.fdateLastWrite;
      ((FDATETIME*)&fi->time)->time = info.ftimeLastWrite;
    }
  } while( !found && !finished );

  return found;
}

// expand a wildcard
int expand_wildcard (mstring * ms, char * wild, int return_fileinfos)
{
  fileinfo fi, *fi1;
  dirhandle* dh;
  int  found = 0;
  char buf[MAXPATH];

  if( wild[0] == '~' )
     sprintf( buf, "%s%s", get_home_directory(wild), wild+1 );
  else
     strcpy (buf, wild);

  if(( dh = p_opendir( buf, &fi, 0, 1, 0)) != 0 )
    do {
      if( return_fileinfos )
      {
        fi1 = (fileinfo *) ms_npush( ms, &fi, sizeof fi );
        fi1->name = name_from_path ( fi1->fullname );
      }
      else
        ms_push (ms, fi.fullname);

      found++;

    } while ( p_nextdir(dh, &fi));

  return found;
}

void p_Exit( int err )
{
  DosExit( EXIT_PROCESS, err );
}

/* And now the great code for running Zip for trading.
   Zip is started in parallel, and a pipe connected to it so it can
   read the files to zip via stdin.
   I'm quite sure this is MUCH easier in Unix...
   However I suggest you leave this out first.    */

typedef struct {

   int busy;
   PID pid;

} proc_info;

proc_info proc_infos[ZIP_MULTI];
int ziperrors = 0;

void InitZip( void )
{
  int i;
  for( i = ZIP_MULTI; i--; )
       proc_infos[i].busy = proc_infos[i].pid = 0;
}

void WaitZip( int nr )
{
  proc_info*  pi = proc_infos + nr;
  RESULTCODES resCodes;
  PID pid = pi->pid;

  if( pi->busy )
  {
      rc = DosWaitChild( DCWA_PROCESS, DCWW_WAIT, &resCodes, &pid, pid );

      if( resCodes.codeResult )
      {
         lprintf( 2, "Error: Zip returned status %d\n", resCodes.codeResult );
         ziperrors++;
      }

      pi->busy = 0;
   }
}

HFILE hChildStdinWrDup;

void RunZip( char* zipname, int Ecsv )
{
  static int akt_zip = 0;
  proc_info* pi = proc_infos + akt_zip;
  char       cmd[100];

  extern int verbose;

  HFILE  hfSaveIn , hfCurrIn ,
         wrPipeIn , rdPipeIn ;

  CHAR   szFailName[CCHMAXPATH];
  APIRET rc;

  RESULTCODES resCodes;

  hfSaveIn = -1;
  hfCurrIn =  HF_STDIN;

  DosDupHandle ( HF_STDIN , &hfSaveIn  );
  DosCreatePipe( &rdPipeIn, &wrPipeIn , 255 );
  DosSetFHState( wrPipeIn , OPEN_FLAGS_NOINHERIT );
  DosDupHandle ( rdPipeIn , &hfCurrIn  );

  WaitZip( akt_zip );
  akt_zip = (akt_zip + 1) % ZIP_MULTI;

  sprintf( cmd, "zip.exe %s-@ %s%s", Ecsv ? "" : "-j ", verbose ? "" : "-q ", zipname );
  lprintf( 1, "Making Zipfile: %s\n", cmd );
  cmd[sizeof("zip.exe")-1] = 0;
  p_DeleteFile( zipname );

  rc = DosExecPgm( szFailName, sizeof(szFailName ), EXEC_ASYNCRESULT,
                   cmd, (PSZ)NULL, &resCodes, "zip.exe" );

  if( rc != NO_ERROR )
  {
    lprintf( 2, "Can't start zip.exe\n%s\n", system_error());
    ziperrors++;
  }
  else
  {
    pi->pid  = resCodes.codeTerminate;
    pi->busy = 1;
  }

  hChildStdinWrDup = wrPipeIn;

  DosClose    ( rdPipeIn  );
  DosDupHandle( hfSaveIn , &hfCurrIn  );
  DosClose    ( hfSaveIn  );
}

// close Handle to Zip so it starts zipping, return 0 on error
int FlushZip( void )
{
  rc = DosClose( hChildStdinWrDup );
  return rc == NO_ERROR;
}

// send a filename to zip
void WriteZip( char* name )
{
  int written;
  DosWrite( hChildStdinWrDup, name, strlen(name), &written );
}

