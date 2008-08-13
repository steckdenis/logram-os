/*
 * exemple.c
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
 	Driver qui s'occupe de tous les volumes de stockages du système
*/

/* En-tête du fichier */

// Inclusions
#include <types.h>
#include <logram.h>
#include <driver.h>

//Structures

typedef struct
{
	lint	(*ReadBlock)(int device, lint block, void *buf);	//Adresse de la fonction de lecture
	lint	(*WriteBlock)(int device, lint block, void *buf);	//Et d'écrite
	int	nDevice;	//Paramètre à envoyer à device.
} volume;			//Structure volume renseignée par les pilotes

//En-tête du fichier d'extension

exports exps[];
section sections[];
void	*nxtDrv;
DEVICE	device;

resext head = 
	{
		sizeof(resext),
		MAGIC_EXT,
		exps,
		sections 
	};

exports exps[] =
	{
		{&ExtMain, L"ExtMain"},
		{&nxtDrv, L"NextDriver"},
		{&device, L"DriverStruct"},
		{ 0, 0}						//Fin des exportations
	};

section sections[] =
	{
		{ 0, 0x1000, SECTION_DATA },			//Données
		{ (void *) 0x1000, 0x1000, SECTION_CODE },	//Code
		{ 0, 0, 0}					//Section de fin
	};
	
void	*nxtDrv = (void *) 0;

DEVICE	device =
	{
		sizeof(DEVICE),
		DRIVERCLASS_MISC,
		0,
		{ L'V', L'O', L'L', L'U', L'M', L'E', 0, 0 }
	};
	
/* Variable globales */

/* Code du fichier */

int ExtMain (void *ext, void *kernAddr, void *efAddr, lint message, lint param)
{
	void* (*find)(void *ext, lchar *nom);
	void (*kprintf)(char *texte, char *code);
	switch (message)
	{
		case EXT_LOAD:
			//On a chargé l'extension à partir du disque
			
			find = efAddr;
			kprintf = find((void *)0x800000, STRING(ext, L"kprintf"));
			
			if (kprintf)
			{
				kprintf(STRING(ext, "Ca marche !"), 0x02);
			} 
			break;
		case EXT_ATTACH:
			//On a attaché l'extension à un processus
			break;
		case EXT_DETACH:
			//On a détaché l'extension d'un processus
			break;
		case EXT_CLOSE:
			//On a déchargé l'extension de la mémoire
			break;
	}
	return 1;	//Tout marche !
}
