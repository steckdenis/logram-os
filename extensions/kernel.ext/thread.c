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

TSS	*currThread = (TSS *) 0;
TSS	*firstThread = (TSS *) 0;	//Premier thread de la liste

void NMI_enable(void)
{
	outb(0x70, inb(0x70)&0x7F);
	inb(0x71);
}

void ThreadInit()
{
	void	*pThread;		//Thread créé
	int64	rflags;
	
	//Créer le thread principal
	asm(	"pushfq\n"
		"popq %0"
	: "=m" (rflags));
	pThread = _CreateThread(0, (void *) 0x50000, (void *) 0x90000, (void *) 0x80000, rflags, 1, 0, 0); //Pas besoin de start_thread, puisqu'à la prochaine NMI, il se fait archiver
	
	CreateSysSegment(4, (int64) pThread, 4096, 0x0089);
	
	//Passer à ce thread
	short content = 32;
	asm ("ltr (%0)" :: "a"(&content));
	
	//Sauter dedans
	asm("int $2");
}

//structure propre à la NMI
typedef struct
{
	int64	_rflags;
	int64	r15;
	int64	r14;
	int64	r13;
	int64	r12;
	int64	r11;
	int64	r10;
	int64	r9;
	int64	r8;
	int64	_rsp;
	int64	rbp;
	int64	rdi;
	int64	rsi;
	int64	rdx;
	int64	rcx;
	int64	rbx;
	int64	rax;
	int64	rip;	//A partir d'ici, c'est le stackframe fourni par le processeur
	int64	cs;
	int64	rflags;
	int64	rsp;
	int64	ss;
} stackframe;

//Exception #NMI, on commute les threads
void int_nmi() {
	stackframe *st;		//Stackframe donné par l'interruption (voir interruptss.s)
	TSS	*prevThread;	//TSS du thread qui vient de perdre la main
	TSS	*nextThread;	//TSS du thread qui va avoir la main
	int16	tr;		//Registre tr
	int64	addr;		//Variable pour la calcul d'adresses
	int64	cr0;		//cr0
	
	gdtsysrec	tss;	//Segment de TSS
	
	//Récupérer le stackframe
	asm(	"mov %%rax, %0"
	: "=m" (st));
	
	//Récupérer le registre TR courrant
	asm(	"str %0"
	: "=a" (tr));
	
	//On récupère aussi le cr0
	asm(	"mov %%cr0, %0"
	: "=a" (cr0));
	
	//On archive le tout dans le TSS courrant du thread. Il faut d'abord retrouver la base de cette TSS
	ReadSegment(tr, &tss); //On trouve le descripteur
	addr = tss.base0_15 | (((int64 )tss.base16_23) << 16) | (((int64 )tss.base24_31) << 24) | (((int64 )tss.base32_63) << 32); //Et on retrouve son adresse de base
	prevThread = (TSS *) addr; //ouf ! c'est fait.
	
	prevThread = firstThread; //Pour le moment, les quelques lignes ci-dessus ne marchent pas
	
	//On archive le thread courrant
	prevThread->ss = st->ss;
	prevThread->rsp = st->rsp;
	prevThread->rflags = st->rflags;
	prevThread->cs = st->cs;
	prevThread->rip = st->rip;
	prevThread->rax = st->rax;
	prevThread->rbx = st->rbx;
	prevThread->rcx = st->rcx;
	prevThread->rdx = st->rdx;
	prevThread->rsi = st->rsi;
	prevThread->rdi = st->rdi;
	prevThread->rsp = st->rsp;
	prevThread->rbp = st->rbp;
	prevThread->r8 = st->r8;
	prevThread->r9 = st->r9;
	prevThread->r10 = st->r10;
	prevThread->r11 = st->r11;
	prevThread->r12 = st->r12;
	prevThread->r13 = st->r13;
	prevThread->r14 = st->r14;
	prevThread->r15 = st->r15;
	prevThread->cr0 = cr0;
	
	//On archive les données XMM, MMX, x87 et autres
	asm(	"fxsave (%0)"
	:: "r" (prevThread->SSE));
	
	//Le thread est sauvegardé ! On peut maintenant restaurer le suivant.
	//Ici, on prend le thread suivant, bêtement. On pourrait à cet endroit-ci ajouter un olgarithme d'ordonnancement pour trouver le thread qui a le plus besoin de temps d'exécution.
	nextThread = (TSS *) prevThread->NextThread;
	
	if (!nextThread)
	{
		//On est arrivé à la fin de la chaine, on reprend le premier
		nextThread = firstThread;
	}
	
	//On restaure (procédure inverse
	asm(	"fxrstor (%0)"
	:: "r" (nextThread->SSE));
	
	//On restaures les autres registres
	st->ss = nextThread->ss;
	st->rsp = nextThread->rsp;
	st->rflags = nextThread->rflags;
	st->cs = nextThread->cs;
	st->rip = nextThread->rip;
	st->rax = nextThread->rax;
	st->rbx = nextThread->rbx;
	st->rcx = nextThread->rcx;
	st->rdx = nextThread->rdx;
	st->rsp = nextThread->rsp;
	st->rbp = nextThread->rbp;
	st->rsi = nextThread->rsi;
	st->rdi = nextThread->rdi;
	st->r8 = nextThread->r8;
	st->r9 = nextThread->r9;
	st->r10 = nextThread->r10;
	st->r11 = nextThread->r11;
	st->r12 = nextThread->r12;
	st->r13 = nextThread->r13;
	st->r14 = nextThread->r14;
	st->r15 = nextThread->r15;
	cr0 = nextThread->cr0;
	
	//On récupère aussi le cr0
	asm(	"mov %0, %%cr0"
	:: "a" (cr0));
	
	//On change le registre TR
	short content = nextThread->tr;
	asm ("ltr (%0)" :: "a"(&content));
	
	/*
		Et voilà, on a changé de thread. On va retourner dans la partie en assembleur, qui va
		POPer le contenu du stackframe, et donc restaurer tous les registres. iretq finira le
		travail en mettant RIP, RSP, CS et SS aux bonnes valeurs
	*/
	
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
	tr->NextThread = 0;	//Pas encore de thread suivant, c'est très important de mettre ça, on on plante l'ordonnanceur
	
	//On le lie au thread précédent
	if (!firstThread)
	{
		firstThread = tr;
		//Le prochain thread a exécuter est celui-ci
		currThread = tr;
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