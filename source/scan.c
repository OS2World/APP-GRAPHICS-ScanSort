/* Scansort 1.81 - Scan for pics
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

extern int files_deleted, files_copied, hashsizes_len, hashnames_len;
extern mstring * endungen;
extern pic_desc ** hashnames, ** hashsizes;
extern char * jpg_endung;

// Switches

extern int report_recurse, report_crc, kill_extra_files, kill_bad_files, makehavelist,
           ignore_bad_files, move_files, verbose, min_bad_size, min_bad_namelen,
           dont_recurse;
extern char badpath[];

unsigned char JPG_EOF1 = 0xff;
unsigned char JPG_EOF2 = 0xd9;

int kb(double bytes)
{
  return (int) (bytes / 1024.0 + 0.5);
}  

int mb(double bytes)
{
  return (int) (bytes / (1024.0*1024.0) + 0.5);
}  

// nr: Quell- und Zielpfad der Collection ueberlappen
// -1: o.k.
int check_source_overlap (char * source)
{
  char s[MAXPATH], t[MAXPATH];
  int i;

  p_GetFullPathName (source, MAXPATH, s);
  str2upper (s);
  for (i = 0; i < nr_of_collections; i++)
  {
//    GetFullPathName (collections[i]->targetdir, MAXPATH, t, &n);
    strcpy (t, collections[i]->targetdir);
    str2upper (t);
    if (strstr(t,s))
    {
       lprintf (1, "Source/Target overlap: %s %s\n", s, t);
       return i;
    }
  }     
  return -1;
}

// Search by size in Hashtable
// return pointer to first, NULL if not found
pic_desc ** findpic_size_h (int picsize)	
{
  int hash, h;

  hash = hash_size(picsize, hashsizes_len);
  //  lprintf (0, "f_h: %6d %6d\n", hash, picsize);  
  for (h = 0; hashsizes[hash + h]; h++)
  {
    //    lprintf (0, "%2d %6d %s\n", h, hashsizes[hash + h]->size, hashsizes[hash + h]->name);  
    if (hashsizes[hash + h]->size == picsize)
       return &hashsizes[hash + h];
  }      
  return NULL;         
}

// search for name in Hashtable (with or without extension)
// return pointer to first, NULL if not found
pic_desc ** findpic_name_h (char * picname)	
{
  int hash, h;
  char name[MAXPATH];

  if (checkendung3(picname))
     strcpy (name, picname);
  else
     sprintf (name, "%s.%s", picname, endung);
  hash = hash_name(name, hashnames_len);
  // lprintf (0, "f_h: %6d %s\n", hash, name);  
  for (h = 0; hashnames[hash + h]; h++)
  {
  //  lprintf (0, "%2d %s\n", h, hashnames[hash + h]->name);  
    if (!m_stricmp(hashnames[hash + h]->name, name))
       return &hashnames[hash + h];
  }      
  return NULL;         
}

void handle_bad_file (char * fname, char * name,
			p_FILETIME time, int size, pic_desc ** pp)
{
  unsigned char * jpg_eof, saveit[2];
  unsigned int crc;
  char zname[MAXPATH];
  int fileerror = 0;
  int smallest_size = 10000000;
  pic_desc * p = NULL;
  int is_good = 0;
  int is_loaded = 0;

  if (p_File_is_open(fname))
  {	// used by another process (probably downloading). Doesn't work in Unix.
     lprintf (0, "skip file %s: %s\n", fname, system_error());
     return;
  }

  if (size == 0 && (move_files || kill_bad_files || min_bad_size > 0))
  {
     delete_bad (fname);
     return;
  }   
     
  // testen, ob nur Schrott angehaengt und ob File schon in Collection

  for (; *pp; pp++)
  { 
    p = *pp;
    if (m_stricmp (name, p->name)) continue;

    sprintf (zname, "%s%c%s", collections[p->collection]->targetdir, PC, p->name);
    if (p->exists == yes || exists_length(zname, p->size))
       is_good = 1;
    if (p->size < smallest_size)
       smallest_size = p->size;
    
    if (size <= p->size)
       continue;
    if (!is_loaded && !readbuffer (fname))
       return;
    else
       is_loaded = 1;
    // hier loaded einfuegen   
    jpg_eof = picbuffer + p->size - 2;
    saveit[0] = jpg_eof[0];
    saveit[1] = jpg_eof[1];
    if (endung == jpg_endung)
    {
       jpg_eof[0] = JPG_EOF1;
       jpg_eof[1] = JPG_EOF2;
    }
    crc = crcblock (picbuffer, p->size);
    if (crc == p->crc)		// File gut, nur Schrott dran !
    {
        lprintf (2, "Repaired file with %d extra bytes appended\n", size - p->size);
	checkpic (fname, name, time, p->size);	// so gehts am einfachsten...
	return;
    }
    jpg_eof[0] = saveit[0];
    jpg_eof[1] = saveit[1];
  }
  if (!ignore_bad_files && (int)strlen(bn(name)) > min_bad_namelen)
  {
    if (!is_good && !kill_bad_files 
            && (!min_bad_size || size >= min_bad_size * 0.01 * smallest_size ))
    {
      static int bad_path_exists = 0;
    
        sprintf (zname, "%s%c%s", badpath, PC, name); // und NICHT p->name, sonst wird es unter falschem Namen gespeichert !!!
        if (!bad_path_exists)
        {
           check_make_directory (badpath);
           bad_path_exists = 1;
        }
        GetUnusedFilename (zname);
        if (!is_loaded && !readbuffer (fname))
          return;
        else
          is_loaded = 1;
        if (p_write_picbuffer (zname, size, time) == 0)
           fileerror = 1;
  	else {
  	  lprintf (2, "%s bad file %s\n", move_files ? "Moved" : "Copied", fname);
  	}
    } 
    if ((move_files && (is_good || !fileerror)) || kill_bad_files)
    {
      if (is_good)
         lprintf (1, "A good version of file %s exists, so it's deleted.\n",
         	name_from_path (fname));
      delete_bad (fname);
    }  
  }    
}

void checkpic (char * fname, char * name, p_FILETIME time, int size)
{
  int found, fileerror, copied;
  unsigned int crc;
  char zname[MAXPATH];
  pic_desc * p, **pp, **pp0;
  collection * csv;
  unsigned char * jpg_eof;
  int crc_bad = 1;
  static int all_fileerrors = 0;

  lprintf (0, "      Checking File %s  %d  ", fname, size);
  pp = pp0 = findpic_size_h(size);
  if (!pp)
  {
    lprintf (0, "not in table\n");
    if (makehavelist)
       return;
    pp = findpic_name_h(name);
    if (pp)
       handle_bad_file(fname, name, time, size, pp);
    return;
  }

// Optimierung zum CD-Scannen (d.h. zum extrahieren von ein paar
// Bildern aus einer CD mit einem Haufen Zeugs das schon da ist)
  if (!move_files && !makehavelist)	
  {
    int miss = 0;
    for (; *pp; pp++)
    {
      p = *pp;
      if (p->size != size) continue;
      
      sprintf (zname, "%s%c%s", collections[p->collection]->targetdir, PC, p->name);
      if (p->exists != yes && !exists_length(zname, size))
         miss ++;
    }
    if (!miss)
    {
       lprintf (0, "  All pics of this size exist, skip it.\n");
       return;
    }   
  }
  lprintf (0, "\n");
  if (!readbuffer (fname))
    return;
  found = 0;
  fileerror = 0;
  copied = 0;
  jpg_eof = picbuffer + size - 2;
 do {
  crc = crcblock (picbuffer, size);
     //  Zeiten: mit crc 21s, ohne crc 19.5s -> Multithreading lohnt nicht 
  for (pp = pp0; *pp; pp++)
  {
    p = *pp;
    if (p->size != size) continue;

    if (crc == p->crc || (p->crc == 0 && !m_stricmp(name,p->name)))
    {
      csv = collections[p->collection];
      crc_bad = 0;
      lprintf (0, "   Identified %s as %s %s (%s)\n",
        fname, csv->name, p->name, p->description);
      lprintf (3, "   Identified %s %s\n",
        csv->name, p->name);
      if (makehavelist)
      {
        p->exists = yes;
        csv->empty = 0;
        continue;
      }
      found = 1;

      if (csv->Ecsv)
      {
        sprintf (zname, "%s%s%s", csv->targetdir, p->description, p->name);
        p_adjust_ecsvpath (zname);
      }  
      else  
        sprintf (zname, "%s%c%s", csv->targetdir, PC, p->name);

      if (p->exists != yes && !exists_length(zname, size))
      {
        if (csv->Ecsv)
        {
          char s[MAXPATH];
          sprintf (s, "%s%s", csv->targetdir, p->description);
          p_adjust_ecsvpath (s);
          checktargetpath (s, 1, -1);
        }
        else
          checktargetpath (csv->targetdir, 1, csv->nr);

        DeleteWaste (zname);
        if (p_write_picbuffer (zname, size, time) == 0)
        {
     	  fileerror = 1;
     	  all_fileerrors ++;
     	  if (all_fileerrors >= 3)
     	     error ("Too many errors writing files - aborting");
          continue;
        }   
  	else {
  	  lprintf (5, "%s %s to %s\n", move_files ? "Moved" : "Copied", name, zname);
  	  lprintf (0, "%s %s to %s\n", move_files ? "Moved" : "Copied", fname, zname);
  	  copied = 1;
  	  files_copied++;
  	  p->exists = yes;
          csv->empty = 0;
          csv->updated++;
          csv->onhdb += size;
  	}
      }
    }
    else
    {
      lprintf (0, "   wrong CRC %08X - %08X for %s %s\n", crc, p->crc,
        collections[p->collection]->name, p->name);
    }    
  }
  if (makehavelist || found)
     break;

  if (endung != jpg_endung || (jpg_eof[0] == JPG_EOF1 && jpg_eof[1] == JPG_EOF2))
  {
    if (crc_bad && (pp = findpic_name_h(name)))
         handle_bad_file(fname, name, time, size, pp);
    break;	// Datei i.o
  }  
  else
  {
    jpg_eof[0] = JPG_EOF1;  // repair defekt End
    jpg_eof[1] = JPG_EOF2; 
    lprintf (0, "repaired corrupt file ending - trying again\n");
  }  
 } while (1); 
  // gefunden: Quelldatei loeschen
  if (move_files && found && !fileerror)
  {
    if (!copied)
      lprintf (2, "Deleting %s\n", fname);

    if (copied ? !p_DeleteFile(fname) : !DeleteWaste(fname) )
      lprintf (2,"Error deleting %s: %s\n", fname, system_error());
    else
      if (!copied)
         files_deleted++;
  }
}

// only FEW files are ever bad, so we save 4 Bytes per pic for an entry in the picdesc.
// instead we reallocate the description and put the bad number there

static void put_bad_before_description (char ** pd, int bad)
{
  char * s = mm_alloc (m, strlen(*pd) + 5);
  *(int *) s = bad;
  s += 4;
  strcpy (s, *pd);
  *pd = s;
}

void checkpic_report (char * fname, char * name, p_FILETIME time, int size, int rep_collection)
{
  int i, f, k, min, max, found;
  collection * csv = collections[rep_collection];
  pic_desc *col = csv->pic_descs;
  pic_desc *p, **pp;
  char fname1[MAXPATH];
  char o[MAXPATH + 100];

  sprintf (o, "Checking File %s %7d  ", fname, size);
  min = 0;
  max = csv->nrpics - 1;
  found = 0;
  do {
    i = (min + max) / 2;
    f = m_stricmp (name, col[i].name);
    if (f > 0)
      min = i+1;
    else if (f < 0)
      max = i-1;
    else 
      found = 1;
  } while (!found && min <= max);
  while (found && i > 0 && !m_stricmp (name, col[i-1].name))
    i--;
// Ueberpruefen, ob es moeglicherweise zu einer anderen Collection im gleichen Ordner gehoert
  if (!found && csv->samepaths[0] != -1)
  {
    for (pp = findpic_name_h(name); pp && *pp; pp++)
    {
      if (m_stricmp(name, (*pp)->name)) continue;
      for (k = 0; csv->samepaths[k] != -1; k++)
        if ((*pp)->collection == csv->samepaths[k])
        {
//          lprintf (0, "ColInSamePath: Ignore %s belonging to %d\n", name, csv->samepaths[k]);
          return;
        }
    }    
  }

  if (found && csv->Ecsv)
    for (found = 0; i < csv->nrpics && !m_stricmp (name, col[i].name); i++)
    {
      p = col + i;
      sprintf (fname1, "%s%s%s", csv->targetdir, p->description, p->name);
      p_adjust_ecsvpath (fname1);
      found = !m_stricmp (fname, fname1);
      if (found)
         break;
    }

  if (!found)
  {
    lprintf (0, "%sEXTRA FILE\n", o);
    if (!report_recurse && move_files)
    {
      checkpic (fname, name, time, size);
      if (kill_extra_files && DeleteWaste (fname))
         lprintf (0, "%sDeleted extra file %s\n", o, fname);
    }     
// jetzt checken ob noch da (geloescht oder mit -m verschoben)
    if (exists_length (fname, size))
    {
      // if ((report_extra || report_mtcm) && csv->extra == 0)  store extras always
      {
        char buf[1000];
        if (!csv->extras)
           csv->extras = ms_init (m, 20);
        if (csv->Ecsv)
          sprintf (buf,"%*s  %8d   %s\n",
                   -csv->longest -4, name, size, fname + strlen (csv->targetdir));
        else  
          sprintf (buf,"%*s  %8d\n", -csv->longest -4, name, size);
        ms_push (csv->extras, buf);  
      }    
      csv->extra ++;
      csv->extrab += size;
      csv->onhdb += size;
    }  
    return;
  }
  csv->onhdb += size;
  p = col + i;
  if (strcmp (name, p->name))  // ggf Gross/Kleinschreibung korrigieren
  {
    lprintf (0, "Rename %s to %s\n", fname, p->name);
    strcpy  (fname1, fname);
    strcpy  (name_from_path (fname1), p->name);
    if (rename(fname, fname1))
       lprintf (2, "Couldn't rename %s: %s\n", name, system_error());
  }
  if (size == p->size)
  { int crc_ok = 1;
    unsigned int crc = 0;
  
    //  CRC-Check
    if (report_crc && p->crc)
    {
      if (!readbuffer (fname)
           || (crc = crcblock (picbuffer, size)) != p->crc)
         crc_ok = 0;  
    }
    if (crc_ok)
    {
      p->exists = yes;
      csv->empty = 0;
      if (verbose)
         lprintf (0, "%sOK\n", o);
    }
    else
    {
      p->exists = wrongcrc;
      put_bad_before_description (&p->description, crc);
      lprintf (0, "%s\n", o);
      lprintf (2, "WRONG CRC for %s %s\n",
        csv->name, p->name);
    }
  }
  else
  {
    p->exists = wrongsize;
    put_bad_before_description (&p->description, size);
    lprintf (0, "%sWRONG SIZE\n", o);
  }
}

// Checken, ob Endung im Namen vorhanden (endlen ist IMMER 3)
int checkendung (char *s)
{
  char* s1 = s + strlen(s) - endlen - 1;
  int i;
 
  if (s1 < s || *s1 != '.')
    return 0;

  for (i = 0; i < endungen->n; i++) 
    if (!m_stricmp (s1+1, endungen->s[i]))
      return 1;
  return 0;    
}

void ScanForPics (char * path)
{
  fileinfo fi;
  dirhandle dh;
  
  if ((dh = p_opendir (path, &fi, !dont_recurse, 0, 1)))
     do {
        if (checkendung(fi.name) && stricmp (fi.name, DESCRIPT_ION))
           checkpic (fi.fullname, fi.name, fi.time, fi.size);
     } while (p_nextdir (dh, &fi));
  else   
    lprintf(2, "no files found in \"%s\": %s\n", path, system_error());
}



