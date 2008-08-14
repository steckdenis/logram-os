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
	int64	partitions[4];	//Adresse de départ des 4 partitions
} volume;	//Structure volume renseignée par les pilotes

typedef struct
{
	int	startCHS;	//Amorce, C, H et S
	int	endCHS;		//Système, C, H et S
	int	offset;		//Position
	int	length;		//Taille
} __attribute__((packed)) mbr_record;	//Enregistrement MBR sur le disque

typedef struct
{
	char		code[446];
	mbr_record	records[4];
} __attribute__((packed)) mbr_disk;

//Prototypes

void	addVol(void *ReadBlock, void *WriteBlock, int device);
void	readVol(int vol, int part, int64 block, void *buf);
void	writeVol(int vol, int part, int64 block, void *buf);

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
		
		{&addVol, L"AddVolume"},
		{&readVol, L"ReadVolume"},
		{&writeVol, L"WriteVolume"},
		
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

volume 	*volumes = (volume *) 0;	//Adresse du tableau des volumes
int	currVol = 0;			//On commence au 1 car le 0 est celui de Logram

mbr_disk	disk;

/* Importations */

void*	(*VirtualAlloc)(void *physaddr, int pagesNb, int flags, int16 pid);
void*	(*ExtFind)(void *ext, lchar *nom);

/* Code du fichier */

int ExtMain (void *ext, void *kernAddr, void *efAddr, lint message, lint param)
{
	switch (message)
	{
		case EXT_LOAD:
			//On importe les fonctions
			ExtFind = efAddr;
			VirtualAlloc = ExtFind(kernAddr, STRING(ext, L"VirtualAlloc"));
			
			volumes = VirtualAlloc(0, 1, MEM_PUBLIC, 1);
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

void	addVol(void *ReadBlock, void *WriteBlock, int device)
{
	int i;
	
	//Enregistrer le volume
	volumes[currVol].ReadBlock = (void *) ReadBlock;
	volumes[currVol].WriteBlock = (void *) WriteBlock;
	volumes[currVol].nDevice = device;
	
	//Trouver les partitions
	volumes[currVol].ReadBlock(device, 0, (void *) &disk);	//Qui a dit que la prog "objet" n'existe pas en C ? :q
	
	//Explorer les partitions
	for (i=0;i<4;i++)
	{
		volumes[currVol].partitions[i] = (int64) disk.records[i].offset;
	}
	
	currVol++;
}

void	readVol(int vol, int part, int64 block, void *buf)
{
	//Très facile, c'est juste une ligne un peu longue :
	//	On appelle ReadBlock correspondant au volume, avec son paramètre
	//	On ajoute au block le point de départ de la partoche
	//	Et on écrit dans buf
	volumes[vol].ReadBlock(volumes[vol].nDevice, volumes[vol].partitions[part]+block, buf);
}

void	writeVol(int vol, int part, int64 block, void *buf)
{
	//Très facile, c'est juste une ligne un peu longue :
	//	On appelle ReadBlock correspondant au volume, avec son paramètre
	//	On ajoute au block le point de départ de la partoche
	//	Et on lit dans buf
	volumes[vol].WriteBlock(volumes[vol].nDevice, volumes[vol].partitions[part]+block, buf);
}
