/* Scansort 1.81 - split Gully Foyle's Cyberclub CSV
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

extern int fsplit;			// Split Gully Foyle's Cyberclub CSV
extern int fsplit_decades;		//   by decades
extern int fsplit_years;		//   by years
extern int fsplit_categories;		//   by categories
extern int fsplit_ecsv;			//   create a E-CSV
extern int fsplit_original;		//   create original L-Port CSVs
extern int fsplit_autoname;		// automatically find CSV to split

extern char foyle_csv_name[];

extern char * no_description;

typedef struct
{
  int nr;
  int start;
} portfolio;

typedef portfolio portfolios[100][13];	// year 0-99, month 0-12 (0 is Index_Data) 

void get_portfolios (collection * csv, portfolios pf)
{
  int i,j,k, y,m;
  int ly = -1, lm = -1;
  int lc = 0;
  pic_desc * p;
  
  for (i=100; i--;)
    for (j=13; j--;)
      pf[i][j].nr = 0;
  for (i = 0; i < csv->nrpics; i++)
  {
     p = csv->pic_descs + i;
     y = 0;
     for (j = 0; j < 4; j++)
     {
       k = p->name[j] - '0';
       if (k < 0 || k > 9)
          error ("%s is no valid name for a Cyberclub CSV", p->name);
       y = 10 * y + k;
     }
     m = y % 100;
     if (m > 12)
          error ("%s is no valid name for a Cyberclub CSV", p->name);     
     y /= 100;
     if (y == ly && m == lm)
        lc ++;
     if ( !(y == ly && m == lm) || i == csv->nrpics - 1)
     {
       if (ly >= 0)  // NOT in the very first step !!!
       {
         if (pf[ly][lm].nr)
            error ("CSV must be sorted for splitting !");
         pf[ly][lm].nr = lc;
       }  
       lc = 1;
       ly = y;
       lm = m;
       if (i < csv->nrpics - 1)
          pf[ly][lm].start = i;
     }     
  }
}

void write_fsplit_csv (char * name, mstring *ms)
{
  char s[MAXPATH];
  FILE * f;
  int i;
  
  if (!ms->n)
     return;
  sprintf (s, "%s_%d.csv", name, ms->n);
  f = my_fopen (s, WT);
  lprintf (2, "Write %s\n", s);
  for (i = 0; i < ms->n; i++)
    fprintf (f, "%s", ms->s[i]);
  fclose (f);  
}

int compare_text_line (char ** s1, char ** s2)
{
  return m_stricmp (*s1, *s2);
}

void do_fsplit (char * base_csv)
{
  collection * csv1, * csv2 = NULL;
  collection * csvs[2];
  pic_desc * p, * p2;
  int y,mo,i;
  portfolios pf1, pf2;
  mstring *msdec[10], *msy[100], *mslp, *mslc, *mslh, *mscf, *msda;
  char s[MAXPATH], *bname;

  if (!fsplit_decades && !fsplit_years && !fsplit_categories 
      && !fsplit_ecsv && !fsplit_original && !base_csv)	
     help_fsplit();
     
  for (i=10;i--;)
      msdec[i] = ms_init(m, 5);
  for (i=100;i--;)
      msy[i] = ms_init(m, 5);
  mslp = ms_init(m, 5);
  mslc = ms_init(m, 5);
  mslh = ms_init(m, 5);
  mscf = ms_init(m, 5);
  msda = ms_init(m, 5);
  
  if (fsplit_autoname)
  {
    int l = 0, i, k;
    mstring * ms = ms_init(m, 5);
    char mask[] = "Foyle_PCC_xxxx_*.csv";
    int n = expand_wildcard (ms, mask, 0); 
    if (!n)
       error ("Can't find %s in current directory", mask);
    for (i = 0; i < ms->n; i++)
    {
      sscanf (ms->s[i] + 15, "%d", &k);
      if (k > l)
      {
        l = k;
        strcpy (foyle_csv_name, ms->s[i]);
      }
    }
    if (!l)
       strcpy (foyle_csv_name, ms->s[0]);	// shouldn't happen
  }
  no_description = "";	// so we don't get ?s for empty comments
  collections = csvs;
  lprintf (2, "Splitting %s ", foyle_csv_name);
  csv1 = mm_alloc (m, sizeof (collection));
  csvs[0] = csv1;
  if (!readbuffer (foyle_csv_name))
     error ("Can't open %s", foyle_csv_name);
  csv1->pic_descs = mm_alloc (m, countlines (picbuffer) * sizeof (pic_desc));
  csv1->nr = 0;
  parse_csv (0, picbuffer);
  lprintf (2, "(%d pics)\n", csv1->nrpics);

  get_portfolios (csv1, pf1);
  if (base_csv)
  {
    lprintf (2, "using Base CSV %s", base_csv);
    csv2 = mm_alloc (m, sizeof (collection));
    csvs[1] = csv2;
    if (!readbuffer (base_csv))
       error ("Can't open %s", base_csv);
    csv2->pic_descs = mm_alloc (m, countlines (picbuffer) * sizeof (pic_desc));
    csv2->nr = 1;
    parse_csv (1, picbuffer);
    lprintf (2, "(%d pics)\n", csv2->nrpics);
    get_portfolios (csv2, pf2);
    lprintf (2, "Updated Portfolios:\n");
    for (y=53; y<=152; y++)
      for (mo=0; mo<=12; mo++)
      {
        int y1 = y % 100;
        int changed = pf1[y1][mo].nr != pf2[y1][mo].nr;
        for (i = 0; !changed && i < pf1[y1][mo].nr; i++)
        {
          p  = csv1->pic_descs + pf1[y1][mo].start + i;
          p2 = csv2->pic_descs + pf2[y1][mo].start + i;
          if (p->size != p2->size || p->crc != p2->crc)
             changed = 1;
        }
        if (changed)
           lprintf (2, " %02d/%d", mo, 1900 + y);
        else
           pf1[y1][mo].nr = 0;
      }
    lprintf (2, "\n");
  }

  for (y=0; y<=99; y++)
    for (mo=0; mo<=12; mo++)
      for (i = 0; i < pf1[y][mo].nr; i++)
      {
        p = csv1->pic_descs + pf1[y][mo].start + i;
        bname = bn(p->name);
        if (fsplit_ecsv)
        {
          int dec = (y >= 50 ? 1900 : 2000) + (y / 10) * 10;
          int year = y % 10;
          if (fsplit_decades)
             sprintf (s, "\\%ds\\,", dec);
          else if (fsplit_years)  
             sprintf (s, "\\%d\\,", dec + year);
          else 
             sprintf (s, "\\%ds\\%d\\,", dec, dec + year);
          p->description = mm_strdup(m, s);   
        }
        sprintf (s,"%s,%d,%08X,%s\n", bname, p->size, p->crc, p->description);

        if (fsplit_ecsv)
          ms_push (mslp, s);
        else if (fsplit_categories || fsplit_original)
        {
          if (is_tail("_data", bname))
             ms_push (msda, s);
          else if (is_tail("_cf", bname))
             ms_push (mscf, s);   
          else if (is_tail("_cover", bname))
             ms_push (mslc, s);   
          else if (is_tail("_00", bname))
             ms_push (mslh, s);
          else {
            int d;
            char * c;
            if (!(c = strrchr (bname, '_')) || !sscanf (c+1, "%d", &d) || d < 1)
               error ("illegal name in CSV: %s", bname);
            if (fsplit_original)
               sprintf (s,"l-port-%02d-%02d-%02d,%d,%08X,%s\n",
                       mo, y, d, p->size, p->crc, p->description);
            if (fsplit_decades)
               ms_push (msdec[y/10], s);
            else   
               ms_push (mslp, s);
          }
        }
        else if (fsplit_decades)
                ms_push (msdec[y/10], s);
        else if (fsplit_years)
                ms_push (msy[y], s);
        else    ms_push (mslp, s);
      }
  if (fsplit_ecsv)
      write_fsplit_csv ("Foyle_PCC_xxxE", mslp);
  else if (fsplit_original)
  {
    if (fsplit_decades)
      for (i = 5; i <= 14; i++)
      {
        mstring * ms = msdec[i%10];
        sprintf (s, "Foyle_L-Port_Original_%ds", 1900 + 10 * i);
        qsort (ms->s, ms->n, sizeof (char *), (QST) compare_text_line);
        write_fsplit_csv (s, ms);
      }
    else
    {
      qsort (mslp->s, mslp->n, sizeof (char *), (QST) compare_text_line);
      write_fsplit_csv ("Foyle_L-Port_Original", mslp);
    }  
  }
  else if (fsplit_categories)
  {
    write_fsplit_csv ("Foyle_L-Head", mslh);
    write_fsplit_csv ("Foyle_L-Cover", mslc);
    write_fsplit_csv ("Foyle_BigCenterfold", mscf);
    write_fsplit_csv ("Foyle_Chippy_Data_Renamed", msda);
    if (fsplit_decades)
      for (i = 5; i <= 14; i++)
      {
        sprintf (s, "Foyle_L-Port_Renamed_%ds", 1900 + 10 * i);
        write_fsplit_csv (s, msdec[i%10]);
      }
    else
      write_fsplit_csv ("Foyle_L-Port_Renamed", mslp);
  }
  else if (fsplit_decades)
      for (i = 5; i <= 14; i++)
      {
        sprintf (s, "Foyle_PCC_%ds", 1900 + 10 * i);
        write_fsplit_csv (s, msdec[i%10]);
      }
  else if (fsplit_years)
      for (i = 53; i <= 152; i++)
      {
        sprintf (s, "Foyle_PCC_%ds", 1900 + i);
        write_fsplit_csv (s, msy[i%100]);
      }
  else
      write_fsplit_csv ("Foyle_PCC_xxdiff", mslp);
}

