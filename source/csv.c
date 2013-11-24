/* Scansort 1.81 - process config file and CSVs
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

#include <ctype.h>	// for isspace
#include <math.h>	// for fmod
#include "scansort.h"

// maximum length for a filename
#define MAX_FILENAME  100
// maximum length for a E-CSV path
#define MAX_ECSV_PATH 150


extern mstring *endungen, *have_lists;
extern int collection_longest, collection_maxpics;
extern char targetpath[], reportpath[], badpath[];
extern mstring *prefixes, *prefixes_remove, *csv_paths, * single_collection;

// switches
extern int no_underscores, rename_csvs;
extern int to_upper, to_lower, verbose, kill_duplicate_csvs, make_csv;
extern int debug;
extern char * report_identity;
extern int auto_index_dir, log_unused_csvs;
extern int csvpath_used, csvpath_complete;
char * no_description = "?";
char * no_report = "none";
static int  no_samepaths = -1;

// Hashtable Variables
pic_desc ** hashnames, ** hashsizes;
double hashfact = 1.5;		// size of hash tables
int hashextra = 1000;		// extra enties at the end
int hashnames_maxskip = 0;	// maximum skip
int hashnames_avgskip = 0;	// average skip
int hashnames_len;
int hashsizes_maxskip = 0;	// maximum skip
int hashsizes_avgskip = 0;	// average skip
int hashsizes_len;

// Hash-Functions

unsigned int hash_size (unsigned int size, unsigned int tablelen)
{
  static double h=376437.3473946397438347;
  return (unsigned int) fmod(size * h, tablelen);
}

unsigned int hash_name (char * name, unsigned int tablelen)
{
  char s[MAXPATH];

  strcpy (s, name);
  make_search_string (s);
  return crcblock(s, strlen(s)) % tablelen;
}

//set *line to next cr/lf
static void gotoendofline (char ** line)
{
  while (!iscr(**line) && **line)
    (*line) ++;
}

// set *line to next line in buffer, return 0 if last line
// buffer must end with <cr> <0> !
static int getnewline (char ** line)
{
  char *c = *line;

  while (1)
  {
    while (isspace((int)*c))
      c++;
    if (*c == '#' || *c == ';')  // comment line
      gotoendofline (&c);
    else
      break;
  }
  *line = c;

  if (*c == 0)
    return 0;

  return 1;
}

// Kommentare NICHT ueberlesen
static int getnewline1 (char ** line)
{
  char *c = *line;

  while (isspace((int)*c))
      c++;
  *line = c;

  if (*c == 0)
    return 0;

  return 1;
}

int countlines (char * buf)
{
  char * b = buf;
  int i;

  for (i = 0; getnewline1(&b); i++)
    gotoendofline(&b);
    
  return i;
}

// Split a line into tokens, data is NOT changed
// Return: number of Tokens (0: EOF)

int linetokens (char * line,		// line to scan
                char ** newline,	// pointer to next line will be returned here
                int maxtokens,		// how many tokens max.
                char ** tokens,		// pointer to tokens
                int  * tokenlen,	// lengths of tokens
                int  nurkomma,		// 0: Komma or WS, 1: only Komma
                int  last_is_comment)   // 1: last entry is comment (can include Kommas)
{
  int toks = 0;
  int eol = 0;
  int quote = 0;
  char * c;
  int l, l1;

  if (getnewline1 (&line) == 0)		// skip spaces
       return 0;
  
  c = line;
  while (1)
  {
    if (*c == '"')
    {
      c++;
      quote = 1;
    }
    else
      quote = 0;
    tokens[toks] = c;
    for (l = 0; !( (eol = iscr(c[l])) ||
                      (!(last_is_comment && toks == maxtokens - 1) &&
                        (!quote && (c[l] == ',' || (!nurkomma && isspace ((int)(c[l])) )))
                      )
                 ); l++)
                 if (c[l] == '"')
                    quote = !quote;
       
    for (l1 = l; l1 > 0 && (isspace ((int)(c[l1 - 1])) || c[l1 - 1] == '"'); l1--);   // remove trailing spaces and quotes
    tokenlen[toks++] = l1;
    for (c = c + l + 1; isspace ((int)*c); c++)
      eol = eol || iscr(*c);
    if (toks == maxtokens || eol)
       break;
  }
  if (!eol)
     gotoendofline (&c);
  *newline = c;
/*
  for (l = 0; l <toks; l++)
  {
      lprintf (0, "%d >", l);
      for (l1 = 0; l1 < tokenlen[l]; l1++)
        lprintf (0, "%c", tokens[l][l1]);
      lprintf (0, "<  ");
  }
  lprintf (0, "\n");
*/  
  return toks;
}

static int compare_picdesc1 (const pic_desc ** p1, const pic_desc ** p2)
{
  int i = m_stricmp((**p1).name, (**p2).name);
  if (i)
     return i;
  return m_stricmp((**p1).description, (**p2).description);
}

static int compare_picdesc1a (const pic_desc * p1, const pic_desc * p2)
{
  int i = m_stricmp((*p1).name, (*p2).name);
  if (i)
     return i;
  return m_stricmp((*p1).description, (*p2).description);
}

