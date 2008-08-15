/*
 * diskmaker.c
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
	Ce programme copie le contenu du répertoire /disk vers le fichier disk.img, en le formatant auparavant en FSL, et en le rendant bootable.
*/

// Includes de la librairie standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Includes de Logram
#include <types.h>
#include <drivers/fsl_driver.h>

// Defines
#define COLOR(param) printf("\033[%sm", param)		// Change la couleur du texte de la console
#define SECTOR_SIZE 	512				// Taille d'un secteur
#define MAXSTRING	256				// Taille maximum d'une chaîne
#define SIZE_BLOCK	32				// Taille d'un bloc en secteur

// Prototypes
void 	WriteSector 	(int64 numsect, void *buf); 				// Ecrit un secteur dans disk.img
void 	CopyStages 	();							// Copie stage1 et stage2 dans disk.img
int64 	CopyDisk 	(unsigned char *dirname);				// Copie le contenu du dossier disk/ dans disk.img
void 	CopyDiskInfo 	();							// Remplit les secteurs destinés à accueillir des informations sur le disque
int64	CopyFile	(unsigned char *dirname, unsigned char *filename);	// Copie un fichier dans disk.img
void 	CharToWchar 	(unsigned char *in, lchar *out);			// Convertir un chaîne en chaîne unicode
void 	CopyBootloader 	();							// Copie le Bootloader dans disk.img

// Variables globales
FILE 	*img 		= NULL; // Structure du fichier disk.img
int	numtabs 	= 0;	// Le nombre de tabulations à insérer, c'est juste pour améliorer la présentation quand on affiche les fichiers copiés.
int64 	fsl_offset 	= 0;	// Position sur la partition du dernier bloc enregistré
int64	logramPartition	= 64;	// Position de Logram sur le disque (en secteurs)

// Fonction main, point d'entrée du programme
// Retour : int : code d'erreur, = 0 si tout s'est bien passé
int main () {
	// Ouvre le fichier de sortie disk.img qui sera l'image du disque de Logram
	img = fopen ("disk.img", "wb");

	// Copier le bootloader
	CopyBootloader ();

	// Copier stage1 et stage2 dans disk.img
	CopyStages ();

	// Copier le contenu du dossier disk/ dans disk.img
	printf ("\nDebut de la copie des fichiers du dossier disk/\n");
	CopyDisk ("disk");
	printf ("Fin de la copie des fichiers du dossier disk/\n\n");

	// Inscrire dans disk.img des informations sur le disque
	CopyDiskInfo ();

	// On ferme le fichier disk.img
	fclose (img);

	// On affiche un message de fin
	printf ("Création de disk.img terminée %d secteurs écrits pour la partition Logram. %d écrits dans disk.img\n", fsl_offset * SIZE_BLOCK + 2048, fsl_offset * SIZE_BLOCK + 2048 + logramPartition);

	return EXIT_SUCCESS; // Pas d'erreur
}

// Fonction CopyBootloader qui copie le bootloader dans disk.img
void CopyBootloader () {
	FILE *bootloader = NULL;	// Structure du fichier bootloader.b
	char bufsect [SECTOR_SIZE]; 	// Buffer qui contiendra un secteur
	int i;				// Itérateur

	 // S'il y a une erreur
	bootloader = fopen ("bootloader/bootloader.b", "rb");
	if (bootloader == NULL) { // S'il y a une erreur
		printf ("bootloader.b introuvable :\n\n");
		exit (EXIT_FAILURE); // Quitter en retournant une erreur
	}
	// Lit son contenu (62 secteurs)
	for (i = 0; i < 62; i++) {
		fread (bufsect, SECTOR_SIZE, 1, bootloader);
		// L'écrit dans disk.img
		WriteSector (i, bufsect);
	}

	// Ferme bootloader
	fclose (bootloader);
}

