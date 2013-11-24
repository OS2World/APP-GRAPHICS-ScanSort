/* Scansort 1.81 - command line arguments and help functions 
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

extern FILE * logfile;
extern mstring * search_paths;

// Switches

char * jpg_endung = "jpg";	
char * endung;			// extension
int to_upper = 1;		// convert DOS files to uppercase
int to_lower = 0;		// convert files to lowercase
int move_files = 0;		// delete copied (or existing) source files
int verbose = 0;		// verbose output (alnost obsolete)
int check_all_files = 0;	// check all files, no matter what extension)
int nologfile = 0;		// don't write logfile
int makehavelist = 0;		// make list of existing files
int report = 0;			// create reports
int report_mtcm = 0;		// report by alphabet
int report_have = 0;		// report existing
int report_miss = 0;		// report missing
int report_incorrect = 0;	// report bad
int report_extra = 0;		// report extra files
int report_summary = 0;		// add a summary to each report
int report_long_summary = 0;	// verbose summary on console
int report_brief = 0;		// report without descriptions
int report_crc = 0;		// CRC-Check all files again for report
int report_freshen = 0;		// only write reports for collections that got new pics in this run
int report_description = 0;	// create ACDSee descript.ion
int report_makehavelist = 0;   	// create a Havelist from the reports
int report_makehavelist_s = 0;  //   in CSV-format without spaces
int makehavelist_binary = 0;    //   in binary format
int convert_havelists_to_bin=0; // convert all text havelists to binary
int report_html_table = 0;	// create HTML-Table
int report_output_summary = 0;	// write summary to summary.txt 
int report_noempty = 0;		// suppress empty reports
int report_recurse = 0;		// recurse collection dirs when reporting
int report_name_numbers = 0;	// add numbers of have/all to names of reports
int report_inactive = 0;	// reports inaktive collections as well
int report_toggle = 0;	        // don't report
int report_missingcsv = 0;	// export all missings into a CSV
int report_missingcsvs = 0;	// export all missings into individual CSVs
int report_comparelist = 0;	// internal use only :-)
int report_newsreader = 0;	// make filters for newsreader
char * report_identity = NULL;	// name as comment in report
int trade = 0;			// trading-mode
int trade_ask = 0;		// make list of files to ask for
int trade_offer = 0;		// make list of files to offer
int trade_missing = 0;		// make list of files missing in both collections
int trade_give = 0;		// number of files to be copied
int trade_give_long = 0;	// give_collection.txt instead of give.txt
int trade_zip = 0;		// number of files per / size of zip
int trade_fake = 0;		// don't really copy, only check amount
int trade_order = 0;		// 0:alphabet 3:backwards 1:random 
int trade_whole_collections = 0; // copy whole collection
int kill_duplicate_csvs = 0;	// delet duplicate CSVs
int rename_csvs = 0;		// automatically rename CSVs
int kill_bad_files = 0;		// delete bad files (not into waste)
int ignore_bad_files = 0;	// ignore bad files 
int min_bad_size = 0;		// 1 - 100 (%): minimum size for bad files to be copied
int min_bad_namelen = 0;	// ignore all bad files with name (without ext) shorter than this
int no_underscores = 0;		// don't replace spaces by underscores
int make_csv = 0;		// create CSV-file
int csv_exist_only = 0;		// only add existing files
int csv_all_crc = 0;		// recalculate all CRCs
int csv_update = 0;		// only update entries of existing pics without CRC
int csv_all_exts = 0;		// add files with any extension to CSV
int csv_recurse = 0;		// recurse directory for CSV generation
int csv_ECSV = 0;		// generate an E-CSV
int debug = 0;			// flush outputs for debugging
int model_collection = 0;	// create model-collection 
int model_csv = 0;		//    as CSV
int model_picture = 0;		//    copy pictures
int model_rename = 0;		//      rename pictures
int model_AIS = 0;		//    as AIS file (ACDSee 2.4)
int fuzzy_schwelle = 60;	// 20 - 100%
int dont_recurse = 0;		// don't recurse source dirs
int kill_extra_files = 0;	// delete extra files 
int delete_to_waste = 1;	// delete into Waste-Folder
int touch_pics = 0;		// change file date when sorting in
int ignore_havelists = 0;	// ignore all havelists (-hx)
int auto_index_dir = 0;		// make target dir for Scanmaster_Index Scanmaster\Index
int log_unused_csvs = 0;	// print list of unused CSV names into logfile
int fsplit = 0;			// Split Gully Foyle's Cyberclub CSV
int fsplit_decades = 0;		//   by decades
int fsplit_years = 0;		//   by years
int fsplit_categories = 0;	//   by categories
int fsplit_ecsv = 0;		//   create a E-CSV
int fsplit_original = 0;	//   create original L-Port CSVs
int fsplit_autoname = 1;	// automatically find CSV to split
int csvpath_used = -1;		// number of CSV-Path to move incomplete (or all) CSVs to
int csvpath_complete = -1;	// number of CSV-Path to move complete CSVs to

char reportpath[MAXPATH], targetpath[MAXPATH],
     tradepath[MAXPATH], badpath[MAXPATH], wastepath[MAXPATH], newsfilterpath[MAXPATH];
int  wastepathlength;     
char * zipbasename = "give";
char model_name[MAXPATH], model_new_name[MAXPATH], foyle_csv_name[MAXPATH];

mstring *single_collection;	// single collection instead of configfile
mstring *prefixes,		// prefixes / suffixes to be recognized
        *prefixes_remove;  	// 0 : keep / 1 : remove prefix 
mstring *endungen;
mstring *have_lists;
mstring *trade_sourcepaths;
mstring *csv_paths;

void help_report();
void help_trade();
void help_model();

static void add_prefix (char * s, int remove)
{
  int i;
  char rem = remove;

  if (*s == 0)
     error ("Empty prefix not allowed (probably space inserted after -p)");
  for (i = 0; i < prefixes->n; i++)
    if (!m_stricmp(prefixes->s[i], s))    // already there
    {
       if (remove)
          prefixes_remove->s[i][0] = remove;
       return;		
    }   
  ms_push (prefixes, s);
  ms_npush (prefixes_remove, &rem, 1);
}

void init_options (void)
{
  prefixes 		= ms_init (m, 24);
  prefixes_remove 	= ms_init (m, 24);
  endungen		= ms_init (m, 5);
  have_lists		= ms_init (m, 10);
  trade_sourcepaths	= ms_init (m, 5);
  search_paths 		= ms_init (m, 5);
  csv_paths 		= ms_init (m, 5);
  single_collection	= ms_init (m, 5);

// built-in Prefixes. (both with and without '_', otherwise -Pmtcm_ wouldn't work

  add_prefix ("MTCM_", 0);
  add_prefix ("MTCM", 0);
  add_prefix ("McBluna_", 0);
  add_prefix ("McBluna", 0);
  add_prefix ("_finished", 0);
  add_prefix ("finished", 0);
  add_prefix ("_(finished)", 0);
  add_prefix ("(finished)", 0);
  add_prefix ("_final", 0);
  add_prefix ("final", 0);
  add_prefix ("_(final)", 0);
  add_prefix ("(final)", 0);
  add_prefix ("_ongoing", 0);
  add_prefix ("ongoing", 0);
  add_prefix ("_(ongoing)", 0);
  add_prefix ("(ongoing)", 0);

  reportpath[0] = targetpath[0] = tradepath[0] = badpath[0] = newsfilterpath[0] = 0;
  sprintf (wastepath, "ScanSortWaste%c", PC);
  endung = jpg_endung;
  strcpy (model_name, "Lisa Matthews");		// default :-)
}

void get_dir_switch (char * dir, char * para)
{
  int l;
  char buf[MAXPATH];

  if (!para[3])
      error ("no space is allowed between -dx switch and directory");

  if (para[3] == '~')
    sprintf (buf, "%s%s", get_home_directory(para+3), para+4);
  else
    strcpy (buf, para+3);

  p_GetFullPathName (buf, MAXPATH, dir);
  l = strlen(dir);
  if (dir[l - 1] != PC)
  {
    dir[l] = PC;
    dir[l+1] = 0;
  }
  while (l--)			// convert paths to Unix format (code has no effect under Windows)
    if (dir[l] == WINPC)
        dir[l] = PC;
}     

void process_switch (char * s)
{
  int i,d;
  char buffer[MAXPATH];

    switch (s[1]) {
      case 'm' : move_files = 1;
                 break;
      case 'L' : to_lower = 1;		// -L includes -u (no uppercase short names) as well
                 to_upper = 0;
                 if (s[2] == 'e')       // -Le will convert extensions to lowercase only
                     to_lower = 2;
                 break;
      case 'u' : to_upper = 0;
                 break;
      case 'v' : verbose = 1;
                 break;
      case 'r' : report = 1;
                 i = 2;
                 do
                   switch (s[i]) {
                        case 'C' : // kompatibel to old Colver joke
                        case 'M' : report_mtcm = 1;
                        	   break;
                        case 'h' : report_have = 1;
                        	   break;
                        case 'm' : report_miss = 1;
                        	   break;
                        case 'i' : report_incorrect = 1;
                        	   break;
                        case 'e' : report_extra = 1;
                        	   break;
                        case 's' : report_summary = 1;
                        	   break;
                        case 'S' : report_long_summary = 1;
                        	   break;
                        case 'T' : report_html_table = 1;
                                   if (s[i+1] == 'v')
                                   { 
                                     report_html_table = 2;
                                     i++;
                                   }
                        	   break;
                        	   break;
                        case 'a' : report_have = report_miss = report_incorrect
                                     = report_extra = report_summary = 1;
                        	   break;
                        case 'b' : report_brief = 1;
                        	   break;
                        case 'c' : report_crc = 1;
                        	   break;
                        case 'd' : report_description = 1;
                        	   break;
                        case 'D' : report_description = 2;
                        	   break;
                        case 'E' : report_noempty = 1;
                        	   break;
                        case 'A' : report_inactive = 1;
                        	   break;
                        case 'H' : report_makehavelist = 1;
                                   if (s[i+1] == 'b')
                                   {
                                      makehavelist_binary = 1;
                                      i++;
                                   }   
                                   else while (s[i+1] == 'v' || s[i+1] == 's')
                                   { 
                                     if (s[i+1] == 's')
                                       report_makehavelist_s = 1;
                                     else
                                       report_makehavelist = 2;
                                     i++;
                                   }
                                   break;
                        case 'f' : report_freshen = 1;
                                   break;
                        case 'R' : report_recurse = 1;
                                   break;
                        case 'o' : report_output_summary = 1;
                                   break;
                        case 'n' : report_name_numbers = 1;
                                   break;
                        case 'r' : report_toggle ++;
                                   break;
                        case 'x' : report_missingcsv = 1;
                                   break;
                        case 'X' : report_missingcsvs = 1;
                                   break;
                        case 'L' : report_comparelist = 1;
                                   break;
                        case 'N' : report_newsreader = 1;
                                   break;
                        case 'I' : // is evaluated directly in readconfig 
                                   error ("-rI must stand in a line of its own");
                                   break;
                        default  : help_report();
                        	   break;
                   } while (s[++i]);  	   
                 break;
      case 't' : trade = 1;
                 i = 2;
                 do
                   switch (s[i]) {
                        case 'a' : trade_ask = 1;
                        	   break;
                        case 'A' : trade_ask = 2;
                        	   break;
                        case 'o' : trade_offer = 1;
                        	   break;
                        case 'O' : trade_offer = 2;
                        	   break;
                        case 'm' : trade_missing = 1;
                        	   break;
                        case 'M' : trade_missing = 2;
                        	   break;
                        case 'G' : trade_give_long = 1;
                         	   // NO BREAK !!
                        case 'g' : trade_give = 10000000; // 10000 Megabyte
                                   if (sscanf (s+i+1, "%d", &d) == 1)
                                   {
                                     trade_give = d;
                                     while (s[i+1] >= '0' && s[i+1] <= '9')
                                       i++;
                                   }
                        	   break;
                        case 'z' : trade_zip = 5;
                                   if (sscanf (s+i+1, "%d", &d) == 1)
                                   {
                                     trade_zip = d;
                                     while (s[i+1] >= '0' && s[i+1] <= '9')
                                       i++;
                                   }
                                   break;
                        case 'Z' : zipbasename = mm_strdup (m, s + i + 1);
                                   i = strlen(s) - 1;	// break
                                   break;
                        case 'r' : trade_order = 1;	// random
                                   break;
                        case 'b' : trade_order = 3;	// backwards 
                                   break;
                        case 'f' : trade_fake = 1;	// nur so tun als ob
                                   break;
                        case 'F' : trade_fake = 2;	// nur so tun als ob (sources nicht checken)
                                   break;
                        case 'w' : trade_whole_collections = 1;	// ganze Collections erzeugen
                                   break;                                   
                        default  : help_trade();
                        	   break;
                   } while (s[++i]);  	   
                 break;
      case 'M' : model_collection = 1;
                 i = 2;
                 do
                   switch (s[i]) {
                        case '0' :      
                        case '1' :	
                        case '2' :	// Schwelle: 20-100% 
                        case '3' :
                        case '4' :
                        case '5' :
                        case '6' :
                        case '7' :
                        case '8' :
                        case '9' : if (sscanf (s+i, "%d", &d) == 1)
                                   {
                                     fuzzy_schwelle = d;
                                     while (s[i+1] >= '0' && s[i+1] <= '9')
                                       i++;
                                   }
                                   if (fuzzy_schwelle < 20 || fuzzy_schwelle > 100)
                                      help_model();
                        	   break;
	   
                        case 'c' : model_csv = 1;	// without CRC
                        	   break;
                        case 'C' : model_csv = 2;	// with CRC
                        	   break;
                        case 'p' : model_picture = 1;
                                   report = 1;		// otherwise no Copy !
                        	   break;
                        case 'a' : model_AIS = 1;	// create AIS file
                                   report = 1;		// otherwise no Copy !
                        	   break;
                        case 'm' : d = 0;
                                   do {
                                      model_name[d++] = s[++i];
                                   }  while (s[i]);  
                        	   i--;		// fuer Abbruch
                        	   break;   
                        case 'n' : d = 0;
                                   do {
                                      model_new_name[d++] = s[++i];
                                   }  while (s[i]);  
                        	   i--;		// fuer Abbruch
                        	   model_rename = 1;
                        	   break;   
                        default  : help_model();
                        	   break;
                   } while (s[++i]);  	   
                 break;
      case 'C' : make_csv = 1;
                 i = 2;
                 if (s[i]) do
                   switch (s[i]) {
                        case 'e' : csv_exist_only = 1;
                        	   break;
                        case 'c' : csv_all_crc = 1;
                        	   break;
                        case 'u' : csv_update = 1;
                        	   break;
                        case 'a' : csv_all_exts = 1;
                        	   break;
                        case 'r' : csv_recurse = 1;
                        	   break;
                        case 'E' : csv_ECSV = 1;
                        	   break;
                        default  : help();
                                   break;
                   } while (s[++i]);  	   
                 break;
      case 'a' : check_all_files = 1;
                 break;
      case 'b' : if (sscanf (s+2, "%d", &d) == 1 && d > 0)
                    min_bad_size = d;
                 else
                    kill_bad_files = 1;
                 break;
      case 'B' : ignore_bad_files = 1;
                 break;
      case 'E' : kill_extra_files = 1;
                 break;
      case 'e' : endung = mm_strdup (m, s + 2);
                 break;
      case 's' : ms_push (single_collection, s + 2);
                 break;
      case 'l' : nologfile = 1;
                 break;
      case 'H' : makehavelist = 1;
                 if (s[2] == 'v')
                   makehavelist = 2;
                 else if (s[2] == 'b') 
                   makehavelist_binary = 1;
                 break;
      case 'h' : if (s[2] == 'x' && !s[3])
                   ignore_havelists = 1;
                 else if (s[2] == 'b' && !s[3])
                   convert_havelists_to_bin = 1;  
                 else
                   if (!expand_wildcard (have_lists, s + 2, 0))
                      error ("No havelists found matching %s\n", s);
      		 break;
      case 'd' : switch (s[2]) {
                        case 'c' : get_dir_switch (buffer, s);
                                   ms_push (csv_paths, buffer);
                        	   break;
                        case 'C' : get_dir_switch (buffer, s);
                                   ms_push (csv_paths, buffer);
                                   if (csvpath_used == -1)
                                       csvpath_used = csv_paths->n - 1;
                                   csvpath_complete = csv_paths->n - 1;
                        	   break;
                        case 'r' : get_dir_switch (reportpath, s);
                        	   break;
                        case 's' : get_dir_switch (buffer, s);
                                   ms_push (trade_sourcepaths, buffer);
                        	   break;
                        case 't' : get_dir_switch (tradepath, s);
                        	   break;
                        case 'p' : get_dir_switch (targetpath, s);
                        	   break;
                        case 'b' : get_dir_switch (badpath, s);
                        	   break;
                        case 'w' : get_dir_switch (wastepath, s);
                        	   break;
                        case 'N' : get_dir_switch (newsfilterpath, s);
                        	   break;
                        default  : help();
                        	   break;
                 };  	   
                 break;
      case 'K' : kill_duplicate_csvs = 1;
                 if (s[2] == 'r')
                   rename_csvs = 1;
                 break;
      case '_' : no_underscores = 1;
                 break;
      case 'D' : debug = 1;
                 break;
      case 'R' : dont_recurse = 1;
                 break;
      case 'T' : touch_pics = 1;
                 break;
      case 'w' : delete_to_waste = 0;
                 break;
      case 'P' : add_prefix(s+2, 1);
                 break;
      case 'p' : add_prefix(s+2, 0);
                 break;
      case 'x' : i = 2;
                 do
                   switch (s[i]) {
                        case 'i' : auto_index_dir = 1;
                        	   break;
                        case 'u' : log_unused_csvs = 1;
                        	   break;
                        case 'b' : while (s[i+1] >= '0' && s[i+1] <= '9')
                                   {
                                     i++;
                                     min_bad_namelen = min_bad_namelen * 10 + s[i] - '0';
                                   }
                                   break;
                        default  : ;
                        	   break;
                   } while (s[++i]);  	   
                 break;
      case 'F' : fsplit = 1;
                 i = 2;
                 do
                   switch (s[i]) {
                        case 'd' : fsplit_decades = 1;
                        	   break;
                        case 'y' : fsplit_years = 1;
                        	   break;
                        case 'c' : fsplit_categories = 1;
                        	   break;
                        case 'e' : fsplit_ecsv = 1;
                        	   break;
                        case 'o' : fsplit_original = 1;
                        	   break;
                        case 'n' : d = 0;
                                   do {
                                      foyle_csv_name[d++] = s[++i];
                                   }  while (s[i]);  
                        	   i--;		// fuer Abbruch
                        	   fsplit_autoname = 0;
                        case 0   : i--;		// allow -F alone
                        	   break;
                        default  : help_fsplit();
                        	   break;
                   } while (s[++i]);  	   
                 break;
      default  : help();
      		 break;
    }
}

void check_arguments(void)
{
int i;

  if (nologfile && logfile)
  {
    fclose (logfile);
    p_DeleteFile (LOGFILENAME);
    logfile = NULL;
  }  
  if (makehavelist && (move_files || report) )
    error ("Cannot report or move when making list of files you have");
    
  if (report_mtcm && (report_have || report_miss || report_incorrect
                         || report_extra || report_summary))
    error ("Cannot use ScanSort-Style report options on Mastertech reports");

  if (trade)
     prepare_all_trading (search_paths);  
         
  lprintf (0, "Prefixes / Suffixes for CSV files:");
  for (i = 0; i < prefixes->n; i++)
    lprintf (0, "  %s", prefixes->s[i]);
  lprintf (0, "\n");
  lprintf (0, "Prefixes / Suffixes for CSV files which will be removed:");
  for (i = 0; i < prefixes->n; i++)
    if (prefixes_remove->s[i][0])
       lprintf (0, "  %s", prefixes->s[i]);
  lprintf (0, "\n");
}

void help (void)
{
  printf (
  "\n%s - sort pictures from CSV-Files into paths\n"
  "Homepage: http://www.geocities.com/SouthBeach/Pier/3193/ \n"
  "scansort [switches] configfile [sourcepaths]\n"
  "Switches:\n"
  "  -m     'm'ove files (erase source files)\n"
  "  -v     more messages ('v'erbose)\n"
  "  -a     check 'a'll files (regardless of extension)\n"
  "  -l     don't write 'l'ogfile\n"
  "  -H[vb] write list of files you 'h'ave to \"have.txt\" (no copy/move)\n"
  "  -hNAME import list of files you 'h'ave (multiple possible)\n"
  "  -sNAME process 's'ingle collection NAME (no config file)\n"
  "  -exyz  set file 'e'xtension to xyz (instead of jpg)\n"
  "  -b /-B always delete 'b'ad files / ignore them completely\n"
  "  -E     remove 'e'xtra files from collection (after trying to identify them)\n"
  "  -K[r]  'k'ill duplicate CSV-Files (-Kr: rename al CSVs to correct names)\n"
  "  -L[e]  'L'owercase file names (-Le: lowercase only file extensions)\n"
  "  -dcDIR set directory for CSV-files    to DIR (instead of path of config file)\n"
  "  -drDIR set directory for report files to DIR (instead of current path)\n"
  "  -dpDIR set target directory for pics  to DIR (instead of current path)\n"
  "  -dbDIR set target directory for bad pics to DIR (instead of \"BadPictures\")\n"
  "  -dwDIR set target directory for wastebasket,  -w don't delete to wastebasket\n"
  "  -r /-t help on 'r'eports / 't'rading |  a: all files  u: update only CRCs)\n"
  "  -M /-F help on 'M'odel Collections and CSV creation / splitting Foyle-CSVs"
  , version);
  error ("");
}

void help_report (void)
{
  printf (
  "\n%s - sort pictures from CSV-Files into paths\n"
  "Switches for reports:\n"
  "  -r[...] create reports:\n"
  "  -rM     Mastertech-Style\n"
  "  -rh -rm Files you have / miss\n"
  "  -ri -re incorrect / extra Files\n"
  "  -rs     Summary\n"
  "  -ra   = -rhmies\n"
  "  -rA     report ALL collections (even empty ones)\n"
  "  -rb     brief (don't include picture descriptions)\n"
  "  -rc     perform CRC-Check (NOT necessary usually !)\n"
  "  -rf     freshen reports (write only those with pics copied)\n"
  "  -rd -rD create descript.ion for ACDSee / as hidden files\n"
  "  -rE     don't create empty reports\n"
  "  -rH     write list of files you have to \"have.txt\"\n"
  "  -rHv      \"  (include descriptions)\n"
  "  -rHs      \"  (no comments for database import)\n"
  "  -rHb      \"  create binary havelist \"have.bin\"\n"
  "  -rR     recurse collection for report (not recommended !)\n"
  "  -rS     print verbose summary\n" 
  "  -ro     output summary to summary.txt\n"
  "  -rx -rX export all missings to \"missing.csv\" / \"missing_COLLECTION.csv\"\n"
  "  -rT     create HTML tables (for my homepage ;-) )\n"
  "  -rn     add numbers of have/all to the report names"
  ,version);
  error ("");
}

void help_trade (void)
{
  printf (
  "\n%s - sort pictures from CSV-Files into paths\n\n"
  
  "scansort [switches] configfile tradingfile\n"
  "Switches for trading:\n"
  "  -t[aomgzAOMGZrbfFw]  trading mode:\n"
  "  -t      give help on trading\n"
  "  -dt     set output directory for trading\n"
  "  -ds     set source directory for giving\n"
  "  -ta     make list of files to 'a'sk for\n"
  "  -to     make list of files to 'o'ffer\n"
  "  -tm     make list of files 'm'issing in both collections\n"
  "  -tgNR   copy NR files for 'g'iving\n"
  "  -tzNR   put these files in Zips with NR pics each\n"
  "          (NR >= 500: make Zips of max NR kilobyte)\n"
  "  -tZname set basename for Zips (default: \"give\")\n"
  "  -tA, -tO, -tM, -tG   use verbose names for files created\n"
  "  -tr     choose pics at 'r'andom\n"
  "  -tb     choose pics 'b'ackwards (from the end of collection)\n"
  "  -tf     'f'ake: don't copy pics\n"
  "  -tF      \" (don't check if source files can be found)\n"
  "  -tw     trade 'w'hole collections by names (not trading files)\n"
  ,version);
  error ("");
}

void help_model (void)
{
  printf (
  "\n%s - sort pictures from CSV-Files into paths\n\n"
  
  "scansort [switches] configfile\n"
  "Switches for model collections:\n"
  "  -M[acCp123456789m\"Model name\"]\n"
  "  -M      give help on model collection\n"
  "  -Ma     create a AIS file for ACDSee 2.4\n"
  "  -Mc     create a CSV (without CRCs)\n"
  "  -MC     create a CSV (with CRCs)\n"
  "  -Mp     copy pictures that fit\n"
  "  -dt     set target directory for copy\n"
  "  -M50    set fuzzy threshold to 50%% (20-100, default 60)\n"
  "  -MmPamela_Anderson    specify model to search for\n"
  "  -Mm\"Pamela Anderson\"  (use quotes for spaces)\n"
  "  -MnNEWNAME  rename pictures to NEWNAMExxx.jpg\n\n"
  
  "  -C[ecar] NAME PATH  create/update CSV file NAME from PATH\n"
  "  -Ce        existing pics only\n"
  "  -Cc        rebuild all CRCs\n"
  "  -Ca        all files (not only jpg)\n"
  "  -Cr        recurse path (without creating an E-CSV)\n"
  "  -CE     create E-CSV (no update)   -CEa  (all files)\n"
  ,version);
  error ("");
}

void help_fsplit (void)
{
  printf (
  "\n%s - sort pictures from CSV-Files into paths\n\n"
  
  "scansort [switches] base_CSV\n"
  "Switches for splitting Foyle Cyberclub CSV:\n\n"
  
  "  -F      give help on splitting Foyle Cyberclub CSV\n"
  "  -Fd     split by decade\n"
  "  -Fy     split by year\n"
  "  -Fc     split by category (L-Port, L-Head, Cover, Big Centerfold, Chippy-Data\n"
  "  -Fcd    split by category and L-Port by decade\n"
  "  -Fo     create L-Port CSV with original names\n"
  "  -Fod     \" , split by decade\n"
  "  -Fe     create a E-CSV (\\decade\\year)\n"
  "  -Fed    create a E-CSV (\\decade)\n"
  "  -Fey    create a E-CSV (\\year)\n"
  "  -FnCSV_NAME   define CSV to split (default: find automatically)\n\n"

  "  base_CSV    remove all portfolios that are identical in the base CSV\n"
  ,version);
  error ("");
}

