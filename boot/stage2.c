/*
 * stage2.c
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
	Stage2 de Logram
	Charge kernel.ext en mémoire et lui donne la main.
	Voici les étapes précises de stage2 :
		1. Identifiez le support actif (disque ata, sata, cdrom atapi)
		2. Charger les Infos disque FSL
		3. Ouvrir kernel.ext
		4. Le charger en mémoire
		5. Initialiser la pagination publique
		6. Passer en mode long
		7. Sauter dans kernel.ext
*/

// Inclusions
#include <types.h>
#include <drivers/fsl_driver.h>
#include <asm.h>

#include "stage2.h"

// Variables globales
char 		*screenbuf = (char *) 0x98000 + 160; 			// +160 pour sauter la première ligne.
unsigned short 	port 	 	__attribute__((section("data")));	// Port du contrôleur à utiliser
char 		master 	 	__attribute__((section("data")));	// Disque maître ou esclave
char 		protocol 	__attribute__((section("data")));	// Protocole utilisé par le disque
FSL_DISK	disk	 	__attribute__((section("data")));	// Structure des informations de la partition FSL
int64		logramSect	__attribute__((section("data")));	// Secteur de départ de la partition Logram
unsigned char 	bufsect [512] 	__attribute__((section("data")));	// Buffer de secteur

// Fonction start qui doit charger kernel.ext
// Appelée par la fonction main du fichier stages.s
void start () {
	// Prévient l'utilisateur que nous allons charger kernel.ext
	print ("Stage 2 loaded !");
	print ("Loading Logram\\sys64\\kernel.ext");

	// Identifier sur quel support, Logram est actuellement utilisé
	FindDevice ();

	// Charger les infos de la partition FSL
	LoadFSLInfos ();

	// Ouvrir kernel.ext et le charger en mémoire à l'adresse 0x800000
	LoadKernel ();

	// Pagine la mémoire publique pour le kernel
	CreatePublicTable ();

	// Passer en mode long et sauter dans kernel.ext
	JmpToKernel ();
}

// Fonction print qui affiche un message à l'écran
// Paramètres : - const char *str : chaîne à afficher
void print(const char *str) {
	char *mBuf;
	
	mBuf = screenbuf;

	while (*str) { // Ecrit la chaîne
		*mBuf = *str;
		mBuf += 2;
		str++;		
	}

	screenbuf += 160; // Saute une ligne
}

// Fonction CreatePublicTable qui pagine la mémoire publique
void CreatePublicTable () {
	int i = 0;
	int64 *PLM4E = (int64 *) (0x50000-0x20000);
	int64 *PDPE = (int64 *) (0x51000-0x20000);
	int64 *PDE = (int64 *) (0x200000-0x20000);
	int64 *PTE = (int64 *) (0x201000-0x20000);

	*PLM4E = 0x51000 + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;

	*PDPE = 0x200000 + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;

	//Création des PDE, pointant chacune sur une PTE
	for(; i < 512; i++)
	{
		PDE[i] = 0x201000 + (i * 0x1000) + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;
	}

	//Mappage des pages principale, ça crée en même temps les PTE
	for (i=0; i<0x900; i++)
	{
		PTE[i] = (i * 0x1000) + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;
	}
}

