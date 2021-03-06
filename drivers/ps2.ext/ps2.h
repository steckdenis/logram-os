/*
 * ps2.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - royalbru
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
	Fichier d'en-tête du pilote.
*/

#ifndef PS2_H
#define PS2_H

// Ports PS/2
#define PS2_COMMAND	0x64
#define PS2_DATA	0x60

// Prototypes
char SendToCmd 		(unsigned char cmd);
char ReadFromData 	(unsigned char *buf);
void SendToData 	(unsigned char data);
void ReadFromCmd 	(unsigned char *buf);

#endif // PS2_H

