/* Scansort 1.81 - string processing
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
#include <ctype.h>

// replace stupid special characters (umlauts)

static char normalchars[] =
"cueaaaaceeeiiiaaeaaooouuyou     "	// ASCII
"aiounnao                        "	// ASCII
"aaaaaaaceeeeiiiidnooooo ouuuuyps"	// ANSI
"aaaaaaaceeeeiiiidnooooo ouuuuypy";	// ANSI

int normalchar (unsigned int c, int make_underscore)
{
   c &= 0xff;	// damned signed character :-( 
   if (c >= 128)
      c = normalchars[c - 128];
   if (make_underscore && c == ' ')
      c = '_';
   return c;   
}

// own Stricmp function, ignores ' '/'_'
// return: 0 ==, 1: str1 > str2, -1: str1 < str2

static unsigned char m_stricmp_table[256];

void m_stricmp_init(void)
{
  int i, c1;
  for (i = 256; i--;)
  {
    c1 = i;
    if (c1 >= 'A' && c1 <= 'Z') 	c1 += 'a'-'A';
    c1 = normalchar(c1, 1);
    m_stricmp_table[i] = c1;
  }
}

int m_stricmp (unsigned char * s1, unsigned char * s2)
{
  int c1, c2;

  while (*s1)
  {
    c1 = m_stricmp_table[*s1]; c2 = m_stricmp_table[*s2];
    if (c1 == c2)
    {
      s1++; s2++;
    }
    else 
      return c1 < c2 ? -1 : 1;
  }
  return *s2 ? -1 : 0;     
}

// own stricmp function, compares beginning
// return: 0 if nonequal, length of head if equal

char * is_head (unsigned char * s1, unsigned char * s2)
{
   char s[200];		// should be enough
   int l = strlen(s1);

   if (l > 199) l = 199;
   m_strncpy (s, s2, l);
   s[l] = 0;
   return m_stricmp (s, s1) ? NULL : s2 + l;
}

// own stricmp function, compares end
// return: 0 if nonequal, length of tail if equal

char * is_tail (unsigned char * s1, unsigned char * s2)
{
   int l = strlen(s1);
   int l2 = strlen(s2);
   
   if (l2 < l)
      return 0; 
   return m_stricmp (s2 + l2 - l, s1) ? NULL : s2 + l2 - l;
}

// make string for name search (lowercase, spaces)
void make_search_string (unsigned char * s)
{
  int c;
  while (*s)
  {
     if ((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9'))  c = *s;
     else if (*s >= 'A' && *s <= 'Z') c = *s + 'a' - 'A';
     else if (*s == '_') c = ' ';
     else c = normalchar(*s, 0);
     *s++ = c;
  }   
}

// make string for fault tolerant CSV name comparison
// take only letters and numbers, remove scan/scans (except in the beginning)

void make_csv_name_string (unsigned char * dest, unsigned char * s)
{
  char *s1;
  int b;

  strcpy (dest, s);
  str2lower (dest);

  while ((s = strstr (dest+1, "scan")))  // = ist Absicht
  {
    b = s[4] == 's' ? 5 : 4;
    s--;
    do {
      s++;
      *s=s[b];
    } while (*s);  
  }

  s = s1 = dest;
  while (*s)
  {
     if ((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9'))  *s1++ = *s;
     s++;
  }
  *s1 = 0;
}


// strncpy that works CORRECT

char * m_strncpy (char * dest, char *src, int len)
{
  memcpy (dest,src,len);
  dest[len] = 0;
  return dest; 
}

// check if a filename has a 3-char extension (like .jpg)
char * checkendung3 (char *s)
{
  char* s1 = s + strlen(s) - 3 - 1;
 
  if (s1 < s || *s1 != '.')
    return NULL;

  return s1 + 1;   
}

// Basename (no extension, for output) (not reentrant !)
// Problem: printf ("%s %s", bn(a), bn(b)) would return the same string twice
// -> make a round-robin
#define BN_RE 5

char * bn (char *name)
{
  static char s[BN_RE][MAXPATH];
  static int bni = 0;
  char * s1;

  bni = (bni+1) % BN_RE;
  
  strcpy (s[bni], name);
  if ((s1 = strrchr (s[bni], '.')))
     *s1 = 0;
  return s[bni];
}

// extract filename from path
char * name_from_path (char * path)
{
  int i;
  for (i = strlen(path); i >= 0; i--)
    if (path[i] == PC || path[i] == ':')
       break;
  return path + i + 1;   
}

void str2upper (char * s)
{
  int i;
  
  for (i=strlen(s); i--;)
    s[i] = (char) toupper(s[i]);
}

void str2lower (char * s)
{
  int i;
  
  for (i=strlen(s); i--;)
    s[i] = (char) tolower(s[i]);
}

// convert to uppercase if s is a DOS-name
void name2upper (char * s)
{
  int i, l;

    l = strlen (s);
    for (i=0; i < l; i++)
    {
      if (s[i] == '.')
        break;
    }
    if (i > 8 || l - i > 4)
      return;
    str2upper(s);
}

// convert to lowercase, possibly extension only
void name2lower (char * s, int ext_only)
{
  if (ext_only)
  {
    char * p;

    if ((p = strrchr(s, '.')) != NULL) str2lower(p);
  }
  else
  {
    str2lower(s);
  }  
}

// clean junk characters out of name (used in trading and model collections)

void clean_name (char *s)
{
  while (*s)
  {
    *s =   (*s >= 'A' && *s <= 'Z')
        || (*s >= 'a' && *s <= 'z')
        || (*s >= '0' && *s <= '9')
        || *s == '-' || *s == ':'
        || *s == '.' || *s == PC || *s == WINPC	? *s : '_';
    s++;
  }  
}

