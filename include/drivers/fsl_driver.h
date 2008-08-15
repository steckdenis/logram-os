/*
 * fsl_driver.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Bruno Régaldo
 *		      - Denis Steckelmacher
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

#ifndef FSL_DRIVER_H
#define FSL_DRIVER_H

// Codes d'erreur
#define FILE_FULL_DISK		-4
#define FILE_INVALID		-3
#define FILE_OVERFLOW		-2
#define FILE_NOT_FOUND		-1
#define FILE_SUCCESS		0

// Types de blocs
#define	FSL_BLOCKTYPE_FREE	0
#define	FSL_BLOCKTYPE_FILE	1
#define	FSL_BLOCKTYPE_DIR	2

typedef struct {
	int64	version;
	int64	partitionstart;
	int64	size;
	int64	blocksize;
	int64	flags;
	int64	freespace;
	int64	usedspace;
	WCHAR	name[48];
	WCHAR	description[212];
	WCHAR	tags[512];
} FSL_DISK, *lpFSL_DISK;

typedef struct __attribute__((__packed__)) {
	int16	attributes;
	int16	permissions;
	int16	userandpart;		//Utilisateur et partition (Utilisateur dans le byte haut, partition dans le bas)
	int16	disk;			//Disque
	int64	sizebytes;
	int64	sizeblocks;
	int64	startblock;
	int64	creationdate;
	WCHAR	filesystem[8];
	WCHAR	filename[228];
} FSL_FILE, *lpFSL_FILE;

typedef struct __attribute__((__packed__)) {
	int32	bloctype;
	int64	follow;
	int64	startoffset;
	int64	endoffset;
	int32	moveable;
	char	reserved[32];
	int64	previouslyblock;
} FSL_BLOCK, *lpFSL_BLOCK;

#endif // FSL_DRIVER_H

