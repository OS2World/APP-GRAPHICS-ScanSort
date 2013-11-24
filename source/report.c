/* Scansort 1.81 - report generation
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

extern p_FILETIME exists_length_last_filetime;
extern char * no_report, * no_description;
extern int collection_longest;
extern FILE * extralogfile;

// Switches
extern int report, report_mtcm, report_have, report_miss, report_incorrect, report_extra,
           report_summary, report_long_summary, report_brief, report_crc, report_freshen,
           report_description, report_makehavelist, report_makehavelist_s, report_inactive,
           report_noempty, report_name_numbers, report_output_summary, report_html_table,
           report_recurse;
extern char * report_identity, reportpath[], newsfilterpath[];
extern mstring *have_lists;

int gcorrect=0, gincorrect=0, gmissing=0, gextra=0, gfiles=0, gcollections=0, gmissing_in_empty=0,
    cfiles=0, ccollections=0;   // complete
double gcorrectb=0, gincorrectb=0, gmissingb=0, gextrab=0, gfilesb=0, cfilesb=0, gonhdb=0;   // Bytes


// look what pics exist on HD

void prepare_collection (int col)
{
  collection *csv = collections[col];
  fileinfo fi;
  dirhandle dh;
  
  if (report_crc)
     lprintf (5, "CRC-checking %s ...\n", csv->name); 
  if ((dh = p_opendir (csv->targetdir, &fi, report_recurse || csv->Ecsv, 0, 1)))
     do {
        if (checkendung(fi.name) && stricmp (fi.name, DESCRIPT_ION))
           checkpic_report (fi.fullname, fi.name, fi.time, fi.size, col);
     } while (p_nextdir (dh, &fi));
}

void prepare_reports(void)
{
    int i;

    lprintf (2, "Checking for pics on HD...\n");
    for (i = 0; i < nr_of_collections; i++)
        prepare_collection(i);
}

static FILE * rf;

void report_open (collection * csv)
{
  char b[MAXPATH], *c;

  if (!rf && strcmp (csv->reportfile, no_report)
      && (csv->updated || !report_freshen || !exists_length(csv->reportfile, 0)
          || p_CompareFileTime(exists_length_last_filetime, csv->filetime) == -1 ))
  {
    strcpy (b, csv->reportfile);
    if (( c = strrchr (b, PC) ))   // = is correct
    {
      *c = 0;
      check_make_directory (b);
    }
    rf = fopen (csv->reportfile, WT);
    if ( rf == NULL)
      lprintf (2, "Cannot open report file %s: %s\n", csv->reportfile, system_error());
    else
    {
     fprintf (rf, "%s Collection Manager Report\n%s",  // cr ist schon im Datum enthalten
       version, zeit );
     if (report_identity)
       fprintf (rf, "%s\n", report_identity);
     fprintf (rf,   "Collection: %s\n"
      "CSV file:   %s    %s\n",
      csv->targetdir, csv->filename, timestr(csv->filetime));
    }  
  }
}

char * nobn (char * s)
{
  return s;
}  

static int compare_picdesc_ecsv (const pic_desc ** p1, const pic_desc ** p2)
{
  int i = m_stricmp((**p1).description, (**p2).description);
  if (i)
     return i;
  return m_stricmp((**p1).name, (**p2).name);
}


void report_collection (int col)
{
  collection *csv = collections[col];
  int i, fl;
  char *status[4] = {"Missing","OK","Bad Filesize","Wrong CRC"};
  pic_desc * p;
  pic_desc ** pp;
  double perc;
  int  something_written;
  char name[MAXPATH];
  char * (* mbn) (char *) = csv->specialtypes ? nobn : bn;

  something_written = csv->extra && report_extra;
  csv->correct = csv->incorrect = csv->missing = 0;
  csv->correctb = csv->incorrectb = csv->missingb = 0;
  if (!report_inactive && !checktargetpath (csv->targetdir, 0, csv->nr)
      && csv->empty)  // nicht existent
  {
      csv->missing = csv->nrpics;
      gmissing_in_empty += csv->nrpics;
      return;
  }  
  perc = csv->nrpics / 100.0;
  rf = NULL;

  // Index erzeugen (ggf sortierbar)
  pp = mymalloc(csv->nrpics * sizeof (pic_desc*));
  for (i = csv->nrpics; i--;)
    pp[i] = csv->pic_descs + i;

  if (csv->Ecsv)	// nach Kommentaren sortieren
    qsort (pp, csv->nrpics, sizeof (pic_desc *), (QST) compare_picdesc_ecsv);

  if (report_mtcm) {
   report_open (csv);
   something_written = 1;
   if (rf)
   {
    fprintf (rf,
    "\nStatus        %*s      File Size      CRC-32       Description\n"
    "------        %*s      ---------     --------      -----------\n",
     -csv->longest, "Filename", -csv->longest, "--------");

    for (i = 0; i < csv->nrpics; i++)
    {
      p = pp[i];
      fprintf (rf,"%-14s%*s      %8d      %08x",
        status[(int)p->exists], -csv->longest, mbn(p->name), p->size, p->crc);
      if ((report_brief && !csv->Ecsv) || !strcmp(p->description, no_description))
        fprintf (rf, "\n");
      else
        fprintf (rf, "      %s\n", p->description);
    }
   }
  } 

  fl = 1;
  for (i = 0; i < csv->nrpics; i++)
  {
    p = pp[i];
    if (p->exists == yes)
    {
      if (report_have && fl)
      {
        report_open (csv);
        if (rf)
          fprintf (rf, "\nFiles in Collection:\n");
        fl = 0;
        something_written = 1;
      }
      if (report_have && rf)
      {
        fprintf (rf,"%*s      %8d",
          -csv->longest, mbn(p->name), p->size);
        if ((report_brief && !csv->Ecsv) || !strcmp(p->description, no_description))
          fprintf (rf, "\n");
        else
          fprintf (rf, "      %s\n", p->description);
      }
      csv->correct++;
      csv->correctb += p->size;
    }
  }

  fl = 1;    
  for (i = 0; i < csv->nrpics; i++)
  {
    p = pp[i];
    if (p->exists == no)
    {
      if (report_miss && fl)
      {
        report_open (csv);
        if (rf)
          fprintf (rf, "\nFiles missing:\n");
        fl = 0;
        something_written = 1;
      }
      if (report_miss && rf)
      {
        fprintf (rf,"%*s      %8d",
          -csv->longest, mbn(p->name), p->size);
        if ((report_brief && !csv->Ecsv) || !strcmp(p->description, no_description))
          fprintf (rf, "\n");
        else
          fprintf (rf, "      %s\n", p->description);
      }
      csv->missing++;
      csv->missingb += p->size;
    }
  }  
    
  fl = 1;
  for (i = 0; i < csv->nrpics; i++)
  {
    p = pp[i];
    if (p->exists == wrongsize)
    {
      if (report_incorrect && fl)
      {
        report_open (csv);
        if (rf)
          fprintf (rf, "\nWrong Size:\n");
        fl = 0;
        something_written = 1;
      }
      if (report_incorrect && rf)
      {
        fprintf (rf,"%*s      %8d  (correct)    %8d  (in collection)",
          -csv->longest, mbn(p->name), p->size, *(int *)(p->description - 4));
        if ((report_brief && !csv->Ecsv) || !strcmp(p->description, no_description))
          fprintf (rf, "\n");
        else
          fprintf (rf, "      %s\n", p->description);
      }
      csv->incorrect++;
      csv->incorrectb += p->size;
    }  
  }
    
  fl = 1;
  for (i = 0; i < csv->nrpics; i++)
  {
    p = pp[i];
    if (p->exists == wrongcrc)
    {
      if (report_incorrect && fl)
      {
        report_open (csv);
        if (rf)
          fprintf (rf, "\nWrong CRC:\n");
        fl = 0;
        something_written = 1;
      }
      if (report_incorrect && rf)
      {
        fprintf (rf,"%*s      %8d      %8x  (correct)    %8x  (in collection)",
          -csv->longest, mbn(p->name), p->size, p->crc, *(int *)(p->description - 4));
        if ((report_brief && !csv->Ecsv) || !strcmp(p->description, no_description))
          fprintf (rf, "\n");
        else
          fprintf (rf, "      %s\n", p->description);
      }
      csv->incorrect++;
      csv->incorrectb += p->size;
    }
  }

  if (csv->extras && (report_mtcm || report_extra))
  {
    qsort (csv->extras->s, csv->extras->n, sizeof (char *), (QST) compare_text_line);
    report_open (csv);
    something_written = 1;
    if (rf)
    {
      fprintf (rf, "\nExtra files:\n");
      for (i = 0; i < csv->extras->n; i++)
        fprintf (rf, "%s", csv->extras->s[i]);
    }
  }
  
  if (report_mtcm || report_summary)
  {
    if (!report_noempty)
       report_open (csv);
    if (rf)   
       fprintf (rf,
  "\nSummary Information\n"
  "-------------------\n"
  "Files in Database:  %d  (%d k)\n\n" 
  "Correct:    %4d   (%5.1f%%)   (%7d k)\n"
  "Incorrect:  %4d   (%5.1f%%)   (%7d k)\n"
  "Missing:    %4d   (%5.1f%%)   (%7d k)\n"
  "Extra:      %4d              (%7d k)\n\n"
  "Total of %d kilobytes in %d files.\n",
        csv->nrpics, kb(csv->nrbytes),
        csv->correct, csv->correct/perc, kb(csv->correctb),
        csv->incorrect, csv->incorrect/perc, kb(csv->incorrectb),
  	csv->missing, csv->missing/perc, kb(csv->missingb),
  	csv->extra, kb(csv->extrab),
  	kb(csv->correctb+csv->incorrectb+csv->extrab),
  	  csv->correct+csv->incorrect+csv->extra);
  }	  
  if (rf)
    fclose (rf);
  if (report_noempty && !something_written)
    p_DeleteFile (csv->reportfile);
  else if (rf)
  {
    if (report_name_numbers)
    {
      sprintf (name, "%s_%d-%d.txt", bn(csv->reportfile), csv->correct, csv->nrpics);
      p_DeleteFile (name);
      p_MoveFile (csv->reportfile, name);    
    }
    lprintf (0,"Write reportfile %s\n", report_name_numbers ? name : csv->reportfile);
  }

  if (!csv->empty)
  {
    gcorrect += csv->correct;
    gcorrectb += csv->correctb;
    gincorrect += csv->incorrect;
    gincorrectb += csv->incorrectb;
    gmissing += csv->missing;
    gmissingb += csv->missingb;
    gextra += csv->extra;
    gextrab += csv->extrab;
    gcollections++;
    gfiles += csv->nrpics;
    gfilesb += csv->nrbytes;
    gonhdb += csv->onhdb;
    if (csv->correct == csv->nrpics)
    {
      ccollections++;
      cfiles += csv->nrpics;
      cfilesb += csv->correctb;
    }
  }
  if (report_description && checktargetpath (csv->targetdir, 0, col))
  {
    sprintf (name, "%s%c%s", csv->targetdir, PC, DESCRIPT_ION);
    p_DeleteFile(name);
    if ( !csv->Ecsv && csv->descriptions )
    {
      rf = fopen (name, WT);
      if ( rf == NULL)
      {
        lprintf (2, "Cannot open description file %s: %s\n", name, system_error());
        return;
      }
      for (i = 0; i < csv->nrpics; i++)
      {
        p = pp[i];
        if (p->exists != no)
        {
          fprintf (rf,"%s %s\n", p->name, p->description);
        }  
      }
      fclose (rf);
      if (report_description == 2)
        p_MakeFileHidden (name);
    }
  }
  myfree(pp);
}

void generate_reports(void)
{
    double perc = pics / 100.0;
    int tab = collection_longest;
    int i, ld, j;
    char * message[3] = {"Complete:", "Incomplete:", "Inactive:"};

    if (tab > 32)
       tab = 32;
    lprintf (2,"Generating reports ...\n");
    for (i = 0; i < nr_of_collections; i++)
        report_collection(i);
    perc = gfiles / 100.0;
    if (perc == 0.0)
       perc = 100.0;
    ld = report_long_summary ? 2 : 0;

    if (report_output_summary)
    {
      char s[MAXPATH];
      sprintf (s, "%s%s", reportpath, "summary.txt");
      extralogfile = fopen (s, WT);
      lprintf (4,
        "%s Summary Report\n"
        "%s\n", version, zeit);
    }
    lprintf (ld,"%*sTOTAL    OK   BAD  MISS EXT OK[%%]  OK[k] MIS[k]",
         -tab, "Collection:");
    if (have_lists->n)
       lprintf (0, "  HD[k] Location of pics");     
    lprintf (ld,"\n");     
    for (j = 0; j <= 1 + report_inactive; j++)     
    {
      lprintf (0, "%s\n", message[j]);
      for (i = 0; i < nr_of_collections; i++)
      {
        collection * csv = collections[i];
        double cperc = (csv->correct * 100.0) / csv->nrpics;
        int k;

        if (csv->correct < csv->nrpics && cperc > 99.9) 
           cperc = 99.9;
        switch (j) {
          case 0: if (csv->correct != csv->nrpics)	          // complete collections
                     continue;
                  break;   
          case 1: if (csv->correct == csv->nrpics || csv->empty)  // incomplete collections
                     continue;
                  break;   
          case 2: if (!csv->empty)	  			  // inactive collections
                     continue;
                  break;   
        }
        lprintf (ld,"%*s%5d%6d%6d%6d%4d%6.1f%7d%7d", -tab,
          csv->name, csv->nrpics, csv->correct, csv->incorrect, csv->missing,
          csv->extra, cperc, kb(csv->correctb), kb(csv->missingb));
        if (have_lists->n)
        {
          lprintf (0, "%7d", kb(csv->onhdb));
          if (csv->onhdb > 0)
            lprintf (0, " HD");
          for (k = 0; k < have_lists->n; k++)
            if (bf_test(csv->haveindex, k))
            {
               char s[MAXPATH];
               strcpy (s, bn(name_from_path(have_lists->s[k])));
               str2upper (s);
               lprintf (0, " %s", s);
            }   
        }    
        lprintf (ld,"\n");  
      }
    }  
    lprintf (ld,"%*s%6d%6d%6d%6d%4d%6.1f%7dM%6dM", -(tab-1),
         "Summary", gfiles, gcorrect, gincorrect, gmissing,
         gextra, gcorrect/perc, mb(gcorrectb), mb(gmissingb) );
    if (have_lists->n)
       lprintf (0, "%6dM", mb(gonhdb));
    lprintf (ld, "\n");   
    if (extralogfile)
    {
       fclose (extralogfile);
       extralogfile = NULL;
    }   
    lprintf (2-ld,
  "\n\nSummary Information\n"
  "-------------------\n"
  "Database:   %6d  collections  with %6d files (%9d k)\n" 
  "Active:     %6d  collections  with %6d files (%9d k)\n" 
  "Complete:   %6d  collections  with %6d files (%9d k)\n\n" 
  "Correct:    %6d  (%9d k) (%4.1f%%)\n"
  "Incorrect:  %6d  (%9d k) (%4.1f%%)\n"
  "Missing:    %6d  (%9d k) (%4.1f%%)\n"
  "Extra:      %6d  (%9d k)\n\n",
      nr_of_collections, pics, kb(picsb),
      gcollections, gfiles, kb(gfilesb),
      ccollections, cfiles, kb(cfilesb),
      gcorrect, kb(gcorrectb), gcorrect/perc,
      gincorrect, kb(gincorrectb), gincorrect/perc,
      gmissing, kb(gmissingb), gmissing/perc,
      gextra, kb(gextrab));
}  

void check_create_csvzip (char * s, collection * csv)
{
  char cmd[MAXPATH], s1[MAXPATH];

      sprintf (s, "M_%s_%d.zip", csv->name, csv->nrpics);
      clean_name (s);
      sprintf (s1, "%s%s", reportpath, s);
      if (!exists_length (s1, 0))
      {
        lprintf (2, "Creating Zip %s\n", s1);
        sprintf (cmd, "zip -9 -j %s %s ", s1, csv->fullfilename);
        lprintf (1, "%s\n", cmd);
        system (cmd);
      }  
}

void make_htmltable (void)
{
  collection * csv;
  int i;
  FILE *fi, *fo;
  char * Ni = "trade_tp.html";
  char * No = "trade.html";
  char s[MAXPATH], s1[MAXPATH], b[405];

  sprintf (s, "%s%s", reportpath, Ni);
  fi = my_fopen(s, RT);

  sprintf (s, "%s%s", reportpath, No);
  fo = my_fopen(s, WT);

  while (!feof(fi))
  {
    fgets (b, 400, fi);
    if (!strcmp (b, "CHANGE-DATE\n"))
    {
      fprintf (fo, "%s", zeit);
    }
    else if (!strcmp (b, "COMPLETE-TABLE\n"))
    {
	  for (i = 0; i < nr_of_collections; i++)
	  {
	    csv = collections[i];
	    if (csv->correct == csv->nrpics)
	    {
	      check_create_csvzip (s, csv);
	      fprintf (fo, "<TR><TD><A HREF=\"%s\">%s</A></TD><TD><CENTER>%d</CENTER></TD></TR>\n",
	        s, csv->name, csv->nrpics);
	    }
	  }
    }
    else if (!strcmp (b, "INCOMPLETE-TABLE\n"))
    {
	  for (i = 0; i < nr_of_collections; i++)
	  {
	    csv = collections[i];
	    if (csv->correct < csv->nrpics)
	    {
	      check_create_csvzip (s, csv);
	      fprintf (fo, "<TR><TD><A HREF=\"%s\">%s</A></TD><TD><CENTER>%d</CENTER></TD>",
	        s, csv->name, csv->nrpics);
	      sprintf (s, "%s.txt", csv->name);
	      clean_name (s);
	      if (csv->missing == csv->nrpics || report_html_table == 2)
	        fprintf (fo, "<TD><CENTER>%d</A></CENTER></TD></TR>\n", csv->missing);
	      else
	      {
	        fprintf (fo, "<TD><CENTER><A HREF=\"%s\">%d</A></CENTER></TD></TR>\n",
	          s, csv->nrpics - csv->correct);
	        sprintf (s1, "%s%s", reportpath, s);  
	        if (!exists_length (s1, 0))
	          lprintf (2, "Missing Report: %s\n", s1);
	      }    
	    }
	  }
    }
    else
      fputs (b, fo);
  }  
  fclose (fi);
  fclose (fo);
  sprintf (s, "%srequests.zip", reportpath);
  lprintf (2, "Writing %s\n", s);
  p_DeleteFile (s);
  sprintf (b, "zip -9 -j -q %s %s*.txt %smissing.csv", s, reportpath, reportpath);
  lprintf (1, "%s\n", b);
  system (b);
}

void make_missingcsv (void)
{
  collection * csv;
  pic_desc * p;
  int i,j;
  FILE *fo;
  char * No = "missing.csv";
  char s[MAXPATH];

  sprintf (s, "%s%s", reportpath, No);
  fo = my_fopen(s, WT);
  lprintf (2, "Writing %s\n", s);

  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    if (csv->correct < csv->nrpics && (!csv->empty || report_inactive))
      for (j = 0; j < csv->nrpics; j++)
      {
        p = csv->pic_descs + j;
        if (p->exists != yes)
        {
          fprintf (fo, "%s,%d,%08X,%c%s%c",
            p->name, p->size, p->crc, WINPC, csv->name, WINPC);
          if (csv->Ecsv)
             fprintf (fo, "%s,\n", p->description + 1);   // comma for the stupid hunter proggie
          else
             fprintf (fo, "\n");
        }     
      }
  }
  fclose (fo);
}

void make_missingcsvs (void)
{
  collection * csv;
  pic_desc * p;
  int i,j;
  FILE *fo;
  char s[MAXPATH];

  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    if (csv->correct < csv->nrpics && (!csv->empty || report_inactive))
    {
      sprintf (s, "%smissing_%s_%d.csv", reportpath, csv->name, csv->incorrect + csv->missing);
      fo = my_fopen(s, WT);
      lprintf (1, "Writing %s\n", s);
      for (j = 0; j < csv->nrpics; j++)
      {
        p = csv->pic_descs + j;
        if (p->exists != yes)
        {
          fprintf (fo, "%s,%d,%08X,", p->name, p->size, p->crc);
          if (csv->Ecsv)
             fprintf (fo, "%s,\n", p->description);	// comma for the stupid hunter proggie
          else
             fprintf (fo, "MISSING\n");
        }     
      }
      fclose (fo);
    }  
  }
}

/* Make Filters for Forte Newsagent */
int filterlimit = 29000;	// max. size of filter file

