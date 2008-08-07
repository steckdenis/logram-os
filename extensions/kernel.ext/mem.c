/*
 * mem.c
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
	Ce fichier contient tout ce qui est relatif à la gestion de la mémoire (allocation, free, mappage, etc).
*/

// Inclusions
#include <types.h>

#include "mem.h"
#include "processes.h"

// Fonction memcopy qui copie une plage mémoire à un autre endroit
// Paramètres : - char *dst 	: adresse où sera copiée la mémoire
//		- char *src 	: adresse de base de la mémoire à copier
//		- int n		: octets à copier
void memcopy (char *dst, char *src, int n)
{
	while (n--)
		*dst++ = *src++;	
	return;
}

// Fonction VirtualAlloc qui permet d'allouer des pages en mémoire (1 page = 4096 octets)
// Paramètres : - int64 _physAddr 	: adresse physique de la page si on souhaite la spécifier. Mettre à 0 lorsque la fonction doit la choisir. Est une adresse si il y a le flag MEM_OUTPHYSICAL
//		- unsigned int pagesNb 	: nombre de pages à allouer
//		- int mflags 		: flags de la fonction
//		- int pid 		: id du processus qui mappe la page (1 si kernel)
void *VirtualAlloc (int64 physicaladdr, unsigned int pagesNb, int mflags, int pid) {
	int64	physaddr, logaddr, flags, i, _logaddr, c, contpages;
	int64	*globalpages = (int64 *) 0x201000;
	int16	*rampages = (int16 *) 0x400000;	

	flags = PAGE_PRESENT;

	if (mflags & MEM_NOTCACHEABLE) flags += PAGE_CACHEDISABLE;
	if (mflags & MEM_WRITETHROUGH) flags += PAGE_WRITETHROUGH;
	if (mflags & MEM_NX) flags += PAGE_NX;
	if (!(mflags & MEM_READONLY)) flags += PAGE_WRITE;
	if (!(mflags & MEM_PROTECTED)) flags += PAGE_USER; 
	if (mflags & MEM_NOTPAGEABLE) pid = 1;		//pid des pages qu'on ne peut vider dans le swap

	if (mflags & MEM_PUBLIC) {
		//On veut allouer une page globale
		
		//Nous sommes dans l'espace publique, il suffit de trouver un enregistrement à 0
		i = 0x902;	//On saute les premières pages utilisées par le noyau, ne pas modifier.
		contpages = 0;	//Compteur de pages contigues
		while (1)
		{	
			//On vérifie si la page est à 0
			if (!globalpages[i])
			{
				//Si oui, on a trouvé une page contigue
				contpages++;
				
				//Si on a suffisamment de pages, on quitte
				if (contpages == pagesNb) break;
			}
			else
			{
				//Elle est remplie, on brise le bloc
				contpages = 0;
			}
			
			i++;
			
		}

		//La première page libre est le numéro courrant de pages (i) - le nombre de pages + 1
		logaddr = (i-pagesNb+1)<<12;
		
		//Allouer les pages physiques
		for (c=0;c<pagesNb;c++)
		{
			if (mflags & MEM_PHYSICALADDR) 
			{
				//L'adresse physique voulue est donnée
				physaddr = physicaladdr+(c*4096);
			} 
			else 
			{
				//Il va falloir en trouver une
				//Il suffit d'explorer les pages physiques.
				i = 0x902;
				while (rampages[i]) 
				{
					//Passer à la page suivante
					i++;
					//On est peut-être à la fin de la RAM, on avait vérifié la dernière page
					if (rampages[i] == 0xFFFF) 
					{
						//Décharger une page
					}
				}
	
				physaddr = i<<12;
			}
			//Mapper les deux adresses.
			globalpages[(logaddr>>12)+c] = physaddr | flags | PAGE_GLOBAL;
			//On enregistre la page en RAM
			rampages[physaddr>>12] = pid;
		}
	}

	if (mflags & MEM_OUTPHYSICAL) {
		//On renvoie l'adresse de la dernière page trouvée, car quand on demande l'adresse d'une page, c'est qu'on en a alloué qu'une seule.
		int64 *addr = (int64 *) physaddr;
		*addr = physaddr;
	}

	return (void *) logaddr;
}

