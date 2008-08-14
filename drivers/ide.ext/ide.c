/*
 * idecore.c
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
	Fichier principal du pilote ide.ext.
	Ce pilote est chargé de gérer les accès sur les disques durs IDE.
*/

// Inclusions
#include <types.h>
#include <logram.h>
#include <driver.h>
#include <asm.h>
#include <drivers/ide_driver.h>

#include "ide.h"

// Variables globales
exports exps[];
section sections[];
void	*nxtDrv;
DEVICE	device;

//Prototypes

void	DetectDrives(void *ext);

//En-tête

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
		{&ReadBlock, 	L"ReadBlock"},
		{&WriteBlock,	L"WriteBlcok"},
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
		DRIVERCLASS_BLOCK,
		2,
		{ L'I', L'D', L'E', 0, 0, 0, 0, 0 }
	};
	
/* Importations */

void*	(*FindDriver)	(lchar *nom);
void*	(*ExtFind)	(void *ext, lchar *nom);
void	(*kprintf)	(char *str, char attr);

void	(*addVol)	(void *ReadBlock, void *WriteBlock, int device);

/* Variables globales */

void	*Volume = (void *) 0;	//Adresse de volume.ext
	
// Fonction principale de l'extension
int ExtMain (void *ext, void *kernAddr, void *efAddr, lint message, lint param)
{
	switch (message)
	{
		case EXT_LOAD:
			//Détecter les disques et les enregistrer dans volume.ext
			ExtFind = efAddr;
			FindDriver = ExtFind(kernAddr, STRING(ext, L"FindDriver"));
			kprintf = ExtFind(kernAddr, STRING(ext, L"kprintf"));
			
			Volume = FindDriver(STRING(ext, L"VOLUME"));
			
			addVol = ExtFind(Volume, STRING(ext, L"AddVolume"));
			
			//Détecter les disques
			DetectDrives(ext);
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

void	DetectDrives(void *ext)
{
	int i, j, portControl, port;
	unsigned char a, b, byte1, byte2, status;
	
	// Tester les ports IDE
	for (i = 0; i < 4; i++) 
	{
		a = 0;
		b = 0;
		
		// Déterminer quel port sera utilisé pour cette itération
		if (i == 0) { port = 0x1F0; portControl = 0x3F0; }
		if (i == 1) { port = 0x170; portControl = 0x370; }
		if (i == 2) { port = 0xF0; portControl = 0x2F0; }
		if (i == 3) { port = 0x70; portControl = 0x270; }

		// Regarde si le contrôleur est présent
		char byte1 = inb (port + 6);
		char byte2 = (byte1 & 0x10) >> 4;
		if(byte2 == 0) 
		{
			byte1 |= 0x10;
			outb(port + 6, byte1);
		} 
		else 
		{
			byte1 = byte1 & 0xEF;
			outb(port + 6, byte1);
		}
		byte1 = inb(port + 6);
		byte1 = (byte1 & 0x10) >> 4;
		if(byte1 != byte2) 
		{ 
			//Le contrôleur existe
			
			//Voir si le disque maitre existe
			outb(port + 6, 0xa0);	//Master
			outb(port + 7, 0xec); 	//Commande d'indentification
			for (j = 0; j < 30000; j++)
			{
				status = inb(port + ATA_STATUS);
				if(!(status & ATA_S_BSY))
				{
					//Le disque existe, l'ajouter
					for (j=0;j<256;j++) inw(port + ATA_DATA); //On ne s'en sert pas
						
					//On ajoute
					addVol(STRING(ext, &ReadBlock), STRING(ext, &WriteBlock), i*2);
					
					goto disksuiv;
				}
				iowait();
			}
			
			disksuiv:
			
			//Voir si le disque esclave existe
			outb(port + 6, 0xb0);	//Master
			outb(port + 7, 0xec); 	//Commande d'indentification
			for (j = 0; j < 30000; j++)
			{
				status = inb(port + ATA_STATUS);
				if(!(status & ATA_S_BSY))
				{
					//Le disque existe, l'ajouter
					for (j=0;j<256;j++) inw(port + ATA_DATA); //On ne s'en sert pas
						
					//On ajoute
					addVol(STRING(ext, &ReadBlock), STRING(ext, &WriteBlock), (i*2)+1);
					
					goto finverif;
				}
				iowait();
			}
			
			finverif: ;
		} 
	}
}

// Fonction ReadBlock qui permet la lecture d'un secteur sur le disque
// Paramètres : - int device : permet la sélection du bon disque
//		- lint block : numéro de secteur à lire
//		- void *buf  : buffer de sortie qui contiendra la lecture du secteur
lint	ReadBlock(int device, lint block, void *buf) {
	unsigned char cyl_lo, cyl_hi, sect, head, status;
	int devselect, i, timeout;
	int16 *buffer;
	int16 port; // Port du contrôleur

	if (device&1) { // On veut le disque esclave
		devselect = ATA_D_SLAVE;
	} else {	// On veut le disque maître
		devselect = ATA_D_MASTER;
	}

	if (device&2) { // On veut le second contrôleur IDE
		port = 0x170;
	} else {	// On veut le premier contrôleur IDE
		port = 0x1F0;
	}

	// Initialise le contrôleur
	outb(port + ATA_DEVICE_CONTROL, ATA_A_nIEN | ATA_A_4BIT);
	iowait();

	// Convertit l'adresse LBA en adresse CHS
	sect   = (block & 0xff);
	cyl_lo = (block >> 8) & 0xff;
	cyl_hi = (block >> 16) & 0xff;
	head   = ((block >> 24) & 0x7) | 0x40;

	// Sélectionne le disque
	outb(port + ATA_DRIVE, ATA_D_IBM | devselect);
	iowait();

	// Positionne le disque pour la lecture
	outb(port + ATA_DEVICE_CONTROL, ATA_A_4BIT);
	outb(port + ATA_ERROR, 1);
	outb(port + ATA_PRECOMP, 0);
	outb(port + ATA_SECTOR_COUNT, 1);
	outb(port + ATA_SECTOR_NUMBER, sect);
	outb(port + ATA_CYL_LSB, cyl_lo);
	outb(port + ATA_CYL_MSB, cyl_hi);
	outb(port + ATA_DRIVE, (ATA_D_IBM | devselect | head));

	// On envoie la commande de lecture
	outb(port + ATA_CMD, ATA_C_READ);

	// Attend la disponibilité du contrôleur
	for(timeout = 0; timeout < 30000; timeout++) {
		status = inb(port + ATA_STATUS);
		if(!(status & ATA_S_BSY))
			break;

		iowait();
	}
	
	// Récupère la lecture du disque
	buffer = (int16 *) buf;
	for (i = 0; i<256; i++) buffer[i] = inw(port + ATA_DATA);

	return 0; // Pas d'erreur
}

// Fonction WriteBlock qui permet d'écrire un secteur sur le disque
// Paramètres : - int device : permet la sélection du bon disque
//		- lint block : numéro de secteur spécifiant l'emplacement de l'écriture
//		- void *buf  : buffer contenant ce qu'il devra être écrit
lint	WriteBlock(int device, lint block, void *buf) {
	unsigned char cyl_lo, cyl_hi, sect, head, status;
	int devselect, i, timeout;
	int16 *buffer;
	int16 port; // Port du contrôleur

	if (device&1) { // On veut le disque esclave
		devselect = ATA_D_SLAVE;
	} else { // On veut le disque maître
		devselect = ATA_D_MASTER;
	}

	if (device&2) { // On veut le second contrôleur IDE
		port = 0x170;
	} else { // On veut le premier contrôleur IDE
		port = 0x1F0;
	}

	// Initialise le contrôleur
	outb(port + ATA_DEVICE_CONTROL, ATA_A_nIEN | ATA_A_4BIT);
	iowait();

	// Convertit l'adresse LBA en adresse CHS
	sect   = (block & 0xff);
	cyl_lo = (block >> 8) & 0xff;
	cyl_hi = (block >> 16) & 0xff;
	head   = ((block >> 24) & 0x7) | 0x40;

	// Sélectionne le disque
	outb(port + ATA_DRIVE, ATA_D_IBM | devselect);
	iowait();

	// Positionne le disque pour la lecture
	outb(port + ATA_DEVICE_CONTROL, ATA_A_4BIT);
	outb(port + ATA_ERROR, 1);
	outb(port + ATA_PRECOMP, 0);
	outb(port + ATA_SECTOR_COUNT, 1);
	outb(port + ATA_SECTOR_NUMBER, sect);
	outb(port + ATA_CYL_LSB, cyl_lo);
	outb(port + ATA_CYL_MSB, cyl_hi);
	outb(port + ATA_DRIVE, (ATA_D_IBM | devselect | head));

	// Envoie au contrôleur la commande d'écriture
	outb(port + ATA_CMD, ATA_C_WRITE);

	// Attend que la disponibilité du contrôleur
	for(timeout = 0; timeout < 30000; timeout++) {
		status = inb(port + ATA_STATUS);
		if(!(status & ATA_S_BSY))
			break;

		iowait();
	}
	
	// On écrit les données sur le disque
	buffer = (int16 *) buf;
	for (i = 0; i<256; i++) outw(port + ATA_DATA, buffer[i]);

	return 0; // Pas d'erreur
}


