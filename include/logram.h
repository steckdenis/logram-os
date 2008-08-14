/*
 * logram.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Denis Steckelmacher
 *
 * Logram is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
	En-tête générique de Logram, pour les extensions et les applications
*******************************************************************************/

/* Structures descriptives des extensions et applications */

typedef struct {
	void	*baseAddr;	//Adresse de base de la section dans le fichier
	lint	size;		//Taille de la section en octets (aligné sur 4ko)
	int	flags;		//Flags de la section
} section;

typedef struct {
	void	*address;	//Adresse de l'objet exporté
	lchar	*name;		//Nom de l'objet
} exports;

/* Description d'une extension */
typedef struct {
	int	size;		//Taille de la structure
	lint	magicCode;	//Valeur magique pour les extensions
	exports *exports;	//Tableau de fonctions exportées
	section *sections;	//Sections du fichier
} resext;

/* Description d'une application */
typedef struct {
	int	size;		//Taille de la structure
	lint	magicCode;	//Valeur magique pour les applications
	void	*icon;		//Adresse de l'icône qui représente l'application (comme dans Windows)
	section	*sections;	//Sections de l'application
} resexe;

/* Constantes pour les extensions */

#define		MAGIC_EXT	0x4F56A2E4DE89AF0C //Code pris au hasard sans signification

#define		EXT_LOAD	1
#define		EXT_ATTACH	2
#define		EXT_DETACH	3
#define		EXT_CLOSE	4

#define		SECTION_CODE	1
#define		SECTION_DATA	2
#define		SECTION_RODATA	3
#define		SECTION_BSS	4
#define		SECTION_STACK	5

/* Constantes pour la gestion de la mémoire */

#define MEM_NOTCACHEABLE	0x00000001
#define MEM_WRITETHROUGH	0x00000002
#define MEM_NX			0x00000004
#define MEM_READONLY		0x00000008
#define MEM_PROTECTED		0x00000010
#define MEM_NOTPAGEABLE		0x00000020
#define MEM_PUBLIC		0x00000040
#define MEM_PRIVATE		0x00000080
#define MEM_OUTPHYSICAL		0x00000100
#define MEM_PHYSICALADDR	0x00000200

/* Macros */

//Calcul de l'adresse d'une variable globale pour usage externe (affichage, import, export, etc)
//	-__ext = l'adresse de base de l'extension (ext en général)
//	-__chaine = l'adresse de la variable à calculer, comme L"teste" ou &foo
#define STRING(__ext, __chaine) (void *)(((int64)__chaine)+(int64)(__ext))

/* Prototypes */

/* ExtMain : fonction d'entrée d'une extension
	*ext : adresse à laquelle a été chargée l'extension
	message : message envoyé (EXT_xxx)
	param : paramètre (éventuelement passé par l'application hôte) */
int	ExtMain(void *ext, void *kernAddr, void *efAddr, lint message, lint param);
