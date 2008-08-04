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
int64 VirtualAlloc (int64 _physAddr, unsigned int pagesNb, int mflags, int pid) {
	int flags;				// Flags de la page à mapper
	int64 outVirt = 0;			// Adresse virtuelle de la première page
	int64 outPhys = 0;			// Adresse physique de la première page
	int i;					// Itérateur
	int16 *ramPages = (int16 *) 0x400000;	// Adresse de la table des pages RAM
	int64 ramRecord = 0;			// Numéro d'enregistrement dans la table des pages RAM

	// Remplissons les flags
	flags = PAGE_PRESENT; // La page est dans la RAM
	if (mflags & MEM_NOTCACHEABLE) 	flags |= PAGE_CACHEDISABLE; 	// La page ne peut pas être copiée dans le cache
	if (mflags & MEM_WRITETHROUGH) 	flags |= PAGE_WRITETHROUGH; 	// La page est en WRITE_THROUGH
	if (mflags & MEM_NX) 		flags |= PAGE_NX;		// La page ne peut pas être exécutée
	if (!(mflags & MEM_READONLY)) 	flags |= PAGE_WRITE;		// La page peut être écrite
	if (!(mflags & MEM_PROTECTED)) 	flags |= PAGE_USER;		// La page peut être accessible aux applications
	if (mflags & MEM_NOTPAGEABLE) 	pid = 1;			// La page ne peut être déplacée dans le swap

	// Recherchons l'adresse physique contigüe de nos pages à allouer
	for (i = 0; ramPages [i] != 0xFFFF; i++) { // Tant que nous avons pas atteint la fin de la ram, chercher une page libre
		if (ramPages [i] == 0) { // Page libre ?
			int j; // Itérateur

			for (j = 1; j < pagesNb; j++) { // Regardons s'il y a la place pour les pages suivantes
				if (ramPages [i + j] != 0) { // Notre page suivante n'est pas libre
					break; // Continuer la recherche
				}
			}
			if (j == pagesNb) { // On a trouvé une zone de mémoire libre pour toutes nos pages d'affilées
				break; // Quittons l'itération
			}
		}
	}
	if (ramPages [i] == 0xFFFF) { // On a atteint la fin de la Ram
		return 0; // Retourner 0
	} else { // On a bien notre enregistrement
		ramRecord = i; // Sauvegarder son numéro
	}
	// Allouons
	if (mflags & MEM_PUBLIC) { 		// On veut allouer des pages publiques
		for (i = 0; i < pagesNb; i++) { // On alloue les pages
			int64 *PDE = (int64 *) 0x52000; // PDE
			int64 *PTE = 0;			// PTE
			int j;				// Itérateur
			int64 used;			// PTE utilisés dans le PDE
			int64 tmp;			// Temporaire
			int64 physAddr = 0;		// Adresse physique de la page courante
			int64 virtAddr;		// Adresse virtuelle de la page courante

			// ----------------------------- PDE -----------------------------
			
			for (j = 0; j < 512; j++) { // Cherche un PDE qui contient encore de l'espace
				if (PDE [j] & PAGE_NOTFULL) { // L'a t-on trouvé ?
					break; // Oui, on quitte la boucle
				}
			}
			if (j == 512) { // Si on a pas trouvé de PDE libre, on retourne 0
				return 0;
			}
			// On écrit le numéro de notre PDE dans l'adresse virtuelle
			virtAddr = j << 21;

			// On incrémente le nombre de pages utilisées dans le PDE
			used = (PDE [j] & 0x7FF0000000000000) >> 52;
			used++;
			if (used == 512) { // Si notre PDE est rempli, écrire le nouveau nombre de PTE et effacer notre flag PAGE_NOTFULL
				used = used << 52;
				PDE [j] &= 0x800FFFFFFFFFFFFF;
				PDE [j] |= used;
				PDE [j] &= 0xFFFFFFFFFFFFFDFF;
			} else { // Ecrire le nouveau nombre de PTE
				used = used << 52;
				PDE [j] &= 0x800FFFFFFFFFFFFF;
				PDE [j] |= used;
			}

			// ----------------------------- PTE -----------------------------

			// On localise la PTT
			tmp = PDE [j];
			tmp &= 0x7FFFFFFFFF000;
			PTE = (int64 *) tmp;

			for (j = 0; j < 512; j++) { // Cherche un PTE libre
				if (PTE [j] == 0) { // S'il est libre
					break; // Quitter la boucle
				}
			}
			if (j == 512) { // Si on a pas trouvé de PTE libre, c'est très grave, car il y a un problème de codage
				return 0; // Retourne 0
			}
			
			// On peut maintenant mettre le numéro de PTE dans l'adresse virtuelle
			virtAddr |= (j << 12);

			// Créons le PTE
			PTE [j] = flags;
			if (_physAddr != 0 && !(mflags & MEM_OUTPHYSICAL)) { // Si on a indiqué une valeur précise pour l'adresse physique
				physAddr = _physAddr + i * 4096;
				PTE [j] |= (physAddr & 0xFFFFFFFFFF000);
			} else { // Sinon on doit trouver notre adresse physique
				// On a notre page, on l'inscrit dans la table des enregistrements RAM
				ramPages [ramRecord + i] = pid;

				// On calcule l'adresse physique
				physAddr = 0x900000 + (ramRecord + i) * 4096;

				// On indique quelle est l'adresse physique dans le PTE
				PTE [j] |= (physAddr & 0xFFFFFFFFFF000);
			}
			
			if (i == 0) { // Si on map notre première page
				// Sauvegarder son adresse physique et virtuelle pour la fin de la fonction
				outVirt 	= virtAddr;
				outPhys 	= physAddr;
			}
		}
	} else if (mflags & MEM_PRIVATE) { 	// On veut allouer des pages privées
		for (i = 0; i < pagesNb; i++) { // On alloue les pages
			
		}
	}

	if (mflags & MEM_OUTPHYSICAL) { // On demande à connaître l'adresse physique de la première page
		int64 *addr = (int64 *) _physAddr;
		*addr = outPhys;
	}
	
	return outVirt; // Retourne l'adresse virtuelle de la première page
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