// Fonction qui permet la création d'un enregistrement en GDT
// Paramètres : - int index 	: numéro d'indexation dans la table
//		- int16 flags 	: flags de l'enregistrement
void CreateSegment (int index, int16 flags) {
	gdtrec *enrg;

	enrg = (gdtrec *) 0x30000;

	enrg[index].limit 	= 0;
	enrg[index].base0_15 	= 0;
	enrg[index].base16_23 	= 0;
	enrg[index].flags 	= flags;
	enrg[index].base24_31 	= 0;
}

// Fonction qui permet la création d'un enregistrement système en GDT
// Paramètres : - int index 		: numéro d'indexation dans la table
//		- int16 baseAddress 	: adresse de base du segment
//		- int16 limit		: limite du segment
//		- int16 flags 		: flags de l'enregistrement
void CreateSysSegment(int index, int64 baseAddress, int16 limit, int16 flags) {
	gdtsysrec *enrg;
	int64 mindex = (int64) index;

	enrg = (gdtsysrec *)(0x30000+(mindex*8));

	enrg->limit 		= limit;	
	enrg->base0_15 		= (int16)baseAddress;
	enrg->base16_23 	= (int8)(baseAddress>>16);
	enrg->flags 		= flags;
	enrg->base24_31 	= (int8)(baseAddress>>24);
	enrg->base32_63 	= (int32)(baseAddress>>32);
	enrg->zeros 		= 0;
}

// Fonction qui lit les informations d'un segment dans la GDT et les place dans une structure sysrec
//	-index : n° de segment
//	-*out:	adresse d'une structure sysrec pour réceptionner les données
void	ReadSegment		(int index, gdtsysrec *out)
{
	gdtsysrec	*gdt = (gdtsysrec *) 0x30000;

	index = index>>3;	//On reprend le sélecteur de base, sans les attributs mis dans les bits de poids faible

	out->limit = gdt[index].limit;
	out->base0_15 = gdt[index].base0_15;
	out->base16_23 = gdt[index].base16_23;
	out->flags = gdt[index].flags;
	out->base24_31 = gdt[index].base24_31;
	out->base32_63 = gdt[index].base32_63;
}

//Fonction qui retourne l'adresse de base d'une TSS en fonction du segment
//	-desc = contenu du registre TR de la TSS à trouver
void	*GetTSSBaseAddr(int16 desc)
{
	gdtsysrec 	*enrg;
	int64		addr, b015, b1623, b2431, b3263;
	
	desc = desc & 0xFFF8;	//Mettre à 0 les attributs (DPL et LDT)
	enrg = (gdtsysrec *)(0x30000 + (int64) desc);
	
	b015 = enrg->base0_15;
	b1623 = enrg->base16_23;
	b2431 = enrg->base24_31;
	b3263 = enrg->base32_63;
	
	addr = b015 | (b1623 << 16) | (b2431 << 24) | (b3263 << 32);
	
	return (void *) addr;
}

// Fonction VirtualFree qui désalloue des pages précédemment allouées
// Paramètres : - int64 virtAddr 	: adresse virtuelle de la première page
//		- unsigned int pagesNb 	: nombre de page à libérer
//		- char publicMem	: mémoire publique ou privée (1 pour publique, 0 pour privée)
void VirtualFree (int64 virtAddr, unsigned int pagesNb, char publicMem) {
	if (publicMem) { // Si c'est de la mémoire publique
	
	} else { // Si c'est de la mémoire privée
	
	}
}

// Fonction qui crée la structure de pagination d'un processus. Ne doit être appelée qu'une seule fois.
// Paramètres : - int flags 		: flags de la structure de pagination
//		- Process *process 	: structure de processus qui donnera des informations utiles à la création de la structure
void CreatePagingStructure (int mflags, Process *process) {
	int 	flags;	// Flags de la structure de pagination
	int64 	pml4t;	// Adresse de la table des PML4E
	
	// Remplissons les flags
	flags = PAGE_PRESENT; // La page est dans la RAM
	if (mflags & MEM_NOTCACHEABLE) 	flags |= PAGE_CACHEDISABLE; 	// La page ne peut pas être copiée dans le cache
	if (mflags & MEM_WRITETHROUGH) 	flags |= PAGE_WRITETHROUGH; 	// La page est en WRITE_THROUGH
	if (mflags & MEM_NX) 		flags |= PAGE_NX;		// La page ne peut pas être exécutée
	if (!(mflags & MEM_READONLY)) 	flags |= PAGE_WRITE;		// La page peut être écrite
	if (!(mflags & MEM_PROTECTED)) 	flags |= PAGE_USER;		// La page peut être accessible aux applications
	
	// Crée le PML4E
}

