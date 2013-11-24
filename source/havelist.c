/* Scansort 1.81 - havelists
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
#include <ctype.h>	// for isspace

extern char * no_description;
// Switches
extern int makehavelist_binary, report_makehavelist_s, verbose, convert_havelists_to_bin;


char * binary_have_identifier = "ScanSort binary havelist\n\n\n\n\n\n\n";

/*
Format for havelists:
old:
 123456 32de356b SomeChick.jpg Comment
new: (CRC may be 0)
SomeChick.jpg,   123456,    32de356b,  Comment
binary:
size and crc behind each other. if crc == 0 the name follows (for non-CRC collections)
*/

// Write a binary integer in Little Endian format to file f
static void wbi (int i, FILE * f)
{
  char c[4];
  char * c1 = (char *) & i;
  int k;
  
  if (BigEndian)
     for (k=4; k--;)
       c[k] = c1[3-k];
  else
     *(int *) c = i;
  fwrite (c, 4, 1, f);  
}

void make_havelist(int mode)
{
  int i,j,banner;
  FILE * f;
  char * havename = makehavelist_binary ? "have.bin" : "have.txt";
  pic_desc * p;
  collection * csv;
  char buf[MAXPATH];

  lprintf (2, "Writing list of files you have to %s ...\n", havename);
  f = my_fopen (havename, makehavelist_binary ? WB : WT);

  if (makehavelist_binary)
     fwrite (binary_have_identifier, strlen(binary_have_identifier)+1, 1, f); 
  else if (! report_makehavelist_s)
    fprintf (f, "# %s automatically generated list of files in collection\n# %s\n",
    		version, zeit);

  for (i = 0; i < nr_of_collections; i++)
  {
    banner = 0;
    csv = collections[i];
    for (j = 0; j < csv->nrpics; j++)
    {
      p = csv->pic_descs + j;
      if (p->exists == yes)
      {
        if (makehavelist_binary)
        {
          wbi (p->size, f);
             wbi (p->crc, f);
          if (!p->crc)
             fwrite (bn(p->name), strlen(bn(p->name))+1, 1, f);
        }
        else
        {
          if (strchr(p->name, ','))
             sprintf (buf, "\"%s\"", bn(p->name));
          else   
             strcpy (buf, bn(p->name));
          if (report_makehavelist_s)
          {
            fprintf (f, "%s,%d,%08x,%s",
                buf, p->size, p->crc, csv->name);
          }
          else
          {
            if (!banner)
            {
              fprintf (f, "\n# %s\n", csv->name);
              banner = 1;
            }
            fprintf (f, "%*s, %7d, %08x",
                -csv->longest, buf, p->size, p->crc);
          }      
          if (mode == 2 && strcmp(p->description, no_description))
          {
            fprintf (f, "%s", report_makehavelist_s ? "," : ", ");
            if (strchr (p->description, ','))
              fprintf (f, "\"%s\"\n", p->description);
            else  
              fprintf (f, "%s\n", p->description);
          }      
          else
              fprintf (f, "\n");
        }      
      }
    }
  }
  if (makehavelist_binary)
  {
     wbi (0, f);
     wbi (1, f);
  }   
  fclose (f);
}

int read_havelist (char * name, int nr)
{
  char * c, *pname = NULL;
  int size, line = 0;
  unsigned int crc = 0;
  int found = 0, found_new = 0, binary = 0;
  collection * csv = NULL;
  pic_desc * p, **pp;
  int lastcsv = -1;
  FILE * f = NULL;
  char s[MAXPATH];
  
  if (!readbuffer (name))
  {
      lprintf (2, "Cannot read havelist %s\n", name);
      return 0;
  }
  c = picbuffer;
  if (!strcmp(picbuffer, binary_have_identifier))
  {
    c += strlen(binary_have_identifier) + 1;
    binary = 1;
  }
  else if (convert_havelists_to_bin)
  {
    sprintf (s, "%s.bin", bn(name));
    lprintf (2, "Convert text havelist %s to binary havelist %s\n", name, s);
    f = my_fopen(s, WB);
    fwrite (binary_have_identifier, strlen(binary_have_identifier)+1, 1, f); 
  }
  do
  {
   if (binary)
   {
      int l[2], i;
      char * cc = (char *) l;

      if (BigEndian)
      {
        for (i = 8; i--;)
          cc[i] = *c++;
        size = l[1];
        crc  = l[0];
      }
      else
      {
        for (i = 8; i--;)
          *cc++ = *c++;
        size = l[0];
        crc  = l[1];
      }  
//      lprintf (0, "Size %6d  CRC %8X\n", size, crc);
      if (!crc)
      {
        pname = c;
        c += strlen(pname) + 1;
      }
   }
   else
   {
    char *cc;
    int toks;
    char * tokens[3];
    int  tokenlen[3];
    char * sizestr = NULL, * crcstr = NULL;

    toks = linetokens (c, &cc, 3, tokens, tokenlen, 1, 0);
    if (toks == 3)	// new format: name , 123456 , abcd1234 
    {
      sizestr = tokens[1];
      crcstr  = tokens[2];
    }
    else
    { // old format: 123456  abcd1234  Name Comment
      toks = linetokens (c, &cc, 2, tokens, tokenlen, 0, 0);
      if (toks == 2)
      {
        sizestr = tokens[0];
        crcstr  = tokens[1];
      }
    }
    line++;    
    pname = c;
    c = cc;

    if (toks == 0)		// EOF reached
       size = 0;	
    else if (tokens[0][0] == '#')	// comment
       size = 1;
    else if (sizestr && (size = strtoul(sizestr,NULL,10)))
    {
        if (! (crc = strtoul(crcstr,NULL,16)))
        {
           m_strncpy (s, tokens[0], tokenlen[0]);
           pname = s;
        }   
    }
    else
    {
        lprintf (2, "no filesize in %s line %d\n", name, line);
        size = 1;	// otherwise it will exit
    }
   }

   if (size > 1)
   {
    if (f)
    {
          wbi (size, f);
          wbi (crc, f);
          if (!crc)
             fwrite (bn(pname), strlen(bn(pname))+1, 1, f);
    }    
    for (pp = findpic_size_h(size); pp && *pp; pp++)
    {
      p = *pp;
      if (p->size != size) continue;
      if ((crc && p->crc == crc) || (!crc && !m_stricmp(pname, bn(p->name))) )
      {
        if (p->collection != lastcsv)
        {
          lastcsv = p->collection;
          csv = collections[lastcsv];
          csv->empty = 0;
//          bf_set (csv->haveindex, nr);   // here it would mark every havefile
        }
        if (verbose)
           lprintf (0, "%s %s %s\n", p->exists == yes ? "(Have)" : "Have  ",
           	csv->name, p->name);
        found++;
        if (p->exists != yes)
        {
           found_new ++;
           bf_set (csv->haveindex, nr);	   // here it marks only havefiles with previously 
					   // missing pics
        }   
        p->exists = yes;
      }
    }
   } 
  } while (size);
  if (f)
  {
     wbi (0, f);
     wbi (1, f);
     fclose (f);
  }     
  lprintf(2, "found %d   new %d\n", found, found_new);
  return found_new;
}

