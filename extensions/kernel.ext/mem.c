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

void zerofill(char *addr, int n)
{
	while (n--)
		*addr++ = 0;
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
		//Nous sommes dans l'espace publique, il suffit de trouver un enregistrement à 0
		i = 0x900;	//On saute les premières pages utilisées par le noyau, ne pas modifier.
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
				i = 0x900;
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
	} else {
		//On alloue une page en mémoire privée
		int64 pPLM4E, nplm4e, npdpe, npde, npte, totpages;
		int64 *plm4e;
		int64 *pdpe;
		int64 *pde;
		int64 *pte;
		int64 plm4ef, pdpef, pdef, ptef;
		
		totpages = 0;
		
		//Récupérer l'adresse de la PLM4E
		asm(	"mov %%cr3, %0"
		: "=a" (pPLM4E));
		pPLM4E = pPLM4E & 0xFFFFFFFFFFFFF000;	//Effacer les bits réservés
		plm4ef = pPLM4E;
		
		//On a une adresse physique, il faut la mapper pour l'utiliser
		plm4e = (int64 *) VirtualAlloc(plm4ef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
		
		//Explorer les tables jusqu'à trouver assez de pages libres
		for (nplm4e = 0; nplm4e < 512; nplm4e++)
		{
			pdpef = plm4e[nplm4e];
			if (!pdpef)
			{
				pdpe = (int64 *) VirtualAlloc((int64) &pdpef, 1, MEM_PUBLIC | MEM_OUTPHYSICAL, 1);	//Créer une nouvelle pdpe
				*pdpe = 0;
				zerofill((char *) pdpe, 4096);
				plm4e[nplm4e] = pdpef + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;
			}
			else
			{
				pdpe = (int64 *) VirtualAlloc(pdpef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
			}
			
			for (npdpe = 0; npdpe < 512; npdpe++)
			{
				if ((nplm4e == 0) && (npdpe == 0))
				{
					//On va prendre la 1ere PDE dans la première PDPE, ça ne va pas (elle est utilisée pour la mémoire publique)
					break;
				}
				
				pdef = pdpe[npdpe];
				if (!pdef)
				{
					pde = (int64 *) VirtualAlloc((int64) &pdef, 1, MEM_PUBLIC | MEM_OUTPHYSICAL, 1);	//Créer une nouvelle pdpe
					zerofill((char *) pde, 4096);
					pdpe[npdpe] = pdef + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;
				}
				else
				{
					pde = (int64 *) VirtualAlloc(pdef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
				}
				
				for (npde = 0; npde < 512; npde++)
				{
					ptef = pde[npde];
					if (!ptef)
					{
						pte = (int64 *) VirtualAlloc((int64) &ptef, 1, MEM_PUBLIC | MEM_OUTPHYSICAL, 1);	//Créer une nouvelle pdpe
						zerofill((char *) pte, 4096);
						pde[npde] = ptef + PAGE_PRESENT + PAGE_WRITE + PAGE_USER;
					}
					else
					{
						pte = (int64 *) VirtualAlloc(ptef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
					}
					
					for (npte = 0; npte < 512; npte++)
					{
						if (!pte[npte])
						{
							//La page est libre, on incrémente le nombre de pages conigues
							totpages++;
							if (totpages == 1)
							{
								logaddr = ((nplm4e << 27) | (npdpe << 18) | (npde << 9) | npte); //logaddr n'est pas la vraie adresse, il faut encore faire un << 12
							}
							if (totpages == pagesNb) goto pages_ok;
						}
						else
						{
							totpages = 0;
						}
					}
					//On n'a plus besoin de la PTE
					VirtualFree((int64) pte, 1, MEM_DONTFREEPHYSICAL);
				}
				//On n'a plus besoin de la PDE
				VirtualFree((int64) pde, 1, MEM_DONTFREEPHYSICAL);
			}
			//On n'a plus besoin de la PDPE
			VirtualFree((int64) pdpe, 1, MEM_DONTFREEPHYSICAL);
		} 
pages_ok:	
		//Réellement mapper les pages (cette fois-ci, ce sera plus court ;-) )
		for (i=0; i<pagesNb; i++)
		{
			//Découper l'adresse virtuelle en numéros de PLM4E, PDPE, PDE et PTE
			_logaddr = logaddr+i;
			npte = _logaddr & 0x1FF;
			npde = (_logaddr >> 9) & 0x1FF;
			npdpe = (_logaddr >> 18) & 0x1FF;
			nplm4e = (_logaddr >> 27) & 0x1FF;
			
			pdpef = plm4e[nplm4e];
			pdpe = (int64 *) VirtualAlloc(pdpef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
			
			pdef = pdpe[npdpe];
			pde = (int64 *) VirtualAlloc(pdef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
			
			ptef = pde[npde];
			pte = (int64 *) VirtualAlloc(ptef, 1, MEM_PHYSICALADDR | MEM_PUBLIC, 1);
			
			//On a la pte, y placer au bon endroit l'adresse physique de la page
			if (mflags & MEM_PHYSICALADDR) 
			{
				//L'adresse physique voulue est donnée
				physaddr = physicaladdr+(i*4096);
			} 
			else 
			{
				//Il va falloir en trouver une
				//Il suffit d'explorer les pages physiques.
				i = 0x900;
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
			
			pte[npte] = (int64) (physaddr | flags);	//La page est mappée
			rampages[physaddr>>12] = pid;
		}
		
		//On a fini, on adapte juste logaddr et physaddr
		logaddr <<= 12;
	}

	if (mflags & MEM_OUTPHYSICAL) {
		//On renvoie l'adresse de la dernière page trouvée, car quand on demande l'adresse d'une page, c'est qu'on en a alloué qu'une seule.
		int64 *addr = (int64 *) physicaladdr;
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
void ReadSegment(int index, gdtsysrec *out)
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
void VirtualFree (int64 virtAddr, unsigned int pagesNb, int mflags) {
	int64	*globalpages 	= (int64 *) 0x201000;	// PTE de la mémoire publique
	int16	*rampages 	= (int16 *) 0x400000;	// Table des enregistrements de pages RAM
	int 	i;					// Itérateur

	if (mflags & MEM_PUBLIC) { // Si c'est de la mémoire publique
		for (i = 0; i < pagesNb; i++)
		{
			globalpages [((virtAddr >> 12) & 0x3FFFF) + i] = 0;
			if (!(mflags & MEM_DONTFREEPHYSICAL))
			{
				rampages [((virtAddr >> 12) & 0x3FFFF) + i] = 0;
			}
		}
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

