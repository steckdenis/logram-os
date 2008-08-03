/*
 * kernel.c
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
	Fichier principal de l'extension kernel.ext.
	Cette extension comporte toutes les fonctions de gestion de la mémoire, des processus, des threads, des interruptions, etc.
	En bref toutes les fonctions vitales à un OS.
*/

// Inclusions
#include <types.h>
#include <logram.h>
#include <drivers.h>

#include "mem.h"
#include "console.h"
#include "interrupts.h"
#include "ext.h"
#include "boot.h"

// Prototypes des fonctions contenues dans l'extension
int 	CompareString	(lchar *s1, lchar *s2);
void 	InitKernel 	();
void 	Test 		();
void	MakeGDT 	();
void	LoadDrivers	();
void	LoadDriver	(int64 block, lchar *drv);
int 	SplitString	(lchar *str, lchar separator);
int 	SplitNext	(lchar *str, lchar **out);

char kernelInited = 0; 	// Détermine si le noyau a été chargé (protection)
DEVICE *devices;	// Liste des pilotes chargés

/*******************************************************************************
	Fonctions principales du noyau
		Contient les fonctions de bases exportées
*******************************************************************************/


/*****************************************
	Fonctions du démarrage
******************************************/

int	ExtMain(void *ext, lint message, lint param)
{
	switch (message)
	{
		case EXT_LOAD:
			//On a chargé l'extension à partir du disque
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
}

// Fonction InitKernel qui initialise le noyau
void InitKernel () {
	// Créer et charger la GDT
	MakeGDT ();
	// Créer et charger l'IDT (donc activer les interruptions)
	MakeIDT ();
	
	//Initialiser le pilote FSL du démarrage
	LoadFSLInfos();
	
	//Charger les drivers
	kprintf ("Loading drivers...", 0x07);
	LoadDrivers();
	kprintf ("Done.", 0x07);
	
}

// Fonction Test qui contiendra tous les tests du système
void Test () {

}

// Fonction MakeGDT qui se charge de créer et charger la GDT
void MakeGDT () {
	// Crée les enregistrements
	CreateSegment(0, 0);		// Segment nul obligatoire
	CreateSegment(1, 0xC09E);	// Segment 32 bits pour les applications Windows
	CreateSegment(2, 0x209C);	// Code 64 bits
	CreateSegment(3, 0x92);		// Segment de données

	// Et enfin, charge la nouvelle GDT
	LoadGDT ();
}

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
		LoadDriver (block, currDriver_u);
		currDriver = eos+1;	//Passer au driver suivant
	} 
	
	//Magnifique, on a tout chargé :)
}

// Charge un driver
// Paramètres : - int64 block 	: bloc du dossier du driver
//		- lchar *drv	: nom du driver
//		- int32 nb	: numéro de driver
void LoadDriver (int64 block, lchar *drv)
{
	kprintf_unicode(drv, 0x07);
}

/*****************************************
	Fonctions exportées
******************************************/

// Compare deux chaînes de caractères, et retourne 1 si elles sont égales, remarquez que ce n'est pas la fonction de la LibC
// Paramètres : - lchar *s1 : première chaîne à comparer avec la deuxième
//		- lchar *s2 : deuxième chaîne à comparer avec la première
int	CompareString(lchar *s1, lchar *s2) {
	while (*s1 == *s2) 
	{
		s1++;
		s2++;
		if (*s1 == 0)
		{
			if (*s2 == 0) 
			{
				return 1;
			}
		}
	}
	return 0;
}

//Coupe une chaîne en plusieurs sous-chaînes.
int SplitString(lchar *str, lchar separator) 
{
	int i = 0;

	while (str[i]) 
	{
		if (str[i] == separator) 
		{
			str[i] = 0;
		}
		i++;
	} 
	return 0;
}	

//Passe à la chaîne suivante dans une chaîne splitée
int SplitNext(lchar *str, lchar **out) 
{
	int i = 0;

	*out = str;

	while(str[i]) 
	{
		*out += 1;
		i++;
	}

	*out += 1;

	return 0;
}
