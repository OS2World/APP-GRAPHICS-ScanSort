/* Scansort 1.81 - trading 
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

// Switches
extern int verbose, no_underscores, trade_fake, trade_zip, trade_whole_collections,
           trade_ask, trade_offer, trade_missing, trade_give, trade_give_long, trade_order;
extern char tradepath[], * zipbasename;           
extern mstring *trade_sourcepaths;


int    files_given = 0;			// values reached
int    kbytes_given = 0;
int    trade_give_enough = 0;		// break variable

static int compare_int (const int * p1, const int * p2)
{
  if (*p1 > *p2)
     return 1;
  if (*p1 < *p2)
     return -1;
  else
     return 0;
}

// field of randoms - contains all indices in the desired order
// 0: alphabetical   1: random  2: by size  3: reverse alphabetical 

static int * zufalls_feld (int col, int mode)   // max 32767
{
  static int *f = NULL;
  int i;
  collection * csv = collections[col];
  int len = csv->nrpics;

  if (!f)
    f = mm_alloc(m, collection_maxpics * sizeof (int));

  if (mode == 0 || len > 0x7fff) 	// alphabetisch
  {
    for (i = len; i--;)
      f[i] = i; 
  }
  else if (mode == 3)			// alphabetisch rueckwaerts
  {
    for (i = len; i--;)
      f[i] = len - i - 1; 
  }
  else if (mode == 1)			// zufaellig
  {
  
    for (i = len; i--;)
    {
      f[i] = rand() * 0x8000 + i;
//      lprintf (0, "%3d  %12d\n", i, f[i]); 
    }  
    
    qsort (f, len, sizeof (int), (QST) compare_int);

    for (i = len; i--;)
    {
//   int t=f[i];
      f[i] &= 0x7fff;     
//    lprintf (0, "%3d  %d  %12d\n", i, f[i], t); 
    }
  }
  return f;
}

static int check_pattern (char *s, char ** p)
{
  int i;

  for (i = 0; p[i]; i++)
    if (strstr (s, p[i]))
      return 1;
  return 0;
}

 
// Pic suchen fuer trading u. Model-Collection (nicht reentrant)
char * locate_pic (pic_desc * p)
{
  static char source[MAXPATH], _name[MAXPATH], *name = _name+1;
  int found, j;
  collection * csv = collections[p->collection];
  static int first_time = 1;

  if (csv->Ecsv)
     sprintf (_name, "%s%s", p->description, p->name);
  else
     strcpy (name, p->name);
     
  sprintf (source, "%s%c%s", csv->targetdir, PC, name);
  found = exists_length (source, p->size);
  if (first_time)
    for (j = 0; j < trade_sourcepaths->n; j++)
    {
      if (!checktargetpath (trade_sourcepaths->s[j], 0, -1))
      {
         lprintf (2, "nothing found in %s , ignored\n", trade_sourcepaths->s[j]);
         trade_sourcepaths->s[j][0] = 0;
      }   
      first_time = 0;
    }
  for (j = 0; j < trade_sourcepaths->n && !found; j++)
  {
    if (!trade_sourcepaths->s[j][0])
       continue;
    sprintf (source, "%s%s", trade_sourcepaths->s[j], name);
    found = exists_length (source, p->size);
    if (!found)
    {
      sprintf (source, "%s%s%c%s", trade_sourcepaths->s[j], csv->name, PC, name);
      found = exists_length (source, p->size);
    }
  }
  return found ? source : NULL;
}

void do_trading (char * reportname, int multi)
{
  char * report, *c, *c2, *c3;
  static int report_len = 0;
  int l,i,j;
  int col = -1;
  int asked=0, offered=0, missing=0, given=0, needed=0;
  int asked_b=0, offered_b=0, missing_b=0, needed_b=0;
  collection * csv = NULL;
  pic_desc * p, *p1, **pp;
  int minlen = 10000;
  FILE * f;
  char pfad[MAXPATH];
// no == missing, yes == having  
  int global_mode = no;
  int local_mode;
  int found_have = 0, found_miss = 0;

  char * global_have[] = {"IN COLLECTION", "HAVE", "HAVING", NULL};
  char * global_miss[] = {"MISS", "WRONG", NULL};
  char * local_have[] = {"OK", "O.K.", "GOOD", "HAVE", "VALID", "CORRECT", "OKAY",
  			 "UNKNOWN", "EXTRA", NULL};
  char * local_miss[] = {"MISSING", "BAD", "WRONG", NULL};

#define DT_TOKS 10
  char * toks [DT_TOKS];
  int    tokl [DT_TOKS];
  int    toknr, itok;

  if (trade_fake)
     trade_zip = 0;

  if (trade_whole_collections)
  {
    make_csv_name_string (pfad, reportname);    
    for (i = 0; i < nr_of_collections; i++)
    {
      csv = collections[i];
      if (!strcmp (pfad, csv->signame))
         break;
    }
    if (i == nr_of_collections)
    {
      lprintf (2, "No collection named %s found (maybe not in config file)\n", reportname);
      return;
    }
    col = i;
  }
  else
  {
  if (!readfile (reportname, &report, &report_len))
    return;
  c = report;
  str2upper (c);
  for (i=0; c[i]; i++)		// Draco-Reports
    if (c[i] == '|')
      c[i] = ' ';
  while ((toknr = linetokens (c, &c, DT_TOKS, toks, tokl, 0, 0)))
  {
    l = 0;
    for (itok = 1; itok < toknr; itok++)
    {
      l = strtoul(toks[itok],NULL,10);
      if (l >= minlen)
         break;
    }
    if (l >= minlen)
    {
      do
        itok--;
      while (itok > 0 && tokl[itok] < 3);	// einzelne '-' o. ae. weg  
      c2 = toks[itok];
      c2[tokl[itok]] = 0;
      // Endung entfernen - auch .j, .jp (Mastertech-Schrott) (alles ist uppercase)
/*      if ((c3 = strrchr(c2, '.')) && (!c3[1] || c3[1]=='J'
                                  && (!c3[2] || c3[2]=='P' && !c3[3]) ))  
         *c3 = 0;   siehe unten
*/
      if (! no_underscores)     
         for (c3 = c2; c3 > toks[0]; c3--)
             if (*c3 == ' ')
                 *c3 = '_';
      for (pp = findpic_size_h (l); pp && *pp; pp++)
      {
        p = *pp;
        if (p->size != l) continue;
        for (j = itok; j >= 0 && (m_stricmp (toks[j], bn(p->name))
                                  && m_stricmp (bn(toks[j]), bn(p->name)) ); j--);
        if (j < 0)
           continue;
        if (col == -1)
        {
          col = p->collection;
          csv = collections[col];
          lprintf (2, "Identified collection: %s  (%d Pictures)\n", csv->name, csv->nrpics);
        }
        if (col == p->collection)
        { // identifiziert
          local_mode = global_mode;
          if (j > 0)		// nur wenn 1. String nicht der Name
          {
            toks[1][-1] = 0;
            if (check_pattern(toks[0], local_have))
	      local_mode = yes;
	    else if (check_pattern(toks[0], local_miss))
	      local_mode = no;
	  }    
	  p->exists_trade = local_mode;
	  if (local_mode)
	    found_have++;
	  else
	    found_miss++;
	  if (verbose)  
	    lprintf (0, "%s %s\n", local_mode ? "Have" : "Miss", bn(p->name));
        }
      }
    }
    else
    { // have/miss suchen
      c[-1] = 0;  
      if (check_pattern(toks[0], global_have))
        global_mode = yes;
      else if (check_pattern(toks[0], global_miss))
        global_mode = no;
    }
  }
  if (col == -1)   // Plan B: let's assume we have just missing filenames, one per line
  {
    if (!readfile (reportname, &report, &report_len))  // reload
      return;
    c = report;
    while ((toknr = linetokens (c, &c, DT_TOKS, toks, tokl, 0, 0)))
    {
      char * name = toks[0];
  
      name[tokl[0]] = 0;
      for (pp = findpic_name_h (name); pp && *pp; pp++)
      {
        p = *pp;
        if (strchr(name, '.'))
        {
          if (m_stricmp(p->name, name))
             continue;
        }
        else if (m_stricmp(bn(p->name), name))
                continue;
        if (col == -1)
        {
          col = p->collection;
          csv = collections[col];
          lprintf (2, "Identified collection: %s  (%d Pictures)\n", csv->name, csv->nrpics);
        }
        else if (p->collection != col)
          continue;
	p->exists_trade = no;
	found_miss++;
      }
    }
  }
  if (col == -1)
  {
    lprintf (2, "No collection found in report %s (maybe not in config file)\n", reportname);
    return;
  }
  }
  p1 = csv->pic_descs;  
  if (found_miss == 0)    // nur have im report -> unknown auf miss setzen
  {
    for (i = 0; i < csv->nrpics; i++)
    {
      if (p1[i].exists_trade == unknown)
      {
         p1[i].exists_trade = no;
         found_miss++;
      }   
    }  
    lprintf (2, "Assuming pics not mentioned to be missing\n");
  }
  else if (found_have == 0)	// nur miss im report -> unknown auf have setzen
  {
    for (i = 0; i < csv->nrpics; i++)
    {
      if (p1[i].exists_trade == unknown)
      {
         p1[i].exists_trade = yes;
         found_have++;
      }   
    }  
    lprintf (2, "Assuming pics not mentioned to be in collection\n");
  }       
  lprintf (2, "Trader has %d files in collection and needs %d files\n", found_have, found_miss);

  prepare_collection (col);
  f = NULL;
  if (trade_ask)
  {
    for (i = 0; i < csv->nrpics; i++)
    {
      p = p1 + i;
      if (p->exists_trade == yes && p->exists == no)
      {
        if (!f)
	{
	    if (trade_ask == 1)
	      sprintf (pfad, "%sask.txt", tradepath);
	    else 
	      sprintf (pfad, "%sask_%s.txt", tradepath, csv->name);
	    f = my_fopen(pfad, WT);
	    fprintf (f,  
	      "%s automatically generated trading list\n"
	      "%s"			// cr ist schon im Datum enthalten
	      "Collection: %s\n"
	      "CSV file:   %s\n\n"
	      "Please send me some of these files I'm missing:\n\n" ,
	      version,
	      zeit, csv->name, csv->filename);
	}
	asked++;
        asked_b += p->size;
        fprintf (f,"%*s      %8d      %s\n",
          -csv->longest, bn(p->name), p->size, p->description);
      }
    }
    if (f)
    {
      fprintf (f, "\n%d files to ask for (%d k)\n", asked, kb(asked_b));
      fclose (f);
      f = NULL;
    }  
    lprintf (2, "%d files to ask for\n", asked);
  }

  if (trade_offer)
  {
    for (i = 0; i < csv->nrpics; i++)
    {
      p = p1 + i;
      if (p->exists_trade == no && p->exists == yes)
      {
        if (!f)
        {
	    if (trade_offer == 1)
	      sprintf (pfad, "%soffer.txt", tradepath);
	    else 
	      sprintf (pfad, "%soffer_%s.txt", tradepath, csv->name);
	    f = my_fopen(pfad, WT);
	    fprintf (f,  
	      "%s automatically generated trading list\n"
	      "%s"			// cr ist schon im Datum enthalten
	      "Collection: %s\n"
	      "CSV file:   %s\n\n"
	      "I can offer you these files I have:\n\n" ,
	      version,
	      zeit, csv->name, csv->filename);
	}      
        offered++;
        offered_b += p->size;
        fprintf (f,"%*s      %8d      %s\n",
          -csv->longest, bn(p->name), p->size, p->description);
      }
    }
    if (f)
    {
      fprintf (f, "\n%d files to offer (%d k)\n", offered, kb(offered_b));
      fclose (f);
      f = NULL;
    }  
    lprintf (2, "%d files to offer\n", offered);
  }
  
  if (trade_missing)
  {
    for (i = 0; i < csv->nrpics; i++)
    {
      p = p1 + i;
      if (p->exists_trade == no && p->exists != yes)
      {
        if (!f)
        {
	    if (trade_missing == 1)
	      sprintf (pfad, "%smissing.txt", tradepath);
	    else 
	      sprintf (pfad, "%smissing_%s.txt", tradepath, csv->name);
	    f = my_fopen(pfad, WT);
            fprintf (f,  
	      "%s automatically generated trading list\n"
	      "%s"			// cr ist schon im Datum enthalten
	      "Collection: %s\n"
	      "CSV file:   %s\n\n"
	      "Files missing in both collections:\n\n" ,
	      version,
	      zeit, csv->name, csv->filename);
	}      
        missing++;
        missing_b += p->size;
        fprintf (f,"%*s      %8d      %s\n",
             -csv->longest, bn(p->name), p->size, p->description);
      }
    }
    if (f)
    {
      fprintf (f, "\n%d files missing (%d k)\n", missing, kb(missing_b));
      fclose (f);
      f = NULL;
    }  
    lprintf (2, "%d files missing\n", missing);
  }

  if (trade_give)
  {
    int * randfeld;
    char *source, target[MAXPATH];
    char * target_name;
    int  zipnr = 0, zip_files = 0, zip_len = 0;
    FILE * ziplog = NULL;
    char * ziplogname = "zip.log";
    
  


    if (csv->Ecsv && !trade_zip)
    {
      trade_zip = 5;
      lprintf (2, "Switching to zip mode to handle E-CSV %s\n", csv->name);
    }  

    offered = 0;
    for (i = 0; i < csv->nrpics; i++)
    {
      p = p1 + i;
      if (p->exists_trade == no && p->exists == yes)
        offered++;
    }
    if (!offered)
    {
      lprintf (2, "Nothing to offer - cannot give\n");
      return;
    }
    if (nr_of_collections <= col)
       error ("Yikes - Internal error 1, %d should be < %d", col, nr_of_collections);
    randfeld = zufalls_feld(col, trade_order);

    if (multi && !trade_zip)
       sprintf (target, "%s%s%c", tradepath, csv->name, PC);
    else
       strcpy (target, tradepath);
    target_name = target + strlen(target);
    clean_name (target);
    if (!trade_fake)
      check_make_directory (target);
    
    for (i = 0; i < csv->nrpics; i++)
    {
      p = p1 + randfeld[i];
      if (p->exists_trade == no && p->exists == yes)
        {
           if (trade_fake == 2)
             source = "";
           else  
             source = locate_pic(p);

           if (!source)
           {
             lprintf (2, "cannot find picture %s\n", p->name);
             continue;
           }  
           if (trade_zip == 0)
           {
             strcpy (target_name, p->name);
             if (trade_fake || copy_pic (source, target))
             {
               given++;
               files_given++;
               kbytes_given += (int)(p->size / 1000.0 + 0.5);
             }  
           }
           else
           { 
             if (zip_files >= trade_zip ||
                  (trade_zip >= 500 && zip_files
                    && (zip_len + p->size) / 1024 >= trade_zip))
             {
               if (! FlushZip()) 
      		 error("Close pipe failed: %s\n", system_error());
               lprintf (1, "Pipe to zip closed\n");      		 
               zip_files = 0;
             }
             if (zip_files == 0)
             {
                if (!ziplog)
                  ziplog = my_fopen (ziplogname, AT);
             
                if (trade_give_long)
                   zipbasename = csv->name;
                do {   
		  sprintf (target_name, "%s%02d.zip", zipbasename, ++zipnr);
                  clean_name (target);
                } while (exists_length(target, 0));
         	lprintf (0, "RunZip: %s\n", target);
   		RunZip(target, csv->Ecsv);

   		fprintf (ziplog, "\nCollection %s:\n%s created by %s  %s",
   		         csv->name, target_name, version, zeit);
 
               zip_len = 0;
             }
             zip_files++;
             given++;
             files_given++;
             kbytes_given += (int)(p->size / 1000.0 + 0.5);

             zip_len += p->size;
             strcat (source, "\n");

             WriteZip (source);
             fprintf (ziplog,"%*s  %8d\n", -csv->longest-5, p->name, p->size);
           }
           p->exists_trade = yes;
           if ((trade_give <= 500 && files_given >= trade_give)
               || (trade_give > 500 && kbytes_given >= trade_give))
           {
             trade_give_enough = 1;
             break;
           }
        }  
    }
    if (trade_zip && zip_files)
    {
      if (! FlushZip())
      {
      	 lprintf(2, "Error: Close pipe failed\n");
      	 return;
      }	 
/*
   Problem: Zip laeuft im Hintergrund; Ziperrors sind zu diesem Zeitpunkt noch gar nicht
   gesetzt. Deshalb wird bei Zipfehler TROTZDEM das Reportfile geloescht :-(

      if (ziperrors)
        lprintf (2, "%d Errors during zipping\n", ziperrors);
      else
*/
      lprintf (2, "%d files packed into %d Zips (Zip is still running)\n", given, zipnr);
    }
    else
      lprintf (2, "%d files copied\n", given);
    if (trade_fake)
      return;
    if (!trade_give_long)
    {
      sprintf (pfad, "%sneed.txt", tradepath);
      backup (pfad);
    }  
    else 
      sprintf (pfad, "%sneed_%s.txt", tradepath, csv->name);
    f = NULL;  
    for (i = 0; i < csv->nrpics; i++)
    {
      p = p1 + i;
      if (p->exists_trade == no)
      {
        if (!f)
        {
          f = my_fopen(pfad, WT);
          fprintf (f,  
      		"%s automatically generated trading list\n"
      		"%s"			// cr ist schon im Datum enthalten
      		"Collection: %s\n"
      		"CSV file:   %s\n\n"
      		"My trading partner is missing:\n\n" ,
      		version,
      		zeit, csv->name, csv->filename);
        }
        fprintf (f,"%*s      %8d      %s\n",
          -csv->longest, bn(p->name), p->size, p->description);
        needed ++;
        needed_b += p->size;
      }
    }
    if (f)
    {
      fprintf (f, "\n%d files needed (%d k)\n", needed, kb(needed_b));
      fclose (f);
      f = NULL;
      lprintf (2, "Report %s replaced by %s\n", reportname, pfad); 
    }
    else if (!trade_whole_collections)
    {
      DeleteWaste (pfad);
      lprintf (2, "All files from report %s given, deleted\n", reportname); 
    }
    if (stricmp(pfad, reportname))
       DeleteWaste (reportname);
    if (ziplog)
       fclose (ziplog);
   } 
}

mstring * trade_paths;

void prepare_all_trading (mstring * search_paths)
{
       int w, i;

       if (trade_whole_collections)
         trade_paths = search_paths;
       else
       {
         trade_paths = ms_init (m, 6);
         for (i = 0; i < search_paths->n; i++)
           expand_wildcard (trade_paths, search_paths->s[i], 0);
       }  

       w = trade_paths->n;
       if (w == 0)
         error ("no reports found for trading");
       if (w > 1)
       {
         if (trade_ask) trade_ask = 2;
         if (trade_offer) trade_offer = 2;
         trade_give_long = 1;
       }  
}

void do_all_trading (void)
{
       int i;

       InitZip();
       
       for (i = 0; i < trade_paths->n  &&  !trade_give_enough; i++)
       {
         lprintf (2, "Do trading: %s\n", trade_paths->s[i]);
         do_trading (trade_paths->s[i], trade_paths->n > 1);
       }  
       for (i = ZIP_MULTI; i--; )
         WaitZip(i);
       if (files_given)
       {
          if (trade_zip)
             lprintf (2, "Zip finished\n");
          lprintf (2, "%d pics given (%d k)\n", files_given, kbytes_given);
       }   
}

