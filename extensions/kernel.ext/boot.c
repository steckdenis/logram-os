/*
 * boot.c
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
	Fichier reprenant la majorité de son code de stage2 qui sert de transition entre le chargement du kernel et le chargement des pilotes.
	Il permet l'utilisation du système de fichiers FSL avant le chargement du pilote dédié à cette fonction.
*/

// Inclusions
#include <types.h>
#include <asm.h>
#include <drivers/fsl_driver.h>

#include "boot.h"

// Variables globales
unsigned short 	port;		// Port du contrôleur à utiliser
char 		master;		// Disque maître ou esclave
char 		protocol;	// Protocole utilisé par le disque
FSL_DISK	disk;		// Structure des informations de la partition FSL
int64		logramSect;	// Secteur de départ de la partition Logram

// Fonction LoadFSLInfos qui se charge de charger les informations de la partition FSL (espace allant des secteurs 63 à 2048)
void LoadFSLInfos () {
	ReadSector (logramSect + 63, (void *) &disk);
}

// Fonction ReadSector qui permet la lecture d'un secteur sur le disque actif
// Paramètres : - int64 sector 	: numéro de secteur à lire
//		- void *buf	: buffer qui contiendra le secteur après lecture
void ReadSector (int64 sector, void *buf) {
	switch (protocol) { // Regarde quel procole est utilisé pour le disque
		case ATA_DISK: 		// Protocole ATA
			ATA_ReadSector (sector, buf);
			break;

		case ATAPI_DISK: 	// Protocole ATAPI
			ATAPI_ReadSector (sector, buf);
			break;
	}
}

// Fonction udelay qui fournit un temps de latence à certaines opérations
// Paramètres : - int delay : temps d'attente
void udelay (int delay)
{
	int i;

	for (i = 0; i < (delay * 1000) ; i++) {
	      	i++; i--;
	}
}

// Fonction qui permet la lecture d'un secteur sur les disques ATA
// Paramètres : - int64 sector 	: numéro de secteur à lire
//		- void *buf	: buffer qui contiendra le secteur après lecture
void ATA_ReadSector (int64 sector, void *buf) {
	unsigned char cyl_lo, cyl_hi, sect, head, status;
  	int devselect, i, timeout;
  	int16 *buffer;

	if (master == MASTER_DISK) { // Disque maître
		devselect = ATA_D_MASTER;
	} else { // Disque esclave
		devselect = ATA_D_SLAVE;
	}

  	outb(port + ATA_DEVICE_CONTROL, ATA_A_nIEN | ATA_A_4BIT);
 	udelay (1); // Temps de latence

	sect   = sector & 0xff;
  	cyl_lo = (sector >> 8) & 0xff;
  	cyl_hi = (sector >> 16) & 0xff;
  	head   = ((sector >> 24) & 0x7) | 0x40;

	 // Sélectionne le disque
  	outb(port + ATA_DRIVE, ATA_D_IBM | devselect);
  	udelay (100); // Temps de latence

	// Ecrit dans les registres du contrôleur en positionnant notamment le disque (tête, cylindre, secteur)
  	outb (port + ATA_DEVICE_CONTROL, ATA_A_4BIT);
  	outb (port + ATA_ERROR, 1);
  	outb (port + ATA_PRECOMP, 0);
  	outb (port + ATA_SECTOR_COUNT, 1);
  	outb (port + ATA_SECTOR_NUMBER, sect);
  	outb (port + ATA_CYL_LSB, cyl_lo);
  	outb (port + ATA_CYL_MSB, cyl_hi);
  	outb (port + ATA_DRIVE, ATA_D_IBM | devselect | head);

	// Envoie la commande de lecture
	outb(port + ATA_CMD, ATA_C_READ);

	// Attend la fin de la commande
	for(timeout = 0; timeout < 30000; timeout++) {
	      	status = inb(port + ATA_STATUS);
	      	if(!(status & ATA_S_BSY))
			break;
		udelay (1); // Temps de latence
	}

	// Et lit les données
    	buffer = (int16 *) buf;
    	for (i = 0 ; i < 256 ; i++) buffer [i] = inw (port + ATA_DATA);
}

// Fonction qui permet la lecture d'un secteur sur les disques ATAPI
// Paramètres : - int64 sector 	: numéro de secteur à lire
//		- void *buf	: buffer qui contiendra le secteur après lecture
void ATAPI_ReadSector (int64 sector, void *buf) {
	// Pas encore géré
}