typedef struct {
int firstline;
int filenr;
int bytes;
FILE * f;
char basename[MAXPATH];
} filtstr;

static void filt_open (filtstr * fs, char * name)
{
  fs->firstline = 1;
  fs->filenr = 1;
  fs->bytes = 0;
  strcpy (fs->basename, name);
  fs->f = my_fopen (name, WT);
  fprintf (fs->f, "subject: (");
}  

static char or[] = " or\n";
static int  orl;

static void filt_line (filtstr * fs, char * name, int size)
{
  int l;
  char s[MAXPATH], n[MAXPATH];

  if (size == 0)
     sprintf (s, "\"%s\"", name);
  else
     sprintf (s, "(\"%s\" and %d)", name, size);
  l = strlen (s);
  if (fs->bytes + orl + l > filterlimit)
  {
  // limit reached ! close, rename, reopen
     fprintf (fs->f, ")\n");
     fclose (fs->f);
     if (fs->filenr == 1)
     {	// first one: rename
        strcpy (n, fs->basename);
        strcpy (strrchr (n, '.'), "_part01.txt");
        p_MoveFile (fs->basename, n);
     }
     fs->filenr++;
     strcpy (n, fs->basename);
     sprintf (strrchr (n, '.'), "_part%02d.txt", fs->filenr);
     fs->f = my_fopen (n, WT);
     fprintf (fs->f, "subject: (");
     fs->firstline = 1;
     fs->bytes = 10;
  }
  if (fs->firstline)
     fs->firstline = 0;
  else
  {
     fprintf (fs->f, or);
     fs->bytes += orl;
  }
  fprintf (fs->f, s);
  fs->bytes += l + 1;	// add one for the carrige return (not necessary for Unix, but no harm done)
}