// Fonction CopyStages qui copie stage1 et stage2 dans disk.img
void CopyStages () {
	FILE *stage = NULL; 		// Structure des fichiers stage1 et stage2
	char bufsect [SECTOR_SIZE]; 	// Buffer qui contiendra un secteur
	int i;				// Itérateur

	// Ouvre stage1
	stage = fopen ("boot/stage1", "rb");
	if (stage == NULL) { // S'il y a une erreur
		printf ("Stage1 introuvable !\n\n");
		exit (EXIT_FAILURE); // Quitter en retournant une erreur
	}
	// Lit son contenu (1 secteur)
	fread (bufsect, SECTOR_SIZE, 1, stage);
	// L'écrit dans disk.img
	WriteSector (logramPartition, bufsect);

	// Ferme stage1
	fclose (stage);

	// Ouvre stage2
	stage = fopen ("boot/stage2", "rb");
	if (stage == NULL) { // S'il y a une erreur
		printf ("Stage2 introuvable !\n\n");
		exit (EXIT_FAILURE); // Quitter en retournant une erreur
	}
	// Lit son contenu (62 secteurs)
	for (i = logramPartition + 1; i < logramPartition + 63; i++) {
		fread (bufsect, SECTOR_SIZE, 1, stage);
		// L'écrit dans disk.img
		WriteSector (i, bufsect);
	}

	// Ferme stage2
	fclose (stage);
}

