/*
 * fslcore.c
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
	Fichier principal du pilote fsl.ext.
	Ce pilote permet l'utilisation du système de fichiers FSL.
*/

// Inclusions
#include <types.h>
#include <logram.h>
#include <driver.h>
#include <asm.h>
#include <drivers/fsl_driver.h>

// Variables globales
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
	
// Fonction principale de l'extension
int ExtMain (void *ext, void *kernAddr, void *efAddr, lint message, lint param)
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
	return 1;
}

