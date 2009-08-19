/*
 * ps2core.c
 * This file is part of Logram
 *
 * Copyright (C) 2008 - royalbru
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
	Fichier principal du pilote ps2.ext.
	Ce pilote est chargé de contrôler le protocole PS/2
*/

// Inclusions
#include <types.h>
#include <logram.h>
#include <driver.h>
#include <drivers/ps2_driver.h>
#include <asm.h>

#include "ps2.h"

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
		{&SendToCmd, 	L"SendToCmd"},
		{&SendToData, 	L"SendToData"},
		{&ReadFromCmd, 	L"ReadFromCmd"},
		{&ReadFromData, L"ReadFromData"},
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
		1,
		{ L'P', L'S', L'/', L'2', 0, 0, 0, 0 }
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

// Fonction qui permet l'envoi d'une commande sur le port PS/2
// Paramètres : - unsigned char cmd : commande à envoyer
char SendToCmd (unsigned char cmd) {
	int i = 0;
	char in = inb (PS2_COMMAND);

	// On vérifie si on peut envoyer le message
	while (in & 2) {
		in = inb (PS2_COMMAND);
		i++;
		if (i == 65535) { // Erreur, délai d'attente dépassé
			return PS2_ERROR;
		}
	}

	// On envoie le message
	outb (PS2_COMMAND, cmd);

	// On vérifie s'il a bien été envoyé
	i = 0;
	while (in & 2) {
		in = inb (PS2_COMMAND);
		i++;
		if (i == 65535) { // Erreur, délai d'attente dépassé
			return PS2_ERROR;
		}
	}
	
	return PS2_SUCCESS; // Tout s'est bien passé
}

// Fonction qui permet la lecture des paquets envoyés par le port PS/2 sur le port de data
// Paramètres : - unsigned char *buf : octet qui contiendra la valeur envoyée par le port
char ReadFromData (unsigned char *buf) {
	int i = 0;
	char in = inb (PS2_COMMAND);

	// On vérifie si on peut lire
	while (!(in & 1)) {
		in = inb (PS2_COMMAND);
		i++;
		if (i == 65535) { // Erreur, délai d'attente dépassé
			return PS2_ERROR;
		}
	}

	// On lit le packet et on le stock dans le buffer
	*buf = inb (PS2_DATA);

	return PS2_SUCCESS; // Tout s'est bien passé
}

// Fonction qui envoie un packet de données sur le port PS/2
// Paramètres : - unsigned char data : octet qui contient la valeur à envoyer au port
void SendToData (unsigned char data) {
	outb (PS2_DATA, data);
}

// Fonction qui permet la lecture des paquets envoyés par le port PS/2 sur le port de commande
// Paramètres : - unsigned char *buf : octet qui contiendra la valeur envoyée par le port
void ReadFromCmd (unsigned char *buf) {
	*buf = inb (PS2_COMMAND);
}