// Fonction FindDevice qui se charge de détecter quel disque est actuellement utilisé par Logram
void FindDevice () {
	int i; // Itérateur

	// Tester les ports IDE
	for (i = 0; i < 4; i++) {
		int portControl;		// Port de contrôle du contrôleur
		unsigned char a = 0, b = 0;	// Octets utilisés pour lire des registres

		// Déterminer quel port sera utilisé pour cette itération
		if (i == 0) { port = 0x1F0; portControl = 0x3F0; }
		if (i == 1) { port = 0x170; portControl = 0x370; }
		if (i == 2) { port = 0xF0; portControl = 0x2F0; }
		if (i == 3) { port = 0x70; portControl = 0x270; }

		// Regarde si le contrôleur est présent
		char byte1 = inb (port + 6);
		char byte2 = (byte1 & 0x10) >> 4;
		if(byte2 == 0) {
			byte1 |= 0x10;
			outb(port + 6, byte1);
		} else {
			byte1 = byte1 & 0xEF;
			outb(port + 6, byte1);
		}
		byte1 = inb(port + 6);
		byte1 = (byte1 & 0x10) >> 4;
		if(byte1 != byte2) { // Le contrôleur est présent, on peut tester ses éventuels disques
			// Initialiser le contrôleur
			outb (portControl + 6, 4);
			iowait ();
			outb (portControl + 6, 0);

			// ----------------- Disque Maître -----------------

			// Sélectionne le disque
		  	outb(port + ATA_DRIVE, ATA_D_IBM | ATA_D_MASTER);
		  	udelay (100); // Attend la fin de l'opération

			// Y détecter le protocole
			a = inb (port + 2); // Récupère la valeur du registre Sector Count
			b = inb (port + 3); // Récupère la valeur du registre Sector Number

			if ((a == 1) && (b == 1)) { // Si les registres valent 1, il faut lire les registres Cylinder Low et Cylinder High
				a = inb (port + 4); // Récupère la valeur du registre Cylinder Low
				b = inb (port + 5); // Récupère la valeur du registre Cylinder High

				if ((a == 0) && (b == 0)) { // Protocole ATA détecté
					protocol = ATA_DISK; // On écrit dans la variable le nom du protocole
					master = MASTER_DISK; // Disque maître

					// On lit la MBR et on regarde si Logram est dans la table des partitions
					ReadSector (0, &bufsect);
					// On vérifie si la MBR est valide
					if ((bufsect [510] == 0x55) && (bufsect [511] == 0xAA)) { // MBR valide
						int j;	// Itérateur
						for (j = 0; j < 4; j++) { // Parcourt la table des partitions
							if (bufsect [450 + j * 16] == 0xEF) { // Partition Logram détectée
								return;
							}
						}
					}
				} else if ((a == 0x14) && (b == 0xEB)) { // Protocolé ATAPI détecté
					// On ne fait rien car pas encore géré
					protocol = ATAPI_DISK; // On écrit juste dans la variable le nom du protocole
				}
			}

			// ----------------- Disque Esclave -----------------

			// Sélectionne le disque
		  	outb(port + ATA_DRIVE, ATA_D_IBM | ATA_D_SLAVE);
		  	udelay (100); // Temps de latence

			// Y détecter le protocole
			a = inb (port + 2); // Récupère la valeur du registre Sector Count
			b = inb (port + 3); // Récupère la valeur du registre Sector Number

			if ((a == 1) && (b == 1)) { // Si les registres valent 1, il faut lire les registres Cylinder Low et Cylinder High
				a = inb (port + 4); // Récupère la valeur du registre Cylinder Low
				b = inb (port + 5); // Récupère la valeur du registre Cylinder High

				if ((a == 0) && (b == 0)) { // Protocole ATA détecté
					protocol = ATA_DISK; // On écrit dans la variable le nom du protocole
					master = SLAVE_DISK; // Disque maître

					// On lit la MBR et on regarde si Logram est dans la table des partitions
					ReadSector (0, &bufsect);
					// On vérifie si la MBR est valide
					if ((bufsect [510] == 0x55) && (bufsect [511] == 0xAA)) { // MBR valide
						int j;
						for (j = 0; j < 4; j++) { // Parcourt la table des partitions
							if (bufsect [450 + j * 16] == 0xEC) { // Partition Logram détectée
								return;
							}
						}
					}
				} else if ((a == 0x14) && (b == 0xEB)) { // Protocolé ATAPI détecté
					// On ne fait rien car pas encore géré
					protocol = ATAPI_DISK; // On écrit juste dans la variable le nom du protocole
				}
			}
		} else {
			// Contrôleur absent, passer au prochain contrôleur
		}
	}

	// On ne sait pas d'où on est en train de booter
	Quit ();
}

// Fonction LoadFSLInfos qui se charge de charger les informations de la partition FSL (espace allant des secteurs 63 à 2048)
void LoadFSLInfos () {
	ReadSector (logramSect + 63, (void *) &disk);
}

// Fonction LoadKernel qui se charge d'ouvrir le fichier kernel.ext et de le charger en mémoire à l'adresse 0x800000
void LoadKernel () {
	int64 block;

	// Ouvre le fichier kernel.ext
	block = FSL_Open (0, L"Logram"); // Ouvre le dossier /Logram/
	if (block == 0) { // Si il n'est pas trouvé, renvoyer une erreur
		print ("Can't find folder /Logram/. Logram must shutdown");
		Quit ();
	}
	block = FSL_Open (block, L"sys64");  // Ouvre le dossier /Logram/sys64/
	if (block == 0) { // Si il n'est pas trouvé, renvoyer une erreur
		print ("Can't find folder /Logram/sys64/. Logram must shutdown");
		Quit ();
	}
	block = FSL_Open (block, L"kernel.ext");  // Ouvre le dossier /Logram/sys64/kernel.ext
	if (block == 0) { // Si il n'est pas trouvé, renvoyer une erreur
		print ("Can't find file /Logram/sys64/kernel.ext. Logram must shutdown");
		Quit ();
	}

	// Et le charge en mémoire à l'adresse 0x800000
	FSL_Read (block, (void *) 0x800000 - 0x20000);
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
	FSL_BLOCK 	*block;	// Pointeur sur le bloc courant
	FSL_FILE	*file;	// Structure du fichier à ouvrir
	int 		i;	// Itérateur
	int64		numBlk;	// Numéro du bloc courant
	int64		nxtBlk; // Numéro du bloc suivant le courant

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
// Paramètres : - int64 baseBlock : bloc de base du fichier à lire
//		- void *buf	  : buffer qui contiendra la lecture des secteurs
void FSL_Read (int64 baseBlock, void *buf) {
	FSL_BLOCK	*block;	// Pointeur sur le bloc courant
	int 		i;	// Itérateur
	int64		numBlk;	// Numéro du bloc courant
	int64		nxtBlk; // Numéro du bloc suivant le courant

	// Le bloc courant est égal à celui passé en paramètres
	numBlk = baseBlock;

	while (1) {
		// Lire l'en-tête du bloc courant
		ReadSector (logramSect + 2048 + numBlk * disk.blocksize, (void *) &bufsect);
		block = (FSL_BLOCK *) &bufsect;

		// Sauvegarde le numéro de bloc suivant
		nxtBlk = block->follow;

		for (i = 1; i < disk.blocksize; i++) { // Pour chaque secteur du bloc courant
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

// Fonction Quit appelée lors d'un erreur et devant fermer Logram
void Quit () {
	// Message d'erreur
	print ("Fatal error");
	// Boucle infinie
	asm ("hlt");
	for (;;);
}

