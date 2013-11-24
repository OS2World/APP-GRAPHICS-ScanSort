/* Scansort 1.81 - external functions
   This stuff is part of other free source
*/

#include "scansort.h"

/*    Fuzzy string search  c't 1997               */
/*    (C) 1997 Reinhard Rapp                                 */
/* sucht n-Gramme im Text und zaehlt die Treffer */

#define  MaxParLen 1000   /* maximale Laenge eines Absatzes */

static int NGramMatch(char* TextPara, char* SearchStr,
               int SearchStrLen, int NGramLen, int* MaxMatch)
{
  char    NGram[8];
  int     NGramCount;
  int     i, Count;

  NGram[NGramLen] = '\0';
  NGramCount = SearchStrLen - NGramLen + 1;

/* Suchstring in n-Gramme zerlegen und diese im Text suchen */
  for(i = 0, Count = 0, *MaxMatch = 0; i < NGramCount; i++)
    {
      memcpy(NGram, &SearchStr[i], NGramLen);
      
      /* bei Wortzwischenraum weiterruecken */
      if (NGram[NGramLen - 2] == ' ' && NGram[0] != ' ')
          i += NGramLen - 3;
      else
        {
          *MaxMatch  += NGramLen;
          if(strstr(TextPara, NGram)) Count++;
        }
    }
  return Count * NGramLen;  /* gewichten nach n-Gramm-Laenge */
}

/* fuehrt die unscharfe Suche durch
RefStr:   Referenz (normalisiert)
MatchStr: anzupassender String
Return:   Aehnlichkeit */

int FuzzyMatching(char* RefStr, char* MatchStr)
{
  char    TextPara[MaxParLen];
  int     TextLen, SearchStrLen;
  int     NGram1Len, NGram2Len;
  int     MatchCount1, MatchCount2;
  int     MaxMatch1, MaxMatch2;
  double  Similarity;

  TextPara[0] = ' ';
  /* n-Gramm-Laenge in Abhaengigkeit vom Suchstring festlegen*/
  SearchStrLen = strlen(RefStr);
  NGram1Len = 3;
  NGram2Len = (SearchStrLen < 7) ? 2 : 5;

      TextLen = strlen (MatchStr);
      if (TextLen < MaxParLen - 2)
        {
          strcpy(&TextPara[1], MatchStr);
          make_search_string (TextPara);
          MatchCount1 = NGramMatch(TextPara, RefStr,
                          SearchStrLen, NGram1Len, &MaxMatch1);
          MatchCount2 = NGramMatch(TextPara, RefStr,
                          SearchStrLen, NGram2Len, &MaxMatch2);

          /* Trefferguete berechnen und Bestwert festhalten  */
          Similarity = 100.0
                     * (double)(MatchCount1 + MatchCount2)
                     / (double)(MaxMatch1 + MaxMatch2);

          return (int) Similarity;
        }
      else
        {
          lprintf(2, "Comment too long: %s\n", MatchStr);
          return 0;
        }
}


/* CRC32 Calculation stuff was taken from the source of JPGCHECK :
 * jcrcsrc.c  derived from jdatasrc.c by Carl Daniel
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a file (or any stdio stream).  While these routines
 * are sufficient for most applications, some will want to use a different
 * source manager.
 * IMPORTANT: we assume that fread() will correctly transcribe an array of
 * JOCTETs from 8-bit-wide elements on external storage.  If char is wider
 * than 8 bits on your machine, you may need to do some tweaking.
 */


typedef unsigned int crc_t;

// CRC-32 polynomial
enum {crcBits = 32, crcPoly = (int)0xEDB88320 };
enum {tblBits = 8, tblSize = 1<<tblBits};

const crc_t initCRC = 0xffffffff;
const crc_t finalXOR = 0xffffffff;

static crc_t table[tblSize];

// ---------------------------------------------------------------
// ---------------------------------------------------------------
static void build_table ()
{

  register int i, j, k;
  crc_t crc;

  for (i = 0; i < tblSize; i++)
  {
    crc = i;
    for (j = 8; j > 0; j--)
    {
      k = (crc & 1) ? crcPoly : 0;
      crc = (crc >> 1) ^ k;
    }
    table[i] = crc;
  }
}

// --------------------------------------------------------------------------
// update a crc with all the bytes from a contiguous range of memory
// --------------------------------------------------------------------------
unsigned int crcblock( unsigned char* p, unsigned count)
{
  crc_t crc = initCRC;
  static int built=0;

  if (!built)
  {
    build_table();
    built=1;
  }

  while (count--)
    crc = ((crc >> 8) & 0x00FFFFFFL) ^ (table[((int) crc ^ *p++) & 0xff]);
  return crc ^ finalXOR;
}