static int compare_csv1 (const collection ** p1, const collection ** p2)
{
  return m_stricmp((**p1).name, (**p2).name);
}

static int compare_csv2 (const collection ** p1, const collection ** p2)
{
  int i = stricmp((**p1).targetdir, (**p2).targetdir);
  if (i)
     return i;
  if ((**p1).nr > (**p2).nr)
     return 1;
  if ((**p1).nr < (**p2).nr)
     return -1;
  else
     return 0;
}


int parse_csv (int nr, char * buffer)
{
  pic_desc * p;
  collection * csv = collections[nr];
  char *c1, *c2;
  char *lastdesc = "";
  int  i, j = 0, line = 0, nocrc = 0;
  char tmpname[MAXPATH];
  static char empty_ecsvpath[2] = "\\";  // platform independent
  char * tokens[4];
  int tokenlen[4];
  int toks;
  int crc_exists;

  csv->longest = 0;

    while ((toks = linetokens (buffer, &buffer, 4, tokens, tokenlen, 1, 1)))
    {
      line ++;
      if (toks < 2)
         continue;
      p = csv->pic_descs + j;
      p->collection = nr;
      p->exists = no;
      p->exists_trade = unknown;

// Process the name
      if (tokenlen[0] > MAX_FILENAME)	// shouldn't happen
          tokenlen[0] = MAX_FILENAME;
      m_strncpy (tmpname, tokens[0], tokenlen[0]);
      if (!no_underscores)
        for (c1 = tmpname; *c1; c1++)
           *c1 = normalchar(*c1, 1);

      c1 = checkendung3 (tmpname);
      if (c1)
      {
        int i;
        for (i = 0; i < endungen->n; i++)
          if (!stricmp (c1, endungen->s[i]))
             break;
        if (i)
           csv->specialtypes = 1;
        if (i == endungen->n)
           str2lower (ms_push(endungen, c1));
      }
      else
      {
        c1 = tmpname + strlen(tmpname);
        *c1 = '.';
        strcpy (c1+1, endung);
      }
      if (to_upper)
         name2upper (tmpname);
      else if (to_lower)
         name2lower (tmpname, to_lower != 1);
      p->name = mm_strdup(m, tmpname);

      if (strlen(p->name) > (unsigned) csv->longest)
        csv->longest = strlen(p->name);
        
// Process the size
      if ((p->size = strtoul(tokens[1],NULL,10)) == 0)
      {
        lprintf (1, "no filesize in %s line %d - entry ignored\n", csv->filename, line);
        continue;
      }
      csv->nrbytes += p->size;

// Is the following a CRC ?
      if (toks < 3)
         crc_exists = 0;
      else
      {
        for (i = 0; isxdigit ((int) tokens[2][i]); i++);
        crc_exists = (i == tokenlen[2] && i <= 8);
//        lprintf (0, "%6d   %d %d %d\n", p->size, crc_exists
      }   
      p->crc = crc_exists ? strtoul(tokens[2],NULL,16) : 0;
      if (p->crc == 0xAAAAAAAA)		// this is a dummy, not a real CRC !
         p->crc = 0;
      if (p->crc == 0)
      {
         if (verbose && nocrc < 3)
            lprintf (0, "No CRC: %s %d\n", p->name, p->size);
         nocrc++;
      }

// Description
      if ( !(toks == 4 || (toks == 3 && !crc_exists)))
         p->description = no_description;
      else
      {
         i = crc_exists ? 3 : 2;
         if (i == 2 && toks == 4)	// no CRC and comment with comma
            tokenlen[2] = tokens[3] - tokens[2] + tokenlen[3];
         if (tokenlen[i] > MAX_ECSV_PATH)		// chop too long comments
            tokenlen[i] = MAX_ECSV_PATH;
         m_strncpy (tmpname, tokens[i], tokenlen[i]);
         if ( tmpname[0] == WINPC)
         {
           // E-CSV. We keep the path in DOS format and convert it to Unix when needed.
           csv->Ecsv = 1;
           // Cut off comments, allow comma in the path (yuck !)
           c1 = tmpname;
           while ((c1 = strchr(c1, ',')))
             if (c1[-1] == WINPC)	   // found a \, 
             {
                *c1 = 0;	// behaviour changed for 1.8
                break;
             }
             else
                c1++;
           c1 = tmpname;
           c2 = c1+strlen(c1)-1;
           while (isspace((int)*c2))
             *c2-- = 0;
           if (*c2 != WINPC)
           {
             *(++c2) = WINPC;
             *(++c2) = 0;
           }
           // remove stupid spaces and special chars from path
           while (c2 >= c1) 
           {
             if (!no_underscores)
                *c2 = normalchar(*c2, 1);
             c2 --;
           }
           // Unix: always convert / to _
           while (PC != WINPC && (c1 = strchr(c1, PC)))
             *c1 = '_';
           if (to_lower == 1)
              str2lower (tmpname);
         }
         if (!stricmp (tmpname, "unknown") || !stricmp (tmpname, "---") )
           p->description = no_description;
         else if (strcmp(tmpname, lastdesc))
         {
           p->description = mm_strdup (m, tmpname);
           lastdesc = p->description;
         }
         else
           p->description = lastdesc;
      }
      j++;
    }  // loop over CSV file
    csv->nrpics = j;
    csv->descriptions = 0;
    for (j = csv->nrpics; j--;)
    {
      p = csv->pic_descs + j;
      // Check if all entries are in E-CSV format
      if (csv->Ecsv)
      {
        if (p->description[0] != WINPC)
           p->description = empty_ecsvpath;
      }
      else
        if (stricmp(p->description, no_description))
           csv->descriptions ++;
    }
    csv->longest -= 4;
    return nocrc;
}