// Fonction FSL_Open qui ouvre un fichier ou un dossier sur disque partitionné en FSL
// Paramètres : - int64 baseBlock 	: bloc de départ du fichier
//		- const lchar *name	: nom du fichier ou dossier à ouvrir
int64 FSL_Open (int64 baseBlock, const lchar *name) {
	FSL_BLOCK 	*block;		// Pointeur sur le bloc courant
	FSL_FILE	*file;		// Structure du fichier à ouvrir
	int 		i;		// Itérateur
	int64		numBlk;		// Numéro du bloc courant
	int64		nxtBlk; 	// Numéro du bloc suivant le courant
	unsigned char 	bufsect [512];	// Buffer de secteur

	// Le bloc courant est égal à celui passé en paramètres
	numBlk = baseBlock;

	while (1) { // Boucle infinie jusqu'à ce qu'on trouve notre fichier
		// Lire l'en-tête du bloc courant
		ReadSector (logramSect + 2048 + numBlk * disk.blocksize, (void *) &bufsect);
		block = (FSL_BLOCK *) &bufsect;

		// Sauvegarde le numéro de bloc suivant
		nxtBlk = block->follow;

		for (i = 1; i < disk.blocksize; i++) { // Pour chaque secteur du bloc courant
			// Lire un secteur d'enregistrement
			ReadSector (logramSect + 2048 + numBlk * disk.blocksize + i, &bufsect);
			file = (FSL_FILE *) &bufsect;
			
			if (strcmp (file->filename, name)) { // Si nous avons trouvé le bon fichier
				return file->startblock; // Alors retourner son adresse
			}
		}

		// On passe au bloc suivant
		numBlk = nxtBlk;

		if (numBlk == 0) { // Si on a atteint la fin du dossier
			return 0; // Alors retourner 0, le fichier n'a pas été trouvé
		}
	}
}

// Fonction FSL_Read qui lit dans un fichier
// Paramètres : - int64 baseBlock 	: bloc de base du fichier à lire
//		- void *buf	  	: buffer qui contiendra la lecture des secteurs
//		- usigned int sectNb	: nombre de secteurs à lire  
void FSL_Read (int64 baseBlock, void *buf, unsigned int sectNb) {
	FSL_BLOCK	*block;		// Pointeur sur le bloc courant
	int 		i;		// Itérateur
	int64		numBlk;		// Numéro du bloc courant
	int64		nxtBlk; 	// Numéro du bloc suivant le courant
	unsigned int	sect;		// Nombre de secteurs lus
	unsigned char 	bufsect [512];	// Buffer de secteur

	// Le bloc courant est égal à celui passé en paramètres
	numBlk = baseBlock;

	while (1) {
		// Lire l'en-tête du bloc courant
		ReadSector (logramSect + 2048 + numBlk * disk.blocksize, (void *) &bufsect);
		block = (FSL_BLOCK *) &bufsect;

		// Sauvegarde le numéro de bloc suivant
		nxtBlk = block->follow;

		for (i = 1; i < disk.blocksize; i++) { // Pour chaque secteur du bloc courant
			sect++; // Un secteur de plus
		
			// Le lire et stocker la lecture dans le buffer
			ReadSector (logramSect + 2048 + numBlk * disk.blocksize + i, buf);
			switch (protocol) { // Selon le protocole, incrémenter le buffer
				case ATA_DISK: // Protocole ATA
					buf += 512;
					break;

				case ATAPI_DISK: // Protocole ATAPI
					// Pas encore géré
					break;
			}
			
			if (sect == sectNb) // Si on a lu les secteurs demandés
			{
				return; // Fin de la fonction
			}

		}

		// On passe au bloc suivant
		numBlk = nxtBlk;

		if (numBlk == 0) { // Si on a atteint la fin du fichier
			return; // Alors terminer la fonction
		}
	}
}

// Fonction strcmp qui compare 2 chaînes
// Paramètres : - const lchar *s1 : chaîne 1
//		- const lchar *s2 : chaîne 2
// Retourne 1 si les chaînes sont les mêmes, sinon 0
int strcmp (const lchar *s1, const lchar *s2) {
	while (*s1 == *s2) {
		s1++;
		s2++;
		if (*s1 == 0) return 1;
	}

	return 0;
}

