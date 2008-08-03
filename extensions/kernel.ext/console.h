/*
 * console.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Bruno Régaldo
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
	Fichier d'en-tête de console.c
*/

#ifndef CONSOLE_H
#define CONSOLE_H

// Prototypes
void ScrollScreen ();
void kprintf(char *str, char style);
void kprintf_unicode(lchar *str, char style);
void kstate(int state);
char *GetBufAddr ();
void SetBufAddr (char *addr);
void MoveBufAddr (short displacement);
void MoveCursor (unsigned char x, unsigned char y);
void ShowCursor ();
void PutCharEx (char buf, char style);
void PutChar (char buf);
void EraseLines (int lines);
void EraseScreen ();
void NextPage ();
void PreviousPage ();

#endif // CONSOLE_H

