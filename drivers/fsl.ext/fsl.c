/*
 * fslcore.c
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Denis Steckelmacher
 *		      - royalbru
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
	Fichier principal du pilote fsl.ext.
	Ce pilote permet l'utilisation du système de fichiers FSL.
*/

// Inclusions
#include <types.h>
#include <logram.h>
#include <driver.h>
#include <asm.h>
#include <drivers/fsl_driver.h>

//Prototypes

int	FSL_Open(FSL_FILE *file, lchar *nom);	//Ouvre un fichier. HANDLE est la valeur retournée par OpenFile (n° de bloc pour le FSL, mais peut aussi être un pointeur, etc). C'est le pilote qui choisit.
int	FSL_Read(FSL_FILE *file, int64 start, int64 count, void *buf);			//Lis des secteurs du fichier

// En-tête
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
		{&ExtMain, 	L"ExtMain"},
		{&nxtDrv, 	L"NextDriver"},
		{&device, 	L"DriverStruct"},
		{&FSL_Open, 	L"OpenFile"},
		{&FSL_Read, 	L"ReadFile"},
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
		DRIVERCLASS_FS,
		3,
		{ L'F', L'S', L'L', 0, 0, 0, 0, 0 }
	};
	
/* Importations */

void*	(*FindDriver)	(lchar *nom);
void*	(*ExtFind)	(void *ext, lchar *nom);
void	(*kprintf)	(char *str, char attr);
int	(*cmpstr)	(lchar *str1, lchar *str2);

void	(*readVol)(int vol, int part, int64 block, void *buf);
	
/* Variables globales */

void	*Volume = (void *) 0;	//Adresse de volume.ext

// Fonction principale de l'extension
int ExtMain (void *ext, void *kernAddr, void *efAddr, lint message, lint param)
{
	switch (message)
	{
		case EXT_LOAD:
			//On importe les fonctions
			ExtFind = efAddr;
			FindDriver = ExtFind(kernAddr, STRING(ext, L"FindDriver"));
			kprintf = ExtFind(kernAddr, STRING(ext, L"kprintf"));
			cmpstr = ExtFind(kernAddr, STRING(ext, L"CompareString"));
			
			Volume = FindDriver(STRING(ext, L"VOLUME"));
			if (!Volume)
			{
				kprintf(STRING(ext, "    Volume.ext must be loaded before other drivers"), 0x04);
				for (;;) asm("hlt");
			}
			
			readVol = ExtFind(Volume, STRING(ext, L"ReadVolume"));
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
	return 1;
}

void memcopy (char *dst, char *src, int n)
{
	while (n--)
		*dst++ = *src++;	
	return;
}

//FSL_Open : lire un fichier
//	-disk et partition	: le n° de volume et de partition
//	-*file			: pointeur sur une structure FSL_FILE qui contient le fichier parent et contiendra à la fin le fichier ouvert
//	-*nom			: nom du fichier à ouvrir
int	FSL_Open(FSL_FILE *file, lchar *nom)
{
	FSL_BLOCK 	*block;		// Pointeur sur le bloc courant
	FSL_DISK	*mdisk;
	FSL_FILE	*mfile;
	int64		blocksize;
	
	int 		i;		// Itérateur
	int64		numBlk;		// Numéro du bloc courant
	int64		nxtBlk; 	// Numéro du bloc suivant le courant
	unsigned char 	bufsect [512];	// Buffer de secteur

	// Le bloc courant est égal à celui passé en paramètres
	numBlk = file->startblock;	//Pour le pilote FSL, Handle est le n° de block. Il pourrait bien être tout autre chose pour les autres pilotes (comme un pointeur sur une structure, etc)
	
	//Récupérer les infos du disque
	readVol(file->disk, file->userandpart, 63, (void *) &bufsect);
	mdisk = (FSL_DISK *) bufsect;
	blocksize = mdisk->blocksize;

	while (1) { // Boucle infinie jusqu'à ce qu'on trouve notre fichier
		// Lire l'en-tête du bloc courant
		readVol(file->disk, file->userandpart, 2048 + (numBlk * blocksize), (void *) &bufsect);
		block = (FSL_BLOCK *) &bufsect;

		// Sauvegarde le numéro de bloc suivant
		nxtBlk = block->follow;

		for (i = 1; i < blocksize; i++) { // Pour chaque secteur du bloc courant
			// Lire un secteur d'enregistrement
			readVol(file->disk, file->userandpart, 2048 + (numBlk * blocksize) + i, &bufsect);
			mfile = (FSL_FILE *) &bufsect;
			
			if (cmpstr(mfile->filename, nom)) { // Si nous avons trouvé le bon fichier
				//On copie le contenu de mfile dans file
				memcopy((char *) file, (char *) mfile, sizeof(FSL_FILE));
				return 1;	//Le bon fichier se trouve dans *file, on retourne
			}
		}

		// On passe au bloc suivant
		numBlk = nxtBlk;

		if (numBlk == 0) { // Si on a atteint la fin du dossier
			return 0; // Alors retourner 0, le fichier n'a pas été trouvé
		}
	}
}

int	FSL_Read(FSL_FILE *file, int64 start, int64 count, void *buf)
{
	FSL_BLOCK	*block;		// Pointeur sur le bloc courant
	FSL_DISK	*mdisk;
	int64		blocksize;
	int 		i;		// Itérateur
	int64		numBlk;		// Numéro du bloc courant
	int64		nxtBlk; 	// Numéro du bloc suivant le courant
	int64		sect = 0;		// N° de secteur lu
	unsigned char 	bufsect [512];	// Buffer de secteur

	// Le bloc courant est égal à celui passé en paramètres
	numBlk = file->startblock;
	
	//Récupérer les infos du disque
	readVol(file->disk, file->userandpart, 63, (void *) &bufsect);
	mdisk = (FSL_DISK *) bufsect;
	blocksize = mdisk->blocksize;

	while (1) {
		// Lire l'en-tête du bloc courant
		readVol(file->disk, file->userandpart, 2048 + (numBlk * blocksize), (void *) &bufsect);
		block = (FSL_BLOCK *) &bufsect;

		// Sauvegarde le numéro de bloc suivant
		nxtBlk = block->follow;

		for (i = 1; i < blocksize; i++) { // Pour chaque secteur du bloc courant
			if ((sect >= start) && (sect < start+count))
			{
				// Le lire et stocker la lecture dans le buffer
				readVol(file->disk, file->userandpart, 2048 + (numBlk * blocksize) + i, buf);
				buf = (void *) (((int64) buf)+512);
			}
			
			sect++;
			
			if (sect == start+count) // Si on a lu les secteurs demandés
			{
				return 1; // Fin de la fonction
			}

		}

		// On passe au bloc suivant
		numBlk = nxtBlk;

		if (numBlk == 0) { // Si on a atteint la fin du fichier
			return 2; // Alors terminer la fonction (code EOF)
		}
	}
}
