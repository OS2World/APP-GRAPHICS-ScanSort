/* Scansort 1.81 - CSV creation and model based collections
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

extern int collection_maxpics;
extern char * no_description;

// Switches

extern int csv_exist_only, csv_all_crc, csv_update, csv_all_exts, csv_recurse, to_upper, to_lower;
extern int model_collection, model_csv, model_picture, model_rename, model_AIS, fuzzy_schwelle;
extern char model_name[], model_new_name[], tradepath[];

void update_csv (char * name, char * dirname)
{
  mstring * ms = ms_init (m, 100);
  fileinfo * fi, fi0;
  dirhandle dh;
  char fname[MAXPATH];
  int pics_in_dir = 0;
  collection *csv;
  pic_desc *p, **pp = NULL;
  int update, i, csvpics, loop;
  unsigned int crc;
  FILE *f;
  char *c, *s;

  if ((dh = p_opendir(dirname, &fi0, csv_recurse, 0, 1)))
     do {
        if (csv_all_exts || is_tail (endung, fi0.fullname))
        {
          fi = (fileinfo *) ms_npush (ms, &fi0, sizeof fi0);
          fi->name = name_from_path (fi->fullname);
          pics_in_dir++;
        }  
     } while (p_nextdir (dh, &fi0)); 
     
  if (pics_in_dir == 0)
      lprintf(2, "no pictures found in \"%s\": %s\n", dirname, system_error());
  csv = mm_alloc(m, sizeof *csv);
  nr_of_collections = 1;
  collections = mm_alloc(m, sizeof csv);
  collections[0] = csv;
  csv->name = "created";
  csv->targetdir = dirname;
  csv->longest = 8;
  csv->nr = 0;

  if ((update = exists_length (name, 0)))	// File vorhanden, updaten
  {
    if (! readbuffer (name))
       error ("aborting");
    csv->pic_descs = mm_alloc (m, (countlines (picbuffer) + pics_in_dir) * sizeof (pic_desc));
    parse_csv (0, picbuffer);
    collection_maxpics = pics = csv->nrpics;
    make_index (-1);	// kein Check
  }
  else
  {
    pics = csv->nrpics = 0;
    csv->pic_descs = mm_alloc (m, pics_in_dir * sizeof (pic_desc));
  }
  
  for (loop = 0; loop < pics_in_dir; loop++)
  {
    fi = (fileinfo *) ms->s[loop];
    s  = fi->name;
    if (fi->isdir || !stricmp(s, LOGFILENAME) || !stricmp(s, DESCRIPT_ION))
       continue; 
    if (update && (pp = findpic_name_h(s)))
    {
      lprintf (0, "Already in collection: %s\n", s);
      p = *pp;
      if (p->size != fi->size)
      {
        lprintf (2, "Filesize changed for %s from %d to %d\n",
            p->name, p->size, fi->size);
        p->size = fi->size;    
        p->crc = 0;
      }  
    }
    else 
    {
      if (update && !pp && csv_update)
         continue;
      lprintf (0, "New picture found:     %s\n", s);
      p = csv->pic_descs + csv->nrpics++;
      p->name = mm_strdup (m, s);
      if (strlen(bn(p->name)) <= 8 && to_upper)
        str2upper (p->name);
      else if (to_lower)
        name2lower (p->name, to_lower != 1);
      p->description = "NEW";
      p->crc = 0;
    }
    p->size = fi->size;
    p->exists = yes;
    p->collection = 0;		// Das war der Crashbug !!!
    
    if (p->crc == 0 || csv_all_crc)
    {
      if (readbuffer (fi->fullname))
      {
        crc = crcblock (picbuffer, p->size);
      }
      else crc = 0;
      if (p->crc && p->crc != crc)
        lprintf (2, "CRC changed for %s from %08x to %08x\n",
            p->name, p->crc, crc);
      p->crc = crc;      
    }
    lprintf (0, "%*s  %6d  %08X  %s\n", -csv->longest-4, p->name, p->size, p->crc, p->description);
  } 
  collection_maxpics = pics = csv->nrpics;

  make_index (2);
  csvpics = 0;
  if (csv_exist_only)
    for (i = 0; i < pics; i++)
    {
      if (csv->pic_descs[i].exists == yes)
         csvpics ++;
    }
  else
    csvpics = pics;
  strcpy (fname, name);
  c = strrchr (fname, '.');
  if (!c)
     c = fname + strlen(fname);
  for (i = 0; c >= fname && '0' <= c[-1] && '9' >= c[-1]; i++)
      c--;
  if (i)
  {
    i = strtoul(c,NULL,10);
    if (i != csvpics)
       sprintf (c, "%d.csv", csvpics);
    else
       backup (fname);
  }
  else
    sprintf (c, "_%d.csv", csvpics);

  lprintf (2, "Open CSV-file %s with %d lines for writing\n", fname, csvpics);
  f = my_fopen (fname, WT);
  for (i = 0; i < pics; i++)
  {
    p = csv->pic_descs + i;
    if (!csv_exist_only || p->exists == yes)
    {
       sprintf (fname, strchr(p->name, ',') ? "\"%s\"" : "%s",
                csv_all_exts ? p->name : bn(p->name));
       fprintf (f, "%s,%d,%08X,", fname, p->size, p->crc);
       if (strcmp(p->description, no_description))
       {
        if (strchr(p->description, ','))
          fprintf (f, "\"%s\"\n", p->description);
        else    
          fprintf (f, "%s\n", p->description);
       }
       else
         fprintf (f, "\n");
    }   
  }
  fclose (f);
}

static int compare_ecsv (fileinfo ** fi1, fileinfo ** fi2)
{
  int i = m_stricmp((**fi1).fullname, (**fi2).fullname);
  if (i)
     return i;
  return m_stricmp((**fi1).name, (**fi2).name);  
}

void make_ECSV (char * name, char * dirname)
{
  int l,i,j;
  dirhandle * d;
  fileinfo  fi, *fi1;
  char fname[MAXPATH], *s;
  minimem_handle * m1 = mm_init (0x4000, 0x400);
  mstring * fis = ms_init (m1, 500);
  FILE * f;

  p_GetFullPathName (dirname, MAXPATH, fname);
  l = strlen (fname) - 1;
  if (fname[l] == PC)
    fname[l--] = 0;
  l++;
 
  d = p_opendir (fname, &fi, 1, 0, 1);
    if (!d)
       error ("Nothing found in directory %s", dirname); 
  do {
     if (csv_all_exts || is_tail (endung, fi.fullname))
     {
       if (!readbuffer(fi.fullname))
          error ("aborting");
       fi.time = crcblock (picbuffer, fi.size);
       ms_npush (fis, &fi, sizeof fi);
     }  
  } while (p_nextdir (d, &fi));
  for (i = 0; i < fis->n; i++)
  {
    fi1 = (fileinfo *) fis->s[i];
    s = name_from_path (fi1->fullname);
    fi1->name = s;
    s[-1] = 0;
  }  
  qsort (fis->s, fis->n, sizeof (fileinfo *), (QST) compare_ecsv);
  sprintf (fname, "%s_%d.csv", bn(name), fis->n);
  lprintf (2, "Open CSV-file %s with %d lines for writing\n", fname, fis->n);
  f = my_fopen (fname, WT);
   
  for (i = 0; i < fis->n; i++)
  {
    fi1 = (fileinfo *) fis->s[i];
    fprintf (f, "%s,%d,%08X,", fi1->name, fi1->size, (int) fi1->time);
    if (PC != WINPC)
      for (j = strlen (fi1->fullname) - 1; j >= l; j--)
        if (fi1->fullname[j] == PC)
           fi1->fullname[j] = WINPC;
    fprintf (f, "%s%c\n", fi1->fullname+l, WINPC);
  }
  fclose (f);
  mm_free (m1);
}

void do_model_collection(void)
{
  collection * csv;
  pic_desc * p;
  int i, j, matchn=0, matchd=0;
  int found = 0, copied = 0, ais_found = 0;
  char key[200];
  char target[MAXPATH], *target_name = NULL, *source;
  FILE *f = NULL, *AIS = NULL;
  char * csv_startname = "SSMCxxxx.csv";

  strcpy (key, model_name);
  make_search_string (key);

  if (model_AIS)
  {
    sprintf (target, "%s.ais", model_name);
    AIS = my_fopen (target, WT);
    lprintf (2, "Making AIS file %s\n", target);  
  }
  if (model_csv)
    f = my_fopen(csv_startname, WT);
  if (model_picture)
  {
    sprintf (target, "%s%s%c", tradepath, model_name, PC);
    target_name = target + strlen(target);
    clean_name (target);
    check_make_directory (target);
  }
  
  lprintf (2, "Searching database for \"%s\", threshold is %d ...\n", key, fuzzy_schwelle);
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    for (j = 0; j < csv->nrpics; j++)
    {
      p = csv->pic_descs + j;
      matchn = FuzzyMatching (key, p->name);
      matchd = FuzzyMatching (key, p->description);
      if (matchn >= fuzzy_schwelle || matchd >= fuzzy_schwelle)
      {
        found++;
        lprintf (0, "%3d %3d %s  %s  %s\n",
           matchn, matchd, csv->name, p->name, p->description);
        if (f)
        {
           if (model_rename)
             fprintf (f,"%s%03d,%d,", model_new_name, found, p->size);
           else
             fprintf (f, strchr(p->name, ',') ? "\"%s\",%d," : "%s,%d,",
			bn(p->name), p->size);
           if (model_csv == 2)
              fprintf (f, "%08X,", p->crc);
           fprintf (f, "%s", csv->name);   
           if (model_rename)
             fprintf (f," %s", bn(p->name));
           fprintf (f, ": %s\n", p->description);   
        }
        if ((model_picture || AIS) && p->exists && (source = locate_pic(p)))
        {
          if (model_picture)
          {
           if (model_rename)
             sprintf (target_name, "%s%03d.%s", model_new_name, found, endung);
           else  
             strcpy (target_name, p->name);
           if (copy_pic (source, target))
              copied++;
          }
          if (AIS)
          {
             fprintf (AIS, "\"%s\"\n", source);
             ais_found++;
          }   
        }
      }
      else if (matchn + 20 >= fuzzy_schwelle || matchd + 20 >= fuzzy_schwelle)
        lprintf (0, "(%3d %3d %s  %s  %s)\n",
           matchn, matchd, csv->name, p->name, p->description);
    }
  }
  if (AIS)
  {
    fclose (AIS);
    lprintf (2, "Created AIS file with %d entries\n", ais_found);
  }  
  if (model_picture)
  {
    *target_name = 0;
    lprintf (2, "%d pics of %d copied to %s\n", copied, found, target);
  }
  if (f)
  {
     fclose (f);
     sprintf (target, "SSMC_%s_%d.csv",
       model_rename ? model_new_name : model_name, found);
     clean_name (target);
     DeleteWaste (target);
     if (p_MoveFile (csv_startname, target))
        lprintf (2, "%d entries written to %s\n", found, target);
     else
        lprintf (2, "Couldn't rename %s to %s: %s\n", csv_startname, target, system_error());
  }
}