void make_newsreader_filters (void)
{
  collection * csv;
  pic_desc * p;
  int i,j;
  char s[MAXPATH];
  filtstr fs1, fs2, fs3, fs4;
  minimem_handle * m1;
  mstring * ms;
  
  lprintf (2, "Writing filters for News Agent...\n");
  orl = strlen (or);

  // first delete all existing filter files
  m1 = mm_init (0x4000, 0x400);
  ms = ms_init (m1, 50);  
  check_make_directory (newsfilterpath);
  sprintf (s, "%sfilter*.txt", newsfilterpath);
  j = expand_wildcard (ms, s, 0);
  for (i = 0; i < j; i++)
    p_DeleteFile (ms->s[i]);
  mm_free (m1);
  
  sprintf (s, "%sfilterA_ALL_%d.txt", newsfilterpath, gincorrect + gmissing);
  filt_open (&fs1, s);
  
  sprintf (s, "%sfilterB_ALL_%d.txt", newsfilterpath, gincorrect + gmissing);
  filt_open (&fs2, s);
  
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    if (csv->correct < csv->nrpics && (!csv->empty || report_inactive))
    {
      sprintf (s, "%sfilterA_%s_%d.txt", newsfilterpath, csv->name, csv->incorrect + csv->missing);
      filt_open (&fs3, s);
      sprintf (s, "%sfilterB_%s_%d.txt", newsfilterpath, csv->name, csv->incorrect + csv->missing);
      filt_open (&fs4, s);
      for (j = 0; j < csv->nrpics; j++)
      {
        p = csv->pic_descs + j;
        if (p->exists != yes)
        {
          filt_line (&fs1, bn(p->name), 0);
          filt_line (&fs3, bn(p->name), 0);
          filt_line (&fs2, bn(p->name), p->size);
          filt_line (&fs4, bn(p->name), p->size);
        }     
      }
      fprintf (fs3.f, ")\n");
      fclose (fs3.f);
      fprintf (fs4.f, ")\n");
      fclose (fs4.f);
    }  
  }
  fprintf (fs1.f, ")\n");
  fclose (fs1.f);
  fprintf (fs2.f, ")\n");
  fclose (fs2.f);
}

