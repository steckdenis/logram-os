/*
 * file.c
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Denis Steckelmacher
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
	Gestion des fichiers par le noyau
*/

#include <types.h>
#include <drivers/fsl_driver.h>

#include "drivers.h"
#include "file.h"

int SplitString(lchar *str, lchar separator);
int SplitNext(lchar *str, lchar **out);

int	OpenFile(lchar	*mpath, FILE *outfile)
{
	void 	*device;	//Pilote de système de fichiers utilisé
	int	(*D_Open)(FSL_FILE *file, lchar *nom);
	
	//Initialiser file
	outfile->filesystem[0] = L'F';
	outfile->filesystem[1] = L'S';
	outfile->filesystem[2] = L'L';
	outfile->filesystem[3] = 0;
	outfile->userandpart = 0;
	outfile->disk = 0;
	outfile->startblock = 0;
	
	SplitString(mpath, L'\\');
	
	while (1)
	{
		//Ouvrir et importer les fonctions du bon pilote de système de fichier
		device = FindDriver(outfile->filesystem);
		D_Open = (void *) ExtFind(device, L"OpenFile");
		
		//Et on ouvre bêtement le fichier
		if (!D_Open(outfile, mpath)) return 0;	//erreur
			
		//Vérifie si c'est un fichier
		if (outfile->attributes == 1) return 1;	//Réussi
			
		//On passe au fichier suivant
		SplitNext(mpath, &mpath);
	}
}

int	ReadFile(FILE *file, int64 start, int64 num, void *buf)
{
	void 	*device;
	int	(*D_Read)(FILE *file, int64 start, int64 count, void *buf);
	
	//On trouve et importe ce qu'il faut du pilote
	device = FindDriver(file->filesystem);
	D_Read = (void *) ExtFind(device, L"ReadFile");
	
	//On lit bêtement le fichier
	return (D_Read(file, start, num, buf));
}
