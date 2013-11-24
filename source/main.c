char version[] = "ScanSort 1.81";
/* Scansort 1.81 - Main			27.08.1999
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

#define  DEFINITIONS_HERE
#include "scansort.h"

extern mstring * endungen, * prefixes, * have_lists;

// Switches
extern char reportpath[], targetpath[], tradepath[], badpath[], wastepath[];
extern char model_name[], *jpg_endung;
extern int  report_html_table, report_missingcsv, report_missingcsvs,
            report_comparelist, report_freshen, report_newsreader,
 	    report, report_toggle, report_makehavelist, trade, move_files, debug, nologfile,
 	    makehavelist, model_collection, make_csv, csv_ECSV,
 	    ignore_havelists, fsplit, csvpath_used;
extern      mstring *single_collection;

char LOGFILENAME[] = "scansort.log";
char DESCRIPT_ION[] = "descript.ion";

double picsb = 0.0;

int collection_longest = 10;	// Laengster Collection-Name
int collection_maxpics = 0;	// groesste  Collection

int picbufferlen = 0;

FILE * logfile = NULL;
FILE * extralogfile = NULL;
mstring * search_paths;
extern mstring * csv_paths;
char * configfilename = NULL;

int files_copied, files_deleted;

char **environment;

int main(int argc, char **argv, char **env)
{
  int arg = 1, i;
  time_t zeit_t;
  int report_all_flag;

  environment = env;
  srand ((unsigned)time(&zeit_t));
  for (i=100;i--;)	// erstmal anschubsen RAND_MAX
    rand();
  p_init();  
  m = mm_init (0x4000, 0x400);
  init_options();

  zeit = ctime (&zeit_t);

// parse switches, increase arg
  for (arg = 1; arg < argc; arg++)
  {
    if (argv[arg][0] == '-' || argv[arg][0] == alternate_switch_char)
      process_switch(argv[arg]);
    else if (!configfilename && !single_collection->n)
      configfilename = argv[arg];
    else ms_push (search_paths, argv[arg]);
  }

  if (!configfilename && !single_collection->n && !fsplit)
    help();

  if (!nologfile)
    logfile = fopen (LOGFILENAME, WT);
  lprintf (0, "%s Collection Manager\n%s", version, zeit);
  if (debug)
     lprintf (0, "Main offset: %p\n", main);
  {
    char p[MAXPATH];
    p_GetCurrentDirectory(p);
    lprintf (0, "Working directory: %s\n", p);
  }

  for (i=0; i < argc; i++)
    lprintf (0, "%s ", argv[i]);
  lprintf (0, "\n");  

  if (strlen (endung) != endlen)
     error ("Illegal default extension %s : length must be %d", endung, endlen);
  endungen = ms_init (m, 4);   
  ms_push (endungen, endung);

  if (make_csv)
  {
     if (single_collection->n)
       error ("switches -s and -C are not allowed together");
     if (csv_ECSV)
       make_ECSV  (configfilename, search_paths->n ? search_paths->s[0] : configfilename);
     else
       update_csv (configfilename, search_paths->n ? search_paths->s[0] : ".");
     error ("");  		 // exit
  }
  
  if (fsplit)
  {
     do_fsplit(configfilename);	 // split Cyberclub CSV
     error ("");  		 // and exit
  }
    
  readconfigfile (configfilename);

  report_all_flag = report && (!(report_toggle & 1) || report_makehavelist || report_html_table
          || report_missingcsv || report_missingcsvs || report_comparelist
          || report_newsreader || model_collection);
  if (report_all_flag)
     prepare_reports();

  if (have_lists->n && !ignore_havelists)
      for (i = 0; i < have_lists->n; i++)
      {
        lprintf(2, "Reading list of existing pictures from %s ...\n", have_lists->s[i]);
        read_havelist(have_lists->s[i], i);
      }   
  
  files_copied = 0;
  files_deleted = 0;
  if (search_paths->n && !trade)
  {
   for (i = 0; i < search_paths->n; i++)
   { char s[MAXPATH];
     int  col;

     p_GetFullPathName (search_paths->s[i], MAXPATH, s);
     lprintf (2, "Searching for Pictures in %s\n", s);
     if (!move_files || (col = check_source_overlap (s)) == -1 )
        ScanForPics (s);
     else
        lprintf (2,"Source path %s overlaps with target path\n  for collection %s - ignored\n",
                 s, collections[col]->name);
   }
   lprintf (2, "%4d Files %s\n", files_copied, move_files ? "Moved" : "Copied");
   if (move_files)
     lprintf (2, "%4d Files deleted\n", files_deleted);
  }
  else if (!((report && !(report_toggle & 1)) || report_all_flag))
    lprintf (2, "Nothing to do !\n");

  if (makehavelist)
    make_havelist(makehavelist);

  if (report)
  {
   if (report_all_flag)
   {
     generate_reports();
     if (report_makehavelist)
       make_havelist (report_makehavelist);
     if (report_missingcsv)
       make_missingcsv();
     if (report_missingcsvs)
       make_missingcsvs();
     if (report_html_table)
       make_htmltable();
     if (report_comparelist)
       make_comparelist();
     if (report_newsreader)
       make_newsreader_filters();
     if (csvpath_used >= 0)
       move_csvs();
   }
   else if (report_freshen) 
   {
      for (i = 0; i < nr_of_collections; i++)
        if (collections[i]->updated)
        {
          prepare_collection(i);
          report_collection(i);
        }
   }
  } 
// check for completed collections
  for (i = 0; i < nr_of_collections; i++)
  {
     collection * csv = collections[i];
     if (csv->updated && csv->correct == csv->nrpics)
        lprintf (2, "Completed %s with %d pics ! :-)\n", csv->name, csv->nrpics);
  }
  
  if (trade)
    do_all_trading();
  
  if (model_collection)
    do_model_collection();

  print_mem_used (0);
  if (logfile)
    fclose (logfile);
  error("");
  return 0;  // Dummy
}