void make_index(int v)
{
  int i, j, *ip;
  collection *csv, **cp;
  pic_desc *p, *pf, **ppf;
  minimem_handle * m1;
  char * lastpath;
  
  lprintf (2, "Sorting images...\n");
// sort Collection-Indices alphabetically
// first check if already sorted. Then make index array to speed things up
  m1  = mm_init  (0x4000, 0x400);
  pf  = mm_alloc (m1, collection_maxpics * sizeof (*pf));
  ppf = mm_alloc (m1, collection_maxpics * sizeof (*ppf));

  for (i = 0; i < nr_of_collections; i++)
  {
    int sorted;
    csv = collections[i];
    p = csv->pic_descs;
    sorted = 1;
    for (j = 1; j < csv->nrpics; j++)
      if (compare_picdesc1a (&p[j-1], &p[j]) > 0)
      {
        sorted = 0;
        break;
      }
    if (sorted)
       continue;
    memcpy (pf, csv->pic_descs, csv->nrpics * sizeof (pic_desc));
    for (j = csv->nrpics; j--;)
      ppf[j] = pf + j;
    qsort (ppf, csv->nrpics, sizeof (pic_desc*), (QST) compare_picdesc1);
    for (j = csv->nrpics; j--;)
      csv->pic_descs[j] = *ppf[j];
  }  

// Now build global index

// Hashtable for size
  lprintf (2, "Hashtable for size...\n");
  hashsizes_len = (int)(pics * hashfact);
  hashsizes = mymalloc((hashsizes_len + hashextra) * sizeof (pic_desc *));
  for (i = hashsizes_len + hashextra; i--;)
    hashsizes[i] = NULL;
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    for (j = 0; j < csv->nrpics; j++)
    {
      int hash, h;
      p = csv->pic_descs + j;
      hash = hash_size(p->size, hashsizes_len);
      for (h = 0; hashsizes[hash + h]; h++)
        if (h == hashextra - 2)
        {
          int l;
          hashsizes = myrealloc(hashsizes, (hashsizes_len + 2*hashextra) * sizeof (pic_desc *));
          for (l = hashsizes_len + hashextra; l < hashsizes_len + 2*hashextra; l++)
            hashsizes[l] = NULL;
          hashextra *= 2;  
        }
      hashsizes[hash + h] = p;
//	 lprintf (0, "%6d %3d %7d %s\n", hash, h, p->size, p->name);      
      if (h > hashsizes_maxskip)
         hashsizes_maxskip = h;
      hashsizes_avgskip += h;   
    }
  }
  if (debug)
  {
    int i,j;
    double k = 0;
    lprintf (2, "Hashsizeskip: max %d  avg %.2f\n", hashsizes_maxskip, hashsizes_avgskip / (double)pics);
    for (i=0;i<hashsizes_len;i++)
    {
      for (j=0; hashsizes[i+j]; j++);
        k += j;
    }
    lprintf (2, "Average hash search: %.2f\n", k / hashsizes_len);
  }

// Hashtable for name
  lprintf (2, "Hashtable for name...\n");
  hashnames_len = (int)(pics * hashfact);
  hashnames = mymalloc((hashnames_len + hashextra) * sizeof (pic_desc *));
  for (i = hashnames_len + hashextra; i--;)
    hashnames[i] = NULL;
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    for (j = 0; j < csv->nrpics; j++)
    {
      int hash, h;
      p = csv->pic_descs + j;
      hash = hash_name(p->name, hashnames_len);
      for (h = 0; hashnames[hash + h]; h++)
        if (h == hashextra - 2)
        {
          int l;
          hashnames = myrealloc(hashnames, (hashnames_len + 2*hashextra) * sizeof (pic_desc *));
          for (l = hashnames_len + hashextra; l < hashnames_len + 2*hashextra; l++)
            hashnames[l] = NULL;
          hashextra *= 2;  
        }
      hashnames[hash + h] = p;
	// lprintf (0, "%6d %3d %s\n", hash, h, p->name);      
      if (h > hashnames_maxskip)
         hashnames_maxskip = h;
      hashnames_avgskip += h;   
    }
  }

  if (debug)
  {
    int i,j;
    double k = 0;
    lprintf (2, "Hashnameskip: max %d  avg %.2f\n", hashnames_maxskip, hashnames_avgskip / (double)pics);
    for (i=0;i<hashnames_len;i++)
    {
      for (j=0; hashnames[i+j]; j++);
        k += j;
    }
    lprintf (2, "Average hash search: %.2f\n", k / hashnames_len);
  }

  lprintf (2, "Index done\n");

