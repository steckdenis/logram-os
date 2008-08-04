/*
* thread.h
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
	En-tête pour la gestion des threads
******************************************************************************/ 

/* Fonction */
void	ThreadInit	();	//Initialisation des threads et de la NMI
void	*_CreateThread	(void *start, void *PLM4E, void *stack, void *rsp0, int64 rflags, int64 pid, int64 flags, int64 param); 	//Créer un thread (fonction de bas niveau)

/* Structures */

typedef struct
{
	int64	sender;		//Thread qui l'envoie
	int64	message;	//Message
	int64	Param1;		//Paramètre 1
	int64	Param2;		//Paramètre 2
} message;

typedef struct
{
	//Champs définis par le processeur
	int	rs0;
	int64	rsp0;
	int64	rsp1;
	int64	rsp2;
	int64	rs1;
	int64	ist1;
	int64	ist2;
	int64	ist3;
	int64	ist4;
	int64	int5;
	int64	ist6;
	int64	ist7;
	int64	rs2;
	int16	rs3;
	int16	iomap;
	//Champs de sauvegarde des registres
	int64	rax;
	int64	rbx;
	int64	rcx;
	int64	rdx;
	int64	rsi;
	int64	rdi;
	int64	rsp;
	int64	rbp;
	int64	r8;
	int64	r9;
	int64	r10;
	int64	r11;
	int64	r12;
	int64	r13;
	int64	r14;
	int64	r15;
	int64	rip;
	int64	rflags;
	int64	cr0;
	int64	ss;		//64 bits pour rester aligné
	int64	cs;		//même chose. Pas besoin de sauvegarder les autres registres
	int64	tr;		//Registre TR, utile pour le retrouver facilement à la commutation
	//Sauvegarde SSE (512 octets)
	char	SSE[512];
	//Champs propres à Logram
	int64	pid;			//Processus parent (seuls 16 bits sont utilisés)
	int64	flags;			//flags du thread
	void	*NextThread;		//Thread suivant (liste chainée)
	int64	pErrorStack;		//Position sur la pile des erreurs
	int64	StartMessage;		//Premier message non-lu
	int64	EndMessage;		//Dernier message posté
	int64	ErrorStack[128];	//pile des erreurs
	message Messages[64];		//Liste des messages
	//Le reste est disponible
} __attribute__((packed)) TSS;