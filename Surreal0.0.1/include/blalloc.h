/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/blalloc.h
 *
 *  Copyright (C) 1990-2007 by the past and present ircd coders, and others.
 *  Refer to the documentation within doc/authors/ for full credits and copyrights.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: blalloc.h 985 2007-02-04 20:51:36Z shadowmaster $
 */

#ifndef BLALLOC_H
#define BLALLOC_H
/* INCLUDES */
#include <stddef.h>

/* DEFINES */
#define BlockHeapALLOC(bh, type)	((type *) BlockHeapAlloc(bh))

/* TYPEDEFS */

/* Block contains status information for an allocated block in our heap. */

typedef struct Block
{
  void *elems;			/* Points to allocated memory */
  void *endElem;		/* Points to last elem for boundck */
  int freeElems;		/* Number of available elems */
  struct Block *next;		/* Next in our chain of blocks */
  unsigned long *allocMap;	/* Bitmap of allocated blocks */
}
Block;

/* BlockHeap contains the information for the root node of the memory heap. */

typedef struct BlockHeap
{
  size_t elemSize;		/* Size of each element to be stored */
  int elemsPerBlock;		/* Number of elements per block */
  int numlongs;			/* Size of Block's allocMap array */
  int blocksAllocated;		/* Number of blocks allocated */
  int freeElems;		/* Number of free elements */
  Block *base;			/* Pointer to first block */
}
BlockHeap;

/* FUNCTION PROTOTYPES */

BlockHeap *BlockHeapCreate (size_t elemsize, int elemsperblock);
int BlockHeapDestroy (BlockHeap * bh);
void *BlockHeapAlloc (BlockHeap * bh);
int BlockHeapFree (BlockHeap * bh, void *ptr);
int BlockHeapGarbageCollect (BlockHeap *);

extern void     initBlockHeap(void);

#endif