// Check if collections go to same path

  cp = mm_memdup (m1, collections, nr_of_collections * sizeof (collection *));
  ip = mm_alloc (m1, (nr_of_collections + 1) * sizeof (int)); 
  qsort (cp, nr_of_collections, sizeof (collection*), (QST) compare_csv2);
  lastpath = "";
  for (i = 0; i < nr_of_collections; i++)
  {
    for (j = 1; j+i < nr_of_collections &&
            !stricmp (cp[i]->targetdir, cp[i+j]->targetdir); j++)
      ip[j] = cp[i+j]->nr;     
    if (j > 1)
    {
      int k, *ip1;
      ip[0] = cp[i]->nr;
      ip[j] = -1;
      ip1 = mm_memdup (m, ip, (j+1) * sizeof (int));
      lprintf (0, "Same target path %s for %d collections:\n", cp[i]->targetdir, j);
      for (k = 0; k < j; k++)
      {
        cp[i+k]->samepaths = ip1;
        lprintf (0, "   %s\n", cp[i+k]->name);
      }  
      i += j-1;  
    }
    else
      cp[i]->samepaths = &no_samepaths;
  }

// check if identically named pics go to the same path
/*   this was disabled when switching to hashtables - maybe later ...
    spaeter, sehr muehsam mit hashing :-(  Wenn man von jedem einzelnen aus untersucht
    werden manche doppelt ausgegeben
    Loesung: Hashtables an Begrenzungsstellen (freie Eintraege) aufspalten, und diese
    Untertabellen sortieren...
    
 if (v >= 0)
 {
  char *lastname, *lastpath;
  int  same, lastsize;
  unsigned int lastcrc;

  p = mm_alloc (m, sizeof *p);
  pic_names[pics] = pic_descs[pics] = p;
  p->name = ".";
  p->size = -1;
  p->collection = 0;
  lastname = lastpath = "";
  same = 0;
  for (i = 0; i <= pics; i++)
  {
    p = pic_names[i];
//    lprintf (0, "%3d  %s\n", p->collection, p->name);
    if (!m_stricmp(p->name, lastname)
        && !m_stricmp(collections[p->collection]->targetdir, lastpath))
      same ++;
    else
    {
      if (same)
      {
        lprintf (v, "Same target path %s for %d pics named %s ( ",
          lastpath, same+1, bn(lastname));
        for (j = -same - 1; j < 0; j++)
          lprintf (v, "%s ", collections[pic_names[i+j]->collection]->name);
        lprintf (v, ")\n");
      }  
      lastname = p->name;
      lastpath = collections[p->collection]->targetdir;
      same = 0;
    }
  }

// Check for identical pics - same problem

  lastsize = lastcrc = 0;
  same = 0;
  for (i = 0; i <= pics; i++)
  {
    p = pic_descs[i];
    if (p->size == lastsize && p->crc && p->crc == lastcrc)
      same ++;
    else
    {
      if (same)
      {
        lprintf (v, "Same picture %6d %08x in %d collections ( ",
          lastsize, lastcrc, same+1);
        for (j = -same - 1; j < 0; j++)
          lprintf (v, "%s %s   ", collections[pic_descs[i+j]->collection]->name,
          		bn(pic_descs[i+j]->name));
        lprintf (v, ")\n");
      }  
      lastsize = p->size;
      lastcrc  = p->crc;
      same = 0;
    }
  }
 }
*/
  mm_free (m1);
  lprintf (2, "Sorting images finished\n");
}

/*  this was removed in 1.7 with the format change
void ignore_collection (collection ** csvs, char * name, int kill_it)
{
  int i, j;
  collection *csv;
  int multi = 0;

  i = strlen(name);
  if (i < 2)    // damit es nicht crasht
     return;
  kill_it = kill_it && kill_duplicate_csvs;   
  if (name[i-1] == '*')
  {
    multi = 1;
    name[i-1] = 0;
  }
  lprintf (1, "Request to %s Collection%s %s\n",
    kill_it ? "delete" : "ignore", multi ? "s" : "", name);
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = csvs[i];
    if (multi ? is_head(name, csv->name) : !stricmp (name, csv->name))
    {
      lprintf (kill_it ? 2 : 1, "%s Collection %s\n",
        kill_it ? "Delete" : "Ignore", csv->filename);
      for (j = i; j < nr_of_collections - 1; j++)
        csvs[j] = csvs[j+1];
      i--;
      nr_of_collections--;
      if (kill_it)
         DeleteWaste (csv->fullfilename);
    }
  }
}
*/

typedef struct csv_desc_ {
    char * name;		
    char * filename;
    char * fullfilename;
    char * signame;
    p_FILETIME filetime;
    int length;
    int nrpics;
    char * used;
    struct csv_desc_ * obsolete;
} csv_desc;

