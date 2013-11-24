/* Scansort 1.81 - memory management
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

// General memory handling

char * corestart = (char *) 0xFFFFFFFF, * coreend = (char *) 0;
int  coreused = 0;

// Alignment (0: 8 Bit, 1: 16 Bit, 3: 32 Bit, 7: 64 Bit (for doubles !))
// This is important for non-Intel CPUs that can't access words on odd adresses

extern int align;

void print_mem_used (int lev)
{
  lprintf (lev, "Memory used: %d  %d\n", coreend - corestart, coreused);
}

void * mymalloc (int l)
{
  char * p = p_alloc(l);

  if (!p)
    error ("Can't allocate %d bytes of memory", l);
  if (p < corestart)
     corestart = p;
  else if (p > coreend)
     coreend = p + l;
  coreused += l;   

/*
{ // mix it up for debugging
  int i;
  for (i = l; i--;)
    p[i] = 0xa6;
}    
*/
  return p;  
}

void myfree (void *p)
{
   coreused -= p_alloc_size (p);
   p_alloc_free (p);
}

/*  I don't trust realloc under Windows. Under Unix, however,
    there's no way around since there's no way to determine the
    size of a memory block... :-(  */

void * myrealloc (void * pold, int l)
{
  char * p;
  int  l1;

  if (pold)
  {
    l1 = p_alloc_size (pold);
    if (l1)
    {
      coreused -= l1;
      p = mymalloc(l);
      if (l1 > l)
         l1 = l;
      memcpy (p, pold, l1);
      p_alloc_free (pold);
    }
    else
      p = realloc (pold, l);
  }
  else  // p_alloc_size is just a dummy :-(
    p = mymalloc(l); 
  return p;  
}

/*
   Mini Memory handler - fast and efficient
*/

#define PLEN (sizeof (char *))

minimem_handle * mm_init (int size, int waste)
{
  char * b;
  minimem_handle * m;

  if (size < 1024)
     error ("Minimum size for minimem_buffer is 1024");
  b = mymalloc (size);
  ((char **) b)[0] = NULL;
  m = (minimem_handle *) (b + PLEN);   // this bug was evil !!!
  m->base = b;
  m->current = b + PLEN + sizeof (minimem_handle);
  m->size  = size;
  m->free  = size - (PLEN + sizeof (minimem_handle));
  m->waste = waste;
  return m;
}
   
static void * _mm_alloc (minimem_handle * m, int size, int align)
{
  char * b;
  int i;

  if (size + align <= m->free)
  {	// normal allocatiom
     b = m->current;
     if ((i = (int)b & align))	// = and & are correct !
     {
       i = align + 1 - i;
       size += i;
       b += i;
     }  
     m->current += size;
     m->free    -= size;
  }
  else if (m->free > m->waste || size + PLEN > (unsigned) m->size)
  {	// free entry
     b = mymalloc (size + PLEN);
     ((char **)b)[0] = m->base;
     m->base = b;
     b += PLEN;
  }
  else
  {	// allocate new buffer
     b = mymalloc (m->size);
     ((char **)b)[0] = m->base;
     m->base = b;
     m->current = b + PLEN + size;
     m->free = m->size - PLEN - size;
     b += PLEN;
  }
// lprintf (0, "mm_alloc: %p  size: %d free: %d\n", b, size, m->free);
  return b;
}

void * mm_alloc (minimem_handle * m, int size)
{
  return _mm_alloc (m, size, align);
}

char * mm_strdup (minimem_handle * m, char * s)
{
  char * b = _mm_alloc (m, strlen(s) + 1, 0);
  strcpy (b,s);
  return b; 
}

void * mm_memdup (minimem_handle * m, void * s, int len)
{
  char * b = mm_alloc (m, len);
  memcpy (b,s,len);
  return b; 
}

char * mm_strndup (minimem_handle * m, char * s, int len)
{
  char * b = _mm_alloc (m, len + 1, 0);
  memcpy (b,s,len);
  b[len] = 0;
  return b; 
}

void mm_free (minimem_handle * m)
{
  char * b, * b1;

  for (b = m->base; b; )
  {
    b1 = b;
    b  = ((char **)b)[0];
    myfree (b1);
  }
}

void mm_debug (minimem_handle * m)
{
  char * b, * b1;

  lprintf (0, "mm_debug (m = %p)\n", m);
  lprintf (0, "Base %p  Current %p  size %d  free %d  waste %d\n",
           m->base, m->current, m->size, m->free, m->waste);
  for (b = m->base; b; )
  {
    b1 = b;
    b  = ((char **)b)[0];
    lprintf (0, "%p  %d\n", b1, p_alloc_size(b1));
  }
}

minimem_handle * m;

// String library

mstring * ms_init (minimem_handle * m, int size)
{
  mstring * ms = mm_alloc (m, sizeof (mstring));
  ms->s = mm_alloc (m, size * PLEN);
  ms->n = 0;
  ms->max = size;
  ms->m = m;
  ms->rl = 0;
  return ms;
}

char * ms_npush (mstring * ms, void * _s, int l)
{
  char *s1, *s = (char *) _s;
  
  if (ms->n == ms->max)
  {
    int i = (int) (ms->max * 1.5);
    ms->recycle = (char *) ms->s;
    ms->rl = ms->max * PLEN;
    ms->s = mm_alloc(ms->m, i * PLEN);
    memcpy (ms->s, ms->recycle, ms->rl);
    ms->max = i;
  }
  if (l <= ms->rl)
  { // recycle
    s1 = ms->recycle;
    ms->recycle += l;
    ms->rl -= l;
    memcpy (s1, s, l);
  }
  else
    s1 = mm_memdup(ms->m, s, l);
  ms->s[ms->n++] = s1;
  return s1;
}

char * ms_push (mstring * ms, char * s)
{
  return ms_npush (ms, s, strlen(s) + 1);
}

// Bit fields

char * bf_init (minimem_handle * m, int size)
{
  char * s;
  int n = size / 8;

  if (size == 0 || size % 8)	// alloc at least one byte
     n++;
  s = mm_alloc (m, n);
  do s[--n] = 0;
     while (n);
  return s;   
}

void bf_write (char * b, int nr, int value)
{
  int idx = nr / 8;
  int mask = 1 << (nr % 8);

  if (value)
     b[idx] |= mask;
  else
     b[idx] &= mask ^ 0xff;
}

void bf_set (char * b, int nr)
{
  bf_write (b,nr,1);
}

void bf_clear (char * b, int nr)
{
  bf_write (b,nr,0);
}

int bf_test (char * b, int nr)
{
  int idx = nr / 8;
  int mask = 1 << (nr % 8);

  return (b[idx] & mask) != 0;
}