void make_comparelist_string (char * s2, char * s1)
{
  int i = 0;
  do if ((s2[i] = *s1++) != ' ') i++;  // Leerzeichen raushauen wg. Supernews bug
     while (s1[-1]);
  make_search_string (s2);
}

/*  Comparelist: this is "internal-use-only" (for me personal ;-) ) at the moment
from,subject,adresse
Subject isolieren, fuer Namensvergleich aufbereiten, mit allen Missings vergleichen
*/

void make_comparelist (void)
{
  FILE * fi, *fo;
  char * inname  = "list.txt";
  char * outname = "getlist.txt";
  minimem_handle * m1;
  char s1[1010], s2[1010], *s3;
  char ** missings;
  collection * csv;
  pic_desc * p;
  int i,j,k,l;
  int nr_miss = gincorrect + gmissing + gmissing_in_empty;

  lprintf (2, "Checking News postings in %s against %d missing files...\n",
              inname, nr_miss);

  fi = my_fopen(inname, RT);
  fo = my_fopen(outname, WT);

  m1 = mm_init (40000, 1000);
  missings = mm_alloc (m1, (nr_miss + 100) * sizeof (char *));
  k = l = 0;
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    if (csv->correct < csv->nrpics)
      for (j = 0; j < csv->nrpics; j++)
      {
        p = csv->pic_descs + j;
        if (p->exists != yes)
        {
          make_comparelist_string (s2, p->name); 
          missings[k++] = mm_strdup (m1, s2);
          lprintf (0, "%4d  %s  %s\n",k-1, p->name, missings[k-1]);
        }  
      }
  }
  for (i = 0; i < k; i++)
    lprintf (0, "%3d  >%s<\n", i, missings[i]);  
  while (!feof(fi))
  {
    l++;
    fgets (s1, 1000, fi);
    if ((s3 = strchr (s1, ',')))
    {
      strcpy (s2, s3+1);
      if ((s3 = strchr (s2, ',')))
         *s3 = 0;
      else
         continue;   // fehlerhafte Zeile
      make_comparelist_string (s2, s2);
      for (i = 0; i < k; i++)
        if (strstr (s2, missings[i]) && !strstr(s2,"(1/2)") && !strstr(s2,"(2/2)"))
        {
          lprintf (2, "%5d %5d >%s<\n", l, i, missings[i]);
          fprintf (fo, "%s - %s", missings[i], s1);
          missings[i] = version;		// find only one occurence
          break;  // Zeile fertig
        }
    }
  }
  mm_free (m1);
  fclose (fi);
  fclose (fo);
}