static int compare_csv_desc (const csv_desc * p1, const csv_desc * p2)
{
  int i;
  i = strcmp((*p1).signame, (*p2).signame);
  if (i) return i;

  if ((*p1).nrpics > (*p2).nrpics)
    return -1;
  if ((*p1).nrpics < (*p2).nrpics)
    return 1;

  if ((*p1).length > (*p2).length)
    return -1;
  if ((*p1).length < (*p2).length)
    return 1;
  else
    return 0;
}

void handle_duplicate_csv (csv_desc * cd, csv_desc * cd1)
{
    char buf[MAXPATH];

      if (cd->length > cd1->length)
      {     
        lprintf (2, "Warning: using %s which is smaller (%d) than %s (%d)\n",
           cd1->filename, cd1->length, cd->filename, cd->length);
        if (kill_duplicate_csvs)
        {
          strcpy (buf, cd->fullfilename);
          strcpy (name_from_path(buf), "old_comment");
          check_make_directory (buf);
          sprintf (buf + strlen(buf), "%c%s", PC, cd->filename);
          p_MoveFile (cd->fullfilename, buf);
        }  
      }
      else
      {
        lprintf (2, "%s duplicate CSV %s\n",
           kill_duplicate_csvs ? "Deleting" : "Ignore", cd->filename);
        if (kill_duplicate_csvs)
	  DeleteWaste (cd->fullfilename);
      }	
}

