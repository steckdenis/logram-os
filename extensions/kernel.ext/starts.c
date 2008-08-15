/*
 * starts.c
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
	Fichier de démarrage de kernel.ext
		Contient la fonction de démarrage, et elle seule
*******************************************************************************/

#include	<types.h>	//Types génériques de Logram
#include	<logram.h>	//En-tête générique

#include	"console.h"
#include	"mem.h"
#include	"drivers.h"
#include 	<drivers/fsl_driver.h>		/*Requis par files.h */
#include 	"file.h"

/* Déclaration des fonctions (ExtMain déclarée dans logram.h) */
int 	CompareString	(lchar *s1, lchar *s2);
void	start		();
void 	InitKernel 	();
void 	Test 		();
void	MakeGDT 	();
int 	SplitString	(lchar *str, lchar separator);
int 	SplitNext	(lchar *str, lchar **out);

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
		{ &start, L"BootKernel" },
		{ &ExtMain, L"ExtMain" },

		{ &kprintf, L"kprintf"},			// Utilisé au démarrage
		{ &kstate, L"kstate"}, 

		{ &memcopy, L"memcopy"},
		{ &CompareString, L"CompareString"},
		{ &SplitString, L"SplitString"},
		{ &SplitNext, L"SplitNext"},
		{ &VirtualAlloc, L"VirtualAlloc"},
		{ &OpenFile, L"OpenFile"},
		{ &ReadFile, L"ReadFile"},
		
		{ &FindDriver, L"FindDriver"},
		{ &FindDriverId, L"FindDriverId"},
		{ 0, 0}
	};

section sections[] =
	{
		{ 0, 0x1000, SECTION_DATA },			//Données
		{ (void *) 0x1000, 0x1000, SECTION_CODE },	//Code
	};

// Variables externes de boot.c
extern unsigned short 	port;
extern char 		master;
extern char 		protocol;
extern int64		logramSect;

/* Code du fichier */

void	start() {
	//Fonction appellée par Stage2 au démarrage.

	// Récupère les valeurs des registres envoyées par stage2
	__asm(	"cli\n"
		"mov %%eax, %0\n"
		"mov %%bx, %1\n"
		"mov %%ch, %2\n"
		"mov %%cl, %3\n"
		: "=m"(logramSect), "=m"(port), "=m"(master), "=m"(protocol));

	__asm(	"mov %ds, %ax\n"
		"mov %ax, %es\n"
		"mov $0x60000, %rax\n"
		"mov %rax, %rsp");


	// Affiche le message pour certifier que nous avons chargé le kernel
	kprintf ("", 0x07);
	kprintf ("Kernel loaded !", 0x02);
	kprintf ("", 0x07);

	InitKernel (); 	// Initialise le kernel

	Test (); 	// Test le noyau

	// Boucle infinie en attendans les applis
	asm ("hlt");
	for (;;);
	
}