// Fonction CopyDisk qui se charge de copier le contenu du dossier disk/ dans disk.img
int64 CopyDisk (unsigned char *dirname) {
	FILE *dircontents = NULL;				// Structure du fichier __dircontents du répertoire courant
	unsigned char dircontentsPath [MAXSTRING * 10] = ""; 	// Chaîne qui contient le chemin du fichier __dircontents du répertoire courant
	unsigned char line [MAXSTRING] = "";			// Buffer qui contiendra des lignes de fichiers
	unsigned char filename [MAXSTRING] = "";		// Nom du fichier à copier courant
	int64 numfiles = 0;					// Nombre de fichiers dans le répertoire courant
	int64 numdirs = 0;					// Nombre de dossiers dans le répertoire courant
	int i;							// Itérateur
	int tab;						// Itérateur
	FSL_FILE file;						// Structure du fichier courant
	int64 startblock;					// Adresse du premier bloc
	int64 numblk;						// Nombre de blocs pour ce répertoire
	FSL_BLOCK block;					// Bloc courant

	// Trouver le chemin du fichier __dircontents
	strcpy 	(dircontentsPath, dirname); 			// On copie le nom du répertoire courant dans le nom du fichier __dircontents courant
	strncat	(dircontentsPath, "/__dircontents", 14); 	// On ajoute à cette chaîne, le nom du fichier

	dircontents = fopen (dircontentsPath, "r");
	if (dircontents == NULL) {
		printf("Impossible de trouver %s", dircontentsPath);
		exit (EXIT_FAILURE);
	}

	// On lit la première ligne
        fgets(line, MAXSTRING, dircontents);
        // On y récupère le nombre de fichiers et de répertoires
	sscanf (line, "%d %d ", &numfiles, &numdirs);

	//On copie un nouveau dossier, augmenter la profondeur des tabulations pour une meilleure présentation
	numtabs++;

	startblock = fsl_offset;
	fsl_offset += (numfiles + numdirs) / (SIZE_BLOCK - 1) + 1; // On additionne le nombre de blocs à écrire

	// Copier les fichiers du répertoire courant
	for (i = 0; i < numfiles; i++) {
		// Récupérer le nom du fichier à copier
		fgets (line, MAXSTRING, dircontents);
		sscanf(line, "%s", filename);

		// Tabulations pour la présentation
		for (tab = 0; tab < numtabs; tab++) printf("    ");

		COLOR("33");
                printf("Copie de %s\n", filename);
		COLOR("0");

		// Remplit la structure de fichier
		file.attributes 	= 1;					// Fichier normal
		file.permissions 	= 0;					// On ne peut rien faire, ni lire, ni écrire, ni lister, sauf root, bien entendu (utile pour la sécurité)
		file.useranddisk 	= 0;					// Root, disque 0
		file.driverid 		= 0;					// Disque principal
		file.sizebytes 		= 0;					// Fichier de taille dynamique non calculée, il faut compter les blocs pour trouver la taille du fichier
		file.sizeblocks 	= 0;					// Même chose
		file.startblock 	= (int64) CopyFile(dirname, filename); 	// Copie le fichier en retournant son adresse de bloc
		CharToWchar ("FSL\0", file.filesystem);				// Système de fichiers FSL
		CharToWchar (filename, file.filename);				// Inscrit le nom du fichier

		// Enregistre le fichier dans le répertoire courant
		WriteSector (startblock * SIZE_BLOCK + 1 + i + i / (SIZE_BLOCK - 1) + 2048 + logramPartition, &file);
	}

	// Copier les dossiers du répertoire courant
	for (i = 0; i < numdirs; i++) {
		// Récupérer le nom du dossier à copier
		fgets (line, MAXSTRING, dircontents);
		sscanf(line, "%s ", filename);

		// Copie du nom du dossier dans le path du dircontents
		strcpy(dircontentsPath, dirname);
                strcat(dircontentsPath, "/");
		strcat(dircontentsPath, filename);

		// Tabulations pour la présentation
		for (tab = 0; tab < numtabs; tab++) printf("    ");

		COLOR("34");
                printf("Copie de %s\n", filename);
		COLOR("0");

		// On remplit la structure du fichier
		file.attributes 	= 2;				// Dossier
		file.permissions 	= 0;				// On ne peut rien faire, ni lire, ni écrire, ni lister, sauf root, bien entendu (utile pour la sécurité)
		file.useranddisk 	= 0;				// Root, disque 0
		file.driverid 		= 0;				// Disque principal
		file.sizebytes 		= 0;				// Fichier de taille dynamique non calculée, il faut compter les blocs pour trouver la taille du fichier
		file.sizeblocks 	= 0;				// Même chose
		file.startblock 	= CopyDisk (dircontentsPath); 	// Copie le dossier en retournant son adresse de bloc
		CharToWchar ("FSL\0", file.filesystem);			// Système de fichiers FSL
		CharToWchar (filename, file.filename);			// Inscrit le nom du fichier

		// On écrit l'enregistrement de dossier
		WriteSector (startblock * SIZE_BLOCK + i + numfiles + (numfiles + i) / (SIZE_BLOCK - 1) + 1 + 2048 + logramPartition, &file);

		// Tabulations pour la présentation
		for (tab = 0; tab < numtabs; tab++) printf("    ");

		COLOR("34");
                printf("Fin de la copie de %s\n", filename);
		COLOR("0");
	}

	numblk = (numfiles + numdirs) / (SIZE_BLOCK - 1) + 1; // On calcule le nombre de bloc qui a été écrit
	for (i = 0; i < numblk; i++) { // Et on écrit les en-têtes de blocs
		// On remplit la structure de bloc
		block.bloctype 		= FSL_BLOCKTYPE_DIR;	// Dossier
		block.follow 		= startblock + i + 1;	// Bloc suivant
		block.startoffset 	= 0;			// Pas encore utilisé
		block.endoffset 	= 0;			// Pas encore utilisé
		block.moveable 		= 0;			// Non déplaçable
		block.previouslyblock 	= startblock + i - 1;	// Bloc précédent

		// On écrit le bloc
		WriteSector ((startblock + i) * SIZE_BLOCK + 2048 + logramPartition, (void *) &block);
	}

	// On a fini la copie, remonter les tabulations
	numtabs--;

	// Fermer le fichier __dircontents
	fclose (dircontents);

	return startblock;
}