void readconfigfile (char *name)
{
  int i, j;
  int min, max, found, cmp, single_col;
  char *cfgfile, *cfgfile0;
  char *c_reportfile, *lastsigname;
  collection *csv;
  char tmpname[MAXPATH], c_targetdir[MAXPATH];
  collection ** csvs;
  
  int total_csvs;
  minimem_handle * m1;
  mstring * ms;
  int wildsused = 0, defaulttarget;

#define RC_TOKS 50
  char * toks [RC_TOKS];
  int    tokl [RC_TOKS];
  int    toknr;
  int    possible_names;

  csv_desc *csv_descs, *cd, *cd1 = NULL;

  m1 = mm_init (0x4000, 0x400);
  ms = ms_init (m1, 100);
  pics = 0; picsb = 0.0;
  if (single_collection->n)
  {
    lprintf (2, "\nProcessing collections ");
    for (i = 0; i < single_collection->n; i++)
      lprintf (2, "%s ", single_collection->s[i]);
    lprintf (2, "\n");  
  }
  if (!name)	// no config file
  {
    cfgfile = cfgfile0 = "\n\n\0\0";
    name = "";
  }
  else
  {
    char *c, *c1;
    if (!readfile (name, &cfgfile0, NULL))
      error ("Cannot open configfile %s", name);
    c = cfgfile = cfgfile0;
    while (1)       // process switches
    {
      getnewline(&c); // ignore comments
      c1 = c;
      if (*c1 != '-' && *c1 != alternate_switch_char)
         break;
      if (c1[1] == 'r' && c1[2] == 'I')
      {
        do c++; while (!iscr(*c));
        *c = 0; c++;
        report_identity = mm_strdup (m, c1+3);
      }
      else
      {
        do c++; while (!isspace((int)*c));
        *c = 0; c++;
        lprintf (0, " %s", c1);
        process_switch (c1);
      }  
    }
    cfgfile = c;

    if (make_csv)
       error ("switch -C not allowed in config file");
  }
  if (csv_paths->n == 0)
  {
       strcpy  (tmpname, name);       // default: path of the config file
       *name_from_path(tmpname) = 0;
       ms_push (csv_paths, tmpname);  // copy only if nothing is defined
  }   
  
  lprintf (0, "\n\n");
  check_arguments();  	// don't read all CSVs and then say "syntax error"
  
  nr_of_collections = 0;
  total_csvs = 0;
  for (i = 0; i < csv_paths->n; i++)
  {
    sprintf (tmpname, "%s*.*", csv_paths->s[i]);
    total_csvs += expand_wildcard (ms, tmpname, 1);
  }
  // (sigh) Unix can't expand *.csv  (ignores *.CSV then), so I have to use *.*
  // and then check all extensions ...
  j = 0;
  for (i = 0; i < total_csvs; i++)
    if (is_tail(".csv", ((fileinfo *)ms->s[i])->fullname ))
       ms->s[j++] = ms->s[i];
  total_csvs = j;     
  if (total_csvs == 0)
     error ("No CSVs found");
  
  csvs = mm_alloc (m1, total_csvs * sizeof (collection *));
  csv_descs = mm_alloc (m1, total_csvs * sizeof (csv_desc));
  
  for (i = 0; i < total_csvs; i++)
  {
    char * c_name, *c1;
    char tmp_cname[MAXPATH];
    fileinfo * fi = (fileinfo *) ms->s[i];
    int  c_nrpics;
    int  j;
    int  c_rename = 0;
  
    // create and clean up name
    cd = csv_descs + i;
    cd->fullfilename = fi->fullname;
    cd->filename = name_from_path(cd->fullfilename);
    strcpy (tmp_cname, cd->filename);
    c_name = tmp_cname;
    cd->length = fi->size;
    cd->filetime = fi->time;

    c1 = strrchr (c_name, '.');
    *c1 = 0;	      // remove extension ".csv"
    c1--;
    if ((*c1 == 'f' || *c1 == 'F') && isdigit((int) c1[-1]))
    {
      *c1 = 0;	 // something like scanmaster_1000f.csv  -> remove the 'f'
      c1--;
    }  
    while (isdigit((int) *c1) && c1 > c_name)
      c1--;
    if (*c1 == '_' || *c1 == '-')
    {
      c_nrpics = strtoul(c1+1,NULL,10);
      *c1 = 0;
      if (c_nrpics > 50000)	// for StupidScans_03101999.csv
         c_nrpics = 0;
    }
    else
      c_nrpics = 0;
    for (j = 0; j < prefixes->n; j++)
    {
      if ((c1 = is_head (prefixes->s[j], c_name)))
      {
          if (*c1 == '_' || *c1 == '-')
             c1++;
          c_name = c1;
          if (prefixes_remove->s[j][0])
             c_rename = 1;
      } 
      if ((c1 = is_tail (prefixes->s[j], c_name)))
      {
          if (c1[-1] == '_' || c1[-1] == '-')
             c1--;
          *c1 = 0;
          if (prefixes_remove->s[j][0])
             c_rename = 1;
      } 
    }
    if (c_nrpics == 0)
    {			// try again for number after removing suffixes
      c1 = c_name + strlen(c_name) - 1;
      while (isdigit((int) *c1) && c1 > c_name)
        c1--;
      if (*c1 == '_' || *c1 == '-')
      {
        c_nrpics = strtoul(c1+1,NULL,10);
        *c1 = 0;
      }
      
      if (c_nrpics == 0 && kill_duplicate_csvs && readbuffer (cd->fullfilename) )
      {
         c_nrpics = countlines (picbuffer);
         c_rename = 1;
      }
    }
    
    if (*c_name == 0)	// shouldn't happen (e.g. MTCM_100.csv)
    {
       lprintf (2, "Illegal CSV name %s (ignored)\n", cd->filename);
       cd->signame = "";  	// junkname
       continue;
    }
    cd->name = mm_strdup(m1, c_name);
    cd->signame = mm_strdup(m1, c_name);
    make_csv_name_string (cd->signame, cd->signame);    
    cd->nrpics = c_nrpics;
    cd->used = NULL;
    cd->obsolete = NULL;

// remove prefixes / suffixes
// Known Problem: if a suffix is to be removed and a prefix isn't, still both are removed
// I can live with that... :-)

    if (kill_duplicate_csvs && c_rename)
    {
      char * ext, *n;
      strcpy (tmp_cname, cd->fullfilename);
      ext = strrchr (cd->fullfilename, '.');	// might be something else than .csv some day, like .crc
      n = name_from_path(tmp_cname);
      strcpy (n, cd->name);
      if (c_nrpics)
         sprintf (n + strlen(n), "_%d", c_nrpics);
      strcat  (n, ext);
      
      lprintf (0, "Rename %s to %s\n", cd->filename, n);
      if (p_MoveFile(cd->fullfilename, tmp_cname))
      {
        cd->fullfilename = mm_strdup(m1, tmp_cname);
        cd->filename = name_from_path (cd->fullfilename);
      }  
      else
        lprintf (2, "Couldn't rename %s: %s\n", cd->fullfilename, system_error());
    }
  }
  
  qsort (csv_descs, total_csvs, sizeof (csv_desc), (QST) compare_csv_desc);

// handle double entries
  lastsigname = " ";  // this can NEVER be a signame - there's been a crsh because of an empty signame
  j = 0;
  for (i = 0; i < total_csvs; i++)
  {    
    cd = csv_descs + i;
    if (cd->signame[0] == 0)	// empty signame (junkname)
       continue;
    if (strcmp(lastsigname, cd->signame))
    {
      cd1 = csv_descs + j;
      csv_descs[j++] = *cd;
      lastsigname = cd->signame;
    }
    else  // Doublette
      handle_duplicate_csv (cd, cd1);
  }
  total_csvs = j;

  single_col = 0;
  toknr = 1;       // default Initialization
  while (single_col < single_collection->n
         || ( !single_collection->n
              && (toknr = linetokens (cfgfile, &cfgfile, RC_TOKS, toks, tokl, 0, 0))))
  {
    int idx;
    int wild, ia, ie;
    char *c_name, c_signame[MAXPATH], *s;

    if (single_collection->n)
      c_name = toks[0] = single_collection->s[single_col++];
    else
    {
      c_name = toks[0];
      c_name[tokl[0]] = 0;
    }
    idx = 0;
    possible_names = 1;
    wild = strchr (c_name, '*') ? 1 : 0;
    wildsused |= wild;
    idx++;
    c_reportfile = NULL; 

    if (c_name[0] == '#')	// ignore comment
       continue;
    if (c_name[0] == '-')
    {
       if (is_head("-dp", c_name)) get_dir_switch (targetpath, c_name);
       else if (is_head("-dr", c_name)) get_dir_switch (reportpath, c_name);
       else error ("switch %s is not allowed among the collections", c_name);
       continue;
    } 
    if (is_tail(".csv", c_name))
       error ("The configfile %s is of old style\n"
              "(CSV filenames instead of collection names)\n"
              , name);    

    defaulttarget = 1;
    for ( ; idx < toknr; idx++)
    {
      s = toks[idx];
      s[tokl[idx]] = 0;
      if (*s == '#')
         break;
      if (*s == '-')
      {
        if (s[1] == 'p' && s[2])
        {
           char *ct = s+2;
           if (targetpath[0] && ct[0] != PC && ct[1] != ':')
              sprintf (tmpname, "%s%s", targetpath, ct);
           else
              strcpy  (tmpname, ct);
           p_GetFullPathName (tmpname, MAXPATH, c_targetdir);
           defaulttarget = 0;
        }
        else if (s[1] == 'r')
        {
          if (s[2])
          {
           if (reportpath[0] && !strchr(s+2, PC))
             sprintf (tmpname, "%s%s", reportpath, s+2);
           else
             strcpy  (tmpname, s+2);
           c_reportfile = mm_strdup(m, tmpname);
          }   
          else   
            c_reportfile = no_report;
        }
        else
          error ("unknown switch %s in config file behind entry %s", s, c_name);
      }
      else
      {  // add alternate CSV names
        toks[possible_names++] = s;
      }
    }
    
    if (c_name[0] == '*')
    {
      ia = 0;
      ie = total_csvs - 1;
      wildsused = 1;
    }
    else
    {
     int valid_csv[RC_TOKS];
     int best = -1;
     int bestidx = -1;
     for (i = 0; i < (wild ? 1 : possible_names); i++)
     {
      make_csv_name_string (c_signame, toks[i]);
// search using intervall halfing
      found = 0;
      min = 0;
      max = total_csvs - 1;
      do {
        j = (min + max) / 2;
        cmp = strcmp (csv_descs[j].signame, c_signame);
        if (cmp < 0)
          min = j+1;
        else if (cmp > 0)
          max = j-1;
        else 
          found = 1;
      } while (!found && min <= max);
      if (found)
      {
        valid_csv[i] = j;
        if (csv_descs[j].nrpics > best)
        {
          best = csv_descs[j].nrpics;
          bestidx = i;
        }  
      }
      else
        valid_csv[i] = -1;
     } 
     if (wild)
     {
          if (is_head(c_signame, csv_descs[j].signame))
             ia = j;
          else if (j+1 < total_csvs && is_head(c_signame, csv_descs[j+1].signame))
             ia = j+1;
          else
             continue;
          for (ie = ia; ie+1 < total_csvs && is_head(c_signame, csv_descs[ie+1].signame); ie++);   
     }
     else if (best == -1)
     {
          lprintf (2, "No CSV file found for collection %s\n", c_name);
          continue;
     }  
     else
     {
          ia = ie = valid_csv[bestidx];
          for (i = 0; i < possible_names; i++)
            if (i != bestidx && valid_csv[i] >= 0)
               csv_descs[valid_csv[i]].obsolete = csv_descs + ia;
     }    
    }

    for (i = ia; i <= ie; i++)    // once usually, all for wildcard
    {
      cd = csv_descs + i;
      if (cd->used)
      {
         if (!wild)
           lprintf (2, "Collection %s ignored: %s already used for %s\n",
             c_name, cd->filename, cd->used);
         continue;
      }
      if (wild && cd->obsolete)
         continue;
      csv = csvs[nr_of_collections++] = mm_alloc(m, sizeof (collection));
      csv->name = mm_strdup(m, wild ? cd->name : c_name);
      cd->used = csv->name;
      make_csv_name_string (c_signame, csv->name);
      csv->signame = mm_strdup(m, c_signame);
      csv->fullfilename = mm_strdup(m, cd->fullfilename);
      csv->filename = name_from_path(csv->fullfilename);
      // lprintf (0,"%4d %4d %s\n", i, nr_of_collections-1, csv->name);
      if (defaulttarget)
      {
        char * s;
        
        sprintf (tmpname, "%s%s", targetpath, csv->name);
        if (auto_index_dir && (
            (s = is_tail ("index", tmpname)) ||
            (s = is_tail ("extra", tmpname)) ||
            (s = is_tail ("extras", tmpname))  ))
        {
          if ( s[-1] == '_' || s[-1] == '-' )
             s[-1] = PC;
          else
          {
            char b[20]; 
            strcpy (b, s);
            strcpy (s+1, b);
            *s = PC;
          }
        }
        p_GetFullPathName (tmpname, MAXPATH, c_targetdir);
      }  
      csv->targetdir = mm_strdup(m, c_targetdir);

      if (c_reportfile)
        csv->reportfile = c_reportfile;
      else
      {
        sprintf (tmpname, "%s%s.txt", reportpath, csv->name);
        csv->reportfile = mm_strdup(m, tmpname);
      }
      csv->filesize = cd->length;
      csv->filetime = cd->filetime;
      csv->targetexists = 0;
      csv->nrpics  = cd->nrpics; 	// vorlaeufig
      csv->extra = 0;
      csv->extrab = csv->onhdb = 0;
      csv->empty = 1;
      csv->updated = 0;
      csv->Ecsv = 0;
      csv->specialtypes = 0;
      csv->samepaths = NULL;
      csv->haveindex = bf_init (m, have_lists->n);
      csv->extras = NULL;
      if ((j = strlen(csv->name)) > collection_longest)
         collection_longest = j;
    }
  }
  for (i = 0; i < total_csvs; i++)  // junk obsoletes
  {
    cd = csv_descs+i;
    if (cd->obsolete && !cd->used)
      handle_duplicate_csv (cd, cd->obsolete);
  }

  qsort (csvs, nr_of_collections, sizeof (collection *), (QST) compare_csv1);
  collections = mm_memdup (m, csvs, sizeof (collection *) * nr_of_collections);
  if (!badpath[0])
     sprintf (badpath, "%sBadPictures", targetpath);
  else
     badpath[strlen(badpath) - 1] = 0;	// remove trailing path seperator

  if (log_unused_csvs)
  {
   lprintf (0, wildsused ? "\nIf you want to get rid of the Wildcards just copy the following to your config file:\n\n"
              : "\nCollections currently not used in the config file:\n\n");
   for (i = 0; i < total_csvs; i++)
    if (wildsused || !csv_descs[i].used)
      lprintf (0, strchr (csv_descs[i].name, ' ') ? "\"%s\"\n" : "%s\n",
        csv_descs[i].name);
   lprintf (0, "\n");
  }
  
  mm_free (m1);
  if (name[0] != '\0')		// configfile available, not just single collection
     p_alloc_free (cfgfile0);   // free up memory used for reading config file

// read CSV files 

  lprintf (2, "Configfile %s contains %d collections\nReading CSV-Files...\n",
    	name, nr_of_collections);

  for (i = 0; i < nr_of_collections; i++)
  {
    char dinfo;
    char s[MAXPATH];
  
    csv = collections[i];
    if (!csv->filename)
       lprintf (2,"No CSV file found for collection %s\n", csv->name);
    if (!csv->filename || !readbuffer (csv->fullfilename))
    {
      for (j = i; j < nr_of_collections - 1; j++)
        collections[j] = collections[j+1];
      i--;
      nr_of_collections--;
      continue;
    }
    csv->pic_descs = mm_alloc (m, countlines (picbuffer) * sizeof (pic_desc));
    csv->nr = i;
    j = parse_csv (i, picbuffer);
    if (csv->descriptions)
       dinfo = 'D';
    else if (csv->Ecsv)
       dinfo = 'E';
    else
       dinfo = ' ';
    lprintf (0, "CSV-File %3d  %3s %c Lines %4d  \"%s\"  \"%s\"   Path \"%s\"   Report \"%s\"\n",
    	i, j ? "":"CRC", dinfo, csv->nrpics,
    	csv->name, csv->filename, csv->targetdir,
    	name_from_path(csv->reportfile));
    pics += csv->nrpics;
    picsb += csv->nrbytes;
    if (csv->nrpics > collection_maxpics)
       collection_maxpics = csv->nrpics;
    sprintf (s, "%s_%d.csv", csv->name, csv->nrpics);
    if (rename_csvs && strcmp (csv->filename, s))
    {
      lprintf (1, "rename %s to %s\n", csv->filename, s);
      strcpy (tmpname, csv->fullfilename);
      strcpy (name_from_path (tmpname), s);
      rename (csv->fullfilename, tmpname);
      if (strlen(s) > strlen(csv->filename))
        csv->fullfilename = mm_strdup (m, tmpname);
      else
        strcpy (csv->fullfilename, tmpname);
      csv->filename = name_from_path(csv->fullfilename);
    }
  }    // loop over all CSV-Files

  lprintf (2, "Total Pics: %d\n", pics);
  if (pics == 0)
    error ("No pictures found in CSV-Files");

  if (endungen->n > 1)
  {
    lprintf (0, "File extensions in collections:");
    for (i = 0; i < endungen->n; i++)
      lprintf (0, " .%s", endungen->s[i]);
    lprintf (0, "\n");  
  }

  make_index(verbose ? 0 : -1);
}

void move_csvs (void)
{
  collection * csv;
  int i, k, comp;
  char s[MAXPATH], *tp;

  check_make_directory (csv_paths->s[csvpath_used]);
  check_make_directory (csv_paths->s[csvpath_complete]);
  
  for (i = 0; i < nr_of_collections; i++)
  {
    csv = collections[i];
    comp = csv->correct == csv->nrpics;
    k =  comp ? csvpath_complete : csvpath_used;
    tp = csv_paths->s[k];
    sprintf (s, "%s%s", tp, csv->filename);
    if (stricmp(s, csv->fullfilename))  // yeah, that might not be correct under Unix,
    {					// but you don't have to PRESS the case-preserve feature
       lprintf (0, "Move %s CSV %s to %s\n", comp ? "completed" : "used", csv->fullfilename, s);
       if (!p_MoveFile(csv->fullfilename, s))
          lprintf (2, "Couldn't move %s to %s: %s\n", csv->fullfilename, s, system_error());
       else
          csv->fullfilename = mm_strdup (m, s);
    }
  }
}

