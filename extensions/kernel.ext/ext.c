/*
 * ext.c
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
	Fichier de gestion des extensions
		Contient les fonctions de gestion des extensions
*******************************************************************************/

#include	<types.h>
#include	<logram.h>

#include	"mem.h"
#include	"ext.h"

int	CompareString(lchar *s1, lchar *s2);

// Fonction ExtFind qui retourne l'adresse d'une fonction appartenant à une extension
// Paramètres : - void *ext 	: extension à explorer
//		- lchar *nom 	: nom de la fonction à exporter
void	*ExtFind(void *ext, lchar *nom)
{
	resext *header;
	exports *exps;
	int i = 0;

	header = (resext *) ext;
	
	exps = (exports *) header->exports;
	if (exps < (exports *) 0x800000)
	{
		//La fonction est en base 0
		exps = (exports *)(((int64) exps)+((int64) ext));
	}

	//explorer les fonctions exportées
	while (exps[i].address)
	{
		if (exps[i].name < (lchar *) 0x800000)
		{
			exps[i].name = (lchar *)(((int64) exps[i].name)+((int64) ext));
		}
		if (CompareString(exps[i].name, nom))
		{
			//On vérifie si l'extension exporte en adresse relative (par rapport à son adresse) ou réel
			if (exps[i].address < (void *) 0x800000)
			{
				return (void *)(((int64) exps[i].address)+((int64) ext));
			}
			else
			{
				return exps[i].address;
			}
		}
		i++;
	}

	//La fonction n'existe pas
	return (void *) 0;
}
