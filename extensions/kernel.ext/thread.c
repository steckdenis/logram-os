/*
* thread.c
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

/******************************************************************************
	Gestion des threads
		Création de threads et commutation par NMI
******************************************************************************/

#include <types.h>
#include <asm.h>

#include "console.h"
#include "thread.h"
#include "mem.h"

TSS	*firstThread;	//Premier thread de la liste

void NMI_enable(void)
{
	outb(0x70, inb(0x70)&0x7F);
}

void ThreadInit()
{
	//Créer le thread principal
	
	//Mettre en place la NMI
	NMI_enable();
}

/*CreateThread : crée un thread
	-start : adresse de la fonction de départ du thread
	-PLM4E : adresse de la structure de pagin du thread
	-stack : adresse de la pile du thread
	-rsp0 : adresse de la pile au niveau de privilège élevé
	-rflags : contenu du registre rflags
	-pid : processus parent
	-flags : flags du thread (0 pour le moment)
	-param : paramètre envoyé au thread (mis dans rax)
*/
void	*_CreateThread	(void *start, void *PLM4E, void *stack, void *rsp0, int64 rflags, int64 pid, int64 flags, int64 param)
{
	TSS	*tr;
	TSS	*thread;
	
	//On alloue le TSS
	tr = (TSS *) VirtualAlloc(0, 1, MEM_PUBLIC, pid); 
	
	//On remplit la TSS
	tr->rs0 = 0; tr->rs1 = 0; tr->rs2 = 0; tr->rs3 = 0;
	tr->rsp0 = rsp0;
	tr->iomap = 4094; //4ko - 2 octets, la fin du TSS (on n'utilise pas IOMAP)
	tr->rax = param;
	tr->rip = (int64) start;
	tr->rflags = rflags;
	tr->cr0 = (int64) PLM4E;
	tr->pid = pid;
	tr->flags = flags;
	tr->pErrorStack = 0;
	tr->StartMessage = 0;
	tr->EndMessage = 0;
	
	//On le lie au thread précédent
	if (!firstThread)
	{
		firstThread = tr;
	}
	else
	{
		thread = firstThread;
		while (thread->NextThread)
		{
			thread = thread->NextThread;
		}
		thread->NextThread = (void *) tr;
	}
	
	//On retourne l'adresse de notre TSS. Le thread est créé, à la prochaine NMI, il aura une chance de recevoir la main.
	return (void *) tr;
}