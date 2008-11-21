/*
 * console.c
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
	Fonctions relatives à la gestion de la console.
	Attention, ces fonctions ne sont que temporaires.
	Elles seront supprimés dès que le mode graphique sera implémenté.
*/

// Inclusions
#include <types.h>
#include <asm.h>

#include "mem.h"
#include "console.h"

// Variables globales
char *screenbuf = (char *) 0xB81E0;
unsigned int screenLines = 0;

// Fonction ScrollScreen qui bouge l'écran en fonction de la position du curseur
void ScrollScreen () {
	if (screenbuf >= (char *) 0xB8FA0) { // On scroll vers le bas
		memcopy ((char *) 0xB9040 + 160 * screenLines, (char *) 0xB8000, 4000);
		char *clockPosition = (char *) 0xB9040 + 160 * screenLines + 158; // Permet de supprimer l'affichage de l'horloge
		*clockPosition = 0;
		memcopy ((char *) 0xB8000, (char *) 0xB9040 + 160 * (screenLines + 1), 4000);
		screenLines++;
		screenbuf -= 160;
	} else if (screenbuf < (char *) 0xB8000 && screenLines > 0) { // On scroll vers le haut
		memcopy ((char *) 0xB9040 + (160 * screenLines), (char *) 0xB8000, 4000);
		char *clockPosition = (char *)0xB9040 + 160 * screenLines + 158; // Permet de supprimer l'affichage de l'horloge
		*clockPosition = 0;
		memcopy ((char *) 0xB8000, (char *) 0xB9040 + 160 * (screenLines - 1), 4000);
		screenLines--;
		screenbuf += 160;
	} else if (screenbuf < (char *) 0xB8000 && screenLines == 0) { // On veut aller trop haut
		screenbuf = (char *) 0xB8000;
		ShowCursor ();
	}
}

// Fonction GetBufAddr qui retoure l'adresse courante du buffer
char *GetBufAddr () {
	return screenbuf;
}

// Fonction SetBufAddr qui met une nouvelle adresse au buffer
// Paramètres : - char *addr : adresse à mettre
void SetBufAddr (char *addr) {
	screenbuf = addr;
	ScrollScreen ();
	ShowCursor ();
}

// Fonction MoveBufAddr qui déplace le buffer par rapport à son adresse courante
// Paramètres : - short displacement : déplacement du buffer
void MoveBufAddr (short displacement) {
	screenbuf = (char *) screenbuf + displacement;
	ScrollScreen ();
	ShowCursor ();
}

// Fonction PutChar qui écrit un caractère à l'écran
// Paramètres : - char buf : caractère à écrire
void PutChar (char buf) {
	*screenbuf = buf;
	*(screenbuf + 1) = 0x0F;
	MoveBufAddr (2);
}

// Fonction PutCharEx qui écrit un caractère sur l'écran avec sa couleur
// Paramètres : - char buf 	: caractère à écrire
//		- char style 	: couleur du caractère
void PutCharEx (char buf, char style) {
	*screenbuf = buf;
	*(screenbuf + 1) = style;
	MoveBufAddr (2);
}

// Fonction MoveCursor qui déplace le curseur et le réaffiche
// Paramètres : - unsigned char lines : ligne sur l'écran (25 au maximum)
//		- unsigned char chars : colonne sur l'écran (80 au maximum)
void MoveCursor (unsigned char lines, unsigned char chars) {
	unsigned short position;
	position = lines * 80 + chars;

	outb(0x3d4, 0x0f);
	outb(0x3d5, (unsigned char) position);
	outb(0x3d4, 0x0e);
	outb(0x3d5, (unsigned char) (position >> 8));
}

// Fonction ShowCursor qui calcule la position du curseur et l'affiche
void ShowCursor () {
	unsigned char x, y;
	int64 tmp = (int64)(screenbuf - 0xB8000);
	y = tmp / 160;
	x = (tmp % 160) / 2;
	MoveCursor (y, x);
}

// Fonction EraseLines qui supprime le nombre de lignes entré en paramètre
// Paramètres : - int lines : nombre de ligne à supprimer
void EraseLines (int lines) {
	int i, j;
	char *addr = GetBufAddr ();
	char *tmp = addr - 0xB8000;
	tmp = (char *) (((int64)tmp / 160) * 160 + 0xB8000);
	addr = tmp;
	for (i = 0; i < lines; i++) {
		for (j = 0; j < 160; j+=2) {
			*(addr + j) = 0;
			*(addr + j + 1) = 0x07;
		}
		addr += 160;
	}
}

// Fonction qui affiche la page suivante
void NextPage () {
	SetBufAddr ((char *)0xB8FA0);
	int i;
	for (i = 0; i < 24; i++) {
		MoveBufAddr (160);
	}
	SetBufAddr ((char *)0xB8000);
}

// Fonction qui affiche la page précédente
void PreviousPage () {
	SetBufAddr ((char *)0xB8000);
	int i;
	for (i = 0; i < 25; i++) {
		MoveBufAddr (-160);
	}
	SetBufAddr ((char *)0xB8000);
}

// Fonction kstate qui affiche [ok] à la fin de la ligne si state != 0, sinon affiche [failed]
// Paramètres : - int state : message à afficher (0 pour [failed], sinon [ok])
void kstate (int state) {
	MoveBufAddr (-22);

	if (state) {
		//afficher [   ok   ]
		PutCharEx ('[', 0x07);
		MoveBufAddr (6);
		PutCharEx ('o', 0x02);
		PutCharEx ('k', 0x02);
		MoveBufAddr (6);
		PutCharEx (']', 0x07);
		MoveBufAddr (2);
		
	} else {
		//afficher [ failed ]
		PutCharEx ('[', 0x07);
		MoveBufAddr (2);
		PutCharEx ('f', 0x04);
		PutCharEx ('a', 0x04);
		PutCharEx ('i', 0x04);
		PutCharEx ('l', 0x04);
		PutCharEx ('e', 0x04);
		PutCharEx ('d', 0x04);
		MoveBufAddr (2);
		PutCharEx (']', 0x07);
		MoveBufAddr (2);
	}
}

// Fonction qui supprime tout l'écran et la mémoire sauvegardée
void EraseScreen () {
	char *i;
	for (i = (char *) 0xB8000; i < (char *) 0xBDDC0; i += 2) {
		*i = 0;
		*(i + 1) = 0x0F;
	}
	SetBufAddr ((char *)0xB8000);
}

// Fonction kprintf qui permet d'affiche un message à l'écran avec sa couleur
// Paramètres : - char *str 	: chaîne à afficher
//		- char style 	: couleur de la chaîne
void kprintf(char *str, char style) {
        volatile char *mBuf = screenbuf;
        
        while (*str) {
                if(*str == '\n')
		{
                        mBuf = screenbuf += 160;
                        ScrollScreen();
                }
                else
		{
                        *mBuf = *str;
                        mBuf++;
                        *mBuf = style;
                        mBuf++;
                }
 
                str++;
        }
 
        screenbuf += 160;
        ScrollScreen ();
        ShowCursor ();
}

void kprintf_unicode(lchar *str, char style)
{
	int i = 0;
	volatile char *mBuf = screenbuf;
	
	while (str[i]) 
	{
		if((char) *str == '\n')
		{
                        mBuf = screenbuf += 160;
                        ScrollScreen();
                }
		else
		{
			*mBuf = (char) str[i];
			mBuf++;
			*mBuf = style;
			mBuf++;
			i++;
		}
	}
	
	screenbuf += 160;
	ScrollScreen ();
	ShowCursor ();
}
