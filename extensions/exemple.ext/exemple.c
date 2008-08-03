/*
 * exemple.c
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
 
/*******************************************************************************
	Exemple d'extension.
		Cet exemple décrit le format d'une extension, ainsi que la 
		méthode de codage à utiliser dans Logram.
*******************************************************************************/

#include	<types.h>	//Types génériques de Logram
#include	<logram.h>	//En-tête générique

/* Déclaration des fonctions (ExtMain déclarée dans logram.h) */
int	TesteFunc(int foo);

/* En-tête du fichier */
exports exps[];
section sections[];

resext head = 
	{
		sizeof(resext),
		MAGIC_EXT,
		exps,
		sections 
	};

exports exps[] =
	{
		{ &ExtMain, L"ExtMain" },
		{ &TesteFunc, L"TesteFunc" }
	};

section sections[] =
	{
		{ 0, 0x1000, SECTION_DATA },			//Données
		{ (void *) 0x1000, 0x1000, SECTION_CODE },	//Code
		{ 0, 0x1000, SECTION_STACK}			//Taille de la pile (adresse ignorée)
	};

/* Code du fichier */

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

int	TesteFunc(int foo)
{
	return (foo*2);
}
