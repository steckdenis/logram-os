/*
 * drivers.c
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
	Détection et chargement des pilotes
*/

// Inclusions
#include <types.h>
#include <driver.h>
#include <logram.h>

#include "boot.h"
#include "ext.h"
#include "console.h"
#include "mem.h"
#include "drivers.h"

void	*firstDriver=0;		//Premier pilote de la liste chainée

// Extraite de la libC, recherche un caractère dans une chaîne
char *strchr (const char *s, int c)
{
    if (s != 0)
    {
        while (*s && s[0] != c)
        {
            s++;
        }
        if (s[0] == c)
            return (char*)s;
        else
            return 0;
    }
    else
        return 0;
}

void char2wchar(char *in, lchar *out)
{
	while (*in != '\0') { // Conversion en lchar
		*out = (lchar) *in;
		in++;
		out++;
	}
	*out = 0;
}

// Charge tous les drivers
void LoadDrivers()
{
	int64	dblock;			//Bloc du fichier drivers.lst
	int64	block;			//bloc de sys64
	char	*drivers;		//Adresse du buffer dans lequel on a lu drivers.lst
	char	*currDriver; 		//Nom du pilote courant
	lchar	currDriver_u [100]; 	//Nom du pilote courant en unicode
	char 	*eos; 			//Pointe sur le caractère de fin de chaîne
	void	*nxtDrv=0;		//Adresse du pilote suivant
	int	first=1;			//Indique si le pilote est le premier
	
	//On ouvre drivers.lst
	block = FSL_Open(0, L"Logram");
	block = FSL_Open(block, L"sys64");
	dblock = FSL_Open(block, L"drivers.lst");
	
	//On alloue une page
	drivers = (char *) VirtualAlloc(0, 1, MEM_PUBLIC, 1);

	//On lit drivers.lst
	FSL_Read(dblock, (void *) drivers, 8); //8 secteurs = 4ko

	//On charge les pilotes
	currDriver = drivers;
	while (1)
	{
		eos = strchr (currDriver, '\n');
		*eos = '\0';
		char2wchar(currDriver, currDriver_u);
		if (CompareString(currDriver_u, L"EndOfList")) break; //Si c'était le dernier pilote, quitter
		nxtDrv = LoadDriver (block, currDriver_u, nxtDrv);
		currDriver = eos+1;	//Passer au driver suivant
		if (first)
		{
			//Enregistrer le premier pilote de la liste
			firstDriver = nxtDrv;
			first = 0;
		}
	} 
	
	//Magnifique, on a tout chargé :)
}

// Charge un driver
// Paramètres : - int64 block 	: bloc du dossier du driver
//		- lchar *drv	: nom du driver
//		-  void *nxtDrv : driver suivant
void	*LoadDriver	(int64 block, lchar *drv, void *nxtDrv)
{
	int64	dblock;		//bloc du fichier du pilote
	void	*buf;		//Adresse du buffer temporaire où charger la première page du pilote
	resext	*head;		//En-tête du pilote
	section	*sections;	//Adresse des sections
	int64	drvSize;	//Taille du pilote
	int	i = 0;		//compteur
	void	**nxtdrv;	//Pilote suivant
	
	int	(*entry)(void *ext, lint message, lint param);
	
	kprintf_unicode(drv, 0x07);
	
	//Ouvrir le pilote
	dblock = FSL_Open(block, drv);
	
	//Allouer la page
	buf = (void *) VirtualAlloc(0, 1, MEM_PUBLIC, 1);
	
	//Charger le pilote
	FSL_Read(dblock, (void *) buf, 8);
	
	//Explorer la liste des sections
	head = (resext *) buf;
	sections = head->sections;
	sections = (section *)(((int64) sections)+((int64) buf)); //Transtypage lourd, mais le compilo n'aime pas les additions de pointeurs :-/
	while (sections[i].size)
	{
		//Pour chaque section, trouver sa taille
		drvSize += sections[i].size;
		i++;
	}
	
	//On alloue les pages
	drvSize -= 4096;	//On a déjà la première page ;-)
	int64 bug = VirtualAlloc(0, drvSize/4096, MEM_PUBLIC, 1); //On alloue (pas besoin de l'adresse de retour, on alloue juste après *buf). Il faut récupérer le résultat, sinon Logram plante (je ne sais pas pourquoi, il entre en boucle infinie quelque part)

	//On charge le pilote (drvSize/512)
	FSL_Read(dblock, (void *) buf, drvSize/512);	//Et on charge (on écrase buf, mais ce n'est rien, car c'est la page qui nous manque :-)
	
	//Le pilote est chargé, on va pouvoir appeler ExtMain B-)
	entry = ExtFind((void *) buf, L"ExtMain");
	if (!entry)
	{
		kprintf("Invalid driver", 0x04);
		return 0;
	}
	//Appeler la fonction
	if (entry((void *) buf, EXT_LOAD, 0))
	{
		//Le chargement s'est bien passé, on lie le pilote au pilote précédant
		kstate(1);
		if (nxtDrv)
		{
			nxtdrv = (void **) ExtFind((void *) buf, L"NextDriver");
			*nxtdrv = nxtDrv;
		}
	}
	else
	{
		kstate(0);
	}
	return buf;
}

/******************************************************************************
	Fonctions exportées
******************************************************************************/

void	*FindDriver	(lchar *nom)
{
	void	*drv;
	DEVICE	*device;
	void	**nextDriver;
	
	drv = firstDriver;
	
	while (drv)
	{
		device = ExtFind(drv, L"DriverStruct");
		if (CompareString(device->driverName, nom))
		{
			return drv;
		}
		nextDriver = (void **) ExtFind(drv, L"NextDriver");
		drv = *nextDriver;
	}
	return 0;
}

void	*FindDriverId	(int16 id)
{
	void	*drv;
	DEVICE	*device;
	void	**nextDriver;
	
	drv = firstDriver;
	
	while (drv)
	{
		device = ExtFind(drv, L"DriverStruct");
		if (device->id == id)
		{
			return drv;
		}
		nextDriver = (void **) ExtFind(drv, L"NextDriver");
		drv = *nextDriver;
	}
	return 0;
}

