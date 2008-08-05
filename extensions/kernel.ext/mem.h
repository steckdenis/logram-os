/*
 * mem.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Denis Steckelmacher
 *		      - Bruno Régaldo
 *
 * Logram is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Logram is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Logram; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

/*
	Fichier d'en-tête de mem.c.
*/

#ifndef MEM_H
#define MEM_H

// Inclusions
#include <types.h>

#include "processes.h"

//Structures

typedef struct {
	int16 limit;
	int16 base0_15;
	int8 base16_23;
	int16 flags;
	int8 base24_31;
} __attribute__ ((packed)) gdtrec;

typedef struct {
	int16 limit;
	int16 base0_15;
	int8 base16_23;
	int16 flags;
	int8 base24_31;
	int32 base32_63;
	int32 zeros;
} __attribute__ ((packed)) gdtsysrec;

// Defines

// Types de pages
#define PAGE_PRESENT		0x00000001
#define PAGE_WRITE		0x00000002
#define PAGE_USER		0x00000004
#define PAGE_WRITETHROUGH	0x00000008
#define PAGE_CACHEDISABLE	0x00000010
#define PAGE_ACCESSED		0x00000020
#define PAGE_DIRTY		0x00000040
#define PAGE_PAGESIZE		0x00000080
#define PAGE_PAT		0x00000080
#define PAGE_GLOBAL		0x00000100
#define PAGE_NOTFULL		0x00000200
#define PAGE_NX			0x8000000000000000

// Flags
#define MEM_NOTCACHEABLE	0x00000001
#define MEM_WRITETHROUGH	0x00000002
#define MEM_NX			0x00000004
#define MEM_READONLY		0x00000008
#define MEM_PROTECTED		0x00000010
#define MEM_NOTPAGEABLE		0x00000020
#define MEM_PUBLIC		0x00000040
#define MEM_PRIVATE		0x00000080
#define MEM_OUTPHYSICAL		0x00000100

// Prototypes
int64 	VirtualAlloc 		(int64 _physAddr, unsigned int pagesNb, int mflags, int pid);
void	VirtualFree		(int64 virtAddr, unsigned int pagesNb, char publicMem);
void 	memcopy			(char *dst, char *src, int n);
void 	CreateSegment		(int index, int16 flags);
void 	CreateSysSegment	(int index, int64 baseAddress, int16 limit, int16 flags);
void	CreatePagingStructure 	(int mflags, Process *process);
void	ReadSegment		(int index, gdtsysrec *out);
void	*GetTSSBaseAddr		(int16 desc);

#endif // MEM_H