// Fonction CopyFile qui copie un fichier dans disk.img
// Paramètres : - unsigned char *dirname : nom du répertoire où le fichier devra être copié
//		- unsigned char *filename: nom du fichier qui sera copié
int64 CopyFile (unsigned char *dirname, unsigned char *filename) {
	FILE *file = NULL;			// Structure sur le fichier à copier
	unsigned char filepath [MAXSTRING * 10];// Chaîne qui contien le chemin du fichier
	int64 numsect = 0;			// Numéro de secteur dans le bloc
	char bufsect [SECTOR_SIZE]; 		// Buffer qui contiendra un secteur
	FSL_BLOCK block;			// Bloc courant
	int64 startblock;			// Position du premier bloc

	// On assemble le nom du fichier
	strcpy (filepath, dirname);
	strncat(filepath, "/", 1);
	strncat(filepath, filename, MAXSTRING);

	// On ouvre le fichier
	file = fopen(filepath, "rb");
	if (file == NULL) { // En cas d'erreur
		printf ("Impossible de trouver '%s'\n", filepath);
		exit (EXIT_FAILURE);
	}

	startblock = fsl_offset;

	do {
		if (numsect == 0) {
			// On débute un bloc, on doit enregistrer son en-tête
			block.bloctype 		= FSL_BLOCKTYPE_FILE; 	// Le bloc est un bloc de fichier
			block.follow 		= fsl_offset + 1;	// Bloc suivant
			block.startoffset 	= 0;			// Pas encore implémenté
			block.endoffset 	= 0;			// Pas encore implémenté
			block.moveable 		= 0;			// C'est déjà non-fragmenté, on va surtout pas le déplacer
			block.previouslyblock	= fsl_offset - 1;	// Bloc précédent

			// Ecrire l'en-tête de bloc
			WriteSector (fsl_offset * SIZE_BLOCK + 2048 + logramPartition, (void *) &block);
			numsect++; // Passer au secteur suivant
		}
		
		// Lire un secteur
               	fread ((void *) &bufsect, SECTOR_SIZE, 1, file);
		// Et le copier dans disk.img
		WriteSector (fsl_offset * SIZE_BLOCK + numsect + 2048 + logramPartition, (void *) &bufsect);

		// Passer au secteur suivant
		numsect++;

		if (numsect == SIZE_BLOCK) { // Fin du bloc ?
			// Alors on passe au bloc suivant
			numsect = 0;
			fsl_offset++;
		}
	} while (!feof(file));

	// On réécrit l'en-tête du dernier bloc pour s'assurer que le fichier a une fin
	block.follow = 0; // Signale la fin du fichier
	WriteSector (fsl_offset * SIZE_BLOCK + 2048 + logramPartition, (void *) &block);
	fsl_offset++; // On change de bloc

	// On ferme le fichier
	fclose (file);

	return startblock;
}

// Fonction CopyDiskInfo qui se charge de remplir les secteurs destinés à accueillir des informations sur le disque
void CopyDiskInfo () {
	FSL_DISK diskInfos; // Structure des informations du disque

	// Remplir la structure
	diskInfos.version 	= 1; 					// Version 1 du FSL
	diskInfos.partitionstart= logramPartition;			// Indique le début de la partition Logram
	diskInfos.size		= 3200; 				// Le disque a une taille totale de 3200 blocs
	diskInfos.blocksize	= SIZE_BLOCK;				// Un bloc représente SIZE_BLOCK secteurs
	diskInfos.flags		= 0;					// Pas de flags
	diskInfos.freespace	= diskInfos.size - fsl_offset - 64;	// On calcule l'espace libre
	diskInfos.usedspace	= fsl_offset + 64;			// Espace utilisé
	CharToWchar ("Logram", diskInfos.name);				// Le disque est nommé "Logram"
	CharToWchar ("Logram disk", diskInfos.description);		// Est décrit comme suit : "Logram disk"
	diskInfos.tags [0]	= 0;					// Pas de tag

	WriteSector (logramPartition + 63, (void *) &diskInfos);
}

// Fonction WriteSector qui écrit dans le fichier disk.img un secteur
// Paramètres : - int64 numsect : numéro de secteur
//		- void *buf	: buffer à écrire
void WriteSector (int64 numsect, void *buf) {
	// Se positionner dans le fichier
	fseek 	(img, numsect * SECTOR_SIZE, SEEK_SET);
	// Et écrire le contenu du buffer
	fwrite 	(buf, SECTOR_SIZE, 1, img);
}

// Fonction CharToWchar qui convertit une chaîne en chaîne unicode
// Paramètres : - unsigned char *in 	: chaîne d'entrée
//		- lchar *out		: chaîne de sortie en unicode
void CharToWchar (unsigned char *in, lchar *out) {
	int i = 0; // Itérateur

	while (in [i]) { // Tant qu'on a pas atteint la fin de la chaîne
		// Copier le contenu dans la chaîne out
		out [i + 1] = 0;
		out [i] = (short) in [i];
		i++;
	}
	out [i] = 0; // Déterminer la fin de la chaîne
}
