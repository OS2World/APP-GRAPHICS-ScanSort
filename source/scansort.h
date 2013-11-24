/* Scansort 1.81 - header file

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

#ifndef SCANSORT_H
#define SCANSORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform dependent (tried to keep this out of here, but - sigh...)
#if defined(_WIN32) || defined(__OS2__)

#else
  #define stricmp strcasecmp
#endif

typedef int (* QST) (const void *, const void *);	// supress stupid qsort warnings 

#define endlen 3	// only 3-char extensions supported

// Memory allocation (memory.c)

void * mymalloc (int l);
void myfree (void *p);
void * myrealloc (void * pold, int l);

typedef struct {
  char * base;		// base adress of buffer 
  char * current;	// pointer to last free element
  int  size;		// total size
  int  free;		// free at the moment
  int  waste;		// how much may be wasted
} minimem_handle;

minimem_handle * mm_init (int size, int waste);
void * mm_alloc (minimem_handle * m, int size);
char * mm_strdup (minimem_handle * m, char * s);
void * mm_memdup (minimem_handle * m, void * s, int len);
char * mm_strndup (minimem_handle * m, char * s, int len);
void mm_free (minimem_handle * m);
extern minimem_handle * m;
void print_mem_used (int lev);

typedef struct {
  char ** s;		// field of pointers to the strings
  int  n;		// current number
  int  max;		// current maxsize
  minimem_handle * m;	// memory source
  char * recycle;	// pointer for recycling of last index
  int  rl;		// bytes for recycling
} mstring;

mstring * ms_init (minimem_handle * m, int size);
char * ms_push  (mstring * ms, char * s);
char * ms_npush (mstring * ms, void * s, int n);

char * bf_init (minimem_handle * m, int size);
void bf_write (char * b, int nr, int value);
void bf_set (char * b, int nr);
void bf_clear (char * b, int nr);
int bf_test (char * b, int nr);

// String handling (string.c)

int normalchar (unsigned int c, int make_underscore);
int m_stricmp (unsigned char * s1, unsigned char * s2);
void m_stricmp_init(void);
char * m_strncpy (char * dest, char *src, int len);
char * is_head (unsigned char * s1, unsigned char * s2);
char * is_tail (unsigned char * s1, unsigned char * s2);
void make_search_string (unsigned char * s);
char * checkendung3 (char *s);
char * bn (char *name);
char * name_from_path (char * path);
void str2upper (char * s);
void str2lower (char * s);
void name2upper (char * s);
void name2lower (char * s, int ext_only);
void clean_name (char *s);
void make_csv_name_string (unsigned char * dest, unsigned char * src);

// Platform dependent (platform_xx.c)

#define MAXPATH 250		  // Windows limit for path length
typedef time_t   p_FILETIME;  	  // store filetime everywhere as 32 Bit value
extern  char PC, WINPC;		  // Path seperators
extern char *RT, *WT, *WB, *AT;	  // file modes for fopen (Win: "rt", Unix "r")


typedef struct {		// Interface for directory expansion
  char * name;
  char fullname[MAXPATH];
  p_FILETIME time;
  int size;
  char isdir;
  } fileinfo;
typedef void * dirhandle;

void   p_init (void);
char * system_error (void);
void * p_alloc (int size);
int    p_alloc_size (void * p);
void   p_alloc_free (void * p);
int    p_MoveFile (char * source, char * dest);
int    p_CopyFile (char * source, char * dest);
void   p_GetFullPathName (char * filename, int bufferlen, char * buffer);
void   p_GetCurrentDirectory (char * p);
int    p_DeleteFile (char * filename);
void   p_MakeFileHidden (char * name);
int    p_CreateDirectory (char * p);
int    p_File_is_open (char * fname);
int    p_CompareFileTime (p_FILETIME t1, p_FILETIME t2);
int    p_write_picbuffer (char * name, int size, p_FILETIME time);
int    readfile (char * name, char ** buffer, int * bufferlen);
int    exists_length (char * name, int size);
extern p_FILETIME exists_length_last_filetime;
dirhandle p_opendir (char *dirname, fileinfo * fi, int recurse, int allow_dirs, int add_wild);
int       p_nextdir (dirhandle dirh, fileinfo * fi);
char      * timestr( p_FILETIME pf );
void   p_Exit (int err);
void   p_adjust_ecsvpath (char *s);
extern char PC, WINPC;	// path character
extern char alternate_switch_char;

// Zip stuff
#define ZIP_MULTI 3
void WaitZip (int nr);
void RunZip(char * zipname, int Ecsv);
int FlushZip (void);
void WriteZip (char *name);
void InitZip (void);

// csv.c

int linetokens (char * line,		// line to scan
                char ** newline,	// pointer to next line will be returned here
                int maxtokens,		// how many tokens max.
                char ** tokens,		// pointer to tokens
                int  * tokenlen,	// lengths of tokens
                int  nurkomma,		// 0: Komma or WS, 1: only Komma
                int  last_is_comment);   // 1: last entry is comment (can include Kommas)

void readconfigfile (char *name);
int parse_csv (int nr, char * buffer);
void make_index(int v);
unsigned int hash_size (unsigned int size, unsigned int tablelen);
unsigned int hash_name (char * name, unsigned int tablelen);
int countlines (char * buf);
#define iscr(c) (c==10 || c==13)
void move_csvs (void);

// havelist.c
int read_havelist (char * name, int nr);
void make_havelist(int mode);

// file.c
void backup (char * pfad);
void delete_bad (char * fname);
int DeleteWaste(char * name);
void GetUnusedFilename (char * name);
int checktargetpath (char * p, int create, int colnr);
void check_make_directory (char * dir);
int copy_pic (char * source, char * target);
int readbuffer(char * name);
int expand_wildcard (mstring * ms, char * wild, int return_fileinfos);
void lprintf (int, char *, ...);
void * error   (char *, ...);
char * get_home_directory (char * s);
FILE * my_fopen (char * s, char * mode);

// scan.c
int kb(double bytes);
int mb(double bytes);
void checkpic (char * fname, char * name, p_FILETIME time, int size);
void checkpic_report (char * fname, char * name, p_FILETIME time, int size, int rep_collection);
int  checkendung (char *s);
void ScanForPics (char * path);
int check_source_overlap (char * source);

// ext_funcs.c
int FuzzyMatching(char* RefStr, char* MatchStr);
unsigned int crcblock( unsigned char* p, unsigned count);

// options.c
void init_options (void);
void process_switch (char * s);
void check_arguments(void);
void help (void);
void help_fsplit (void);
void get_dir_switch (char * dir, char * para);

// report.c
void make_htmltable (void);
void make_missingcsv (void);
void make_missingcsvs (void);
void make_comparelist (void);
void make_newsreader_filters (void);
void generate_reports(void);
void report_collection (int col);
void prepare_collection (int col);
void prepare_reports(void);

// csv_model.c
void do_model_collection(void);
void update_csv (char * name, char * dirname);
void make_ECSV (char * name, char * dirname);

// trade.c
void prepare_all_trading (mstring * ms);
void do_all_trading (void);

// main.c
extern char version[];
extern char LOGFILENAME[], DESCRIPT_ION[];

enum picexist {no, yes, wrongsize, wrongcrc, unknown};

typedef struct {	// Description of a pic
  int size;		// size
  unsigned int crc;	// CRC	(you guessed that :-) )
  char *name;		// name with extension
  char *description;    // comment or E-CSV-Path
  unsigned short collection;  // number of the collection (65535 should be enough :-) )
  char exists;		// state of existance (have, miss, bad)
  char exists_trade;    // for trading (needed, in collection)
} pic_desc;

typedef struct {	// Description of a collection
  char * name;		// name
  char * signame;	// name for identifikation (without special chars)
  char * filename;	// filename
  char * fullfilename;	// filename with complete path
  char * targetdir;	// collection directory
  char * reportfile;	// reportfile 
  int nr;		// number of collection
  int targetexists;	// collection directory exists ?
  int nrpics;		// number of pics
  double nrbytes;	// number of bytes
  int longest;		// length of longest filename (for formatting)
  int empty;		// no file exists ?
  int correct;		// number of good,
  int incorrect;	// bad,
  int missing;		// missing,
  int extra;		// unknown pics in the collection
  double correctb;      // bytes of good,
  double incorrectb;	// bad,
  double missingb;	// missing,
  double extrab;	// unknown pics in the collection
  double onhdb;		// bytes of good pics on your harddisk
  int updated;		// pics were sorted in in the current run
  int filesize;		// filesize of the CSV-File
  int Ecsv;		// 1: E-CSV (comment is subpath)
  int descriptions;	// 1: CSV has comments
  int specialtypes;	// 1: CSV has other filetypes than jpg
  p_FILETIME filetime;	// date/time of CSV file
  pic_desc * pic_descs; // array with the picture descriptions
  int * samepaths;	// array with indices of collections with the same path
  mstring * extras;	// names of extra pics in collection dir
  unsigned char * haveindex;	// Bitfield showing which havefiles carry pics of the collection
} collection;


// trade.c
char * locate_pic (pic_desc * p);
pic_desc ** findpic_size_h (int picsize);
pic_desc ** findpic_name_h (char * picname);


// fsplit.c
void do_fsplit (char * base_csv);
int compare_text_line (char ** s1, char ** s2);

// globals
#ifdef DEFINITIONS_HERE
#define ME
#else
#define ME extern
#endif

ME unsigned char * picbuffer;
ME int nr_of_collections;
ME int pics;			// nr of pics in all collections
ME double picsb;		// nr of bytes in all pics
ME collection ** collections;
ME char * zeit;			// string with date/time
ME int BigEndian;		// 1 for BigEndian machines (like Sun Sparc)

extern char * endung;

#endif

