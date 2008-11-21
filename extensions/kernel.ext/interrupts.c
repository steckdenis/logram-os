/*
 * interrupts.c
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Bruno Régaldo
 * 		      - Denis Steckelmacher
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
	Les fonctions contenues dans ce fichier sont en relation avec la gestion des interruptions.
*/

// Inclusions
#include <types.h>
#include <asm.h>

#include "interrupts.h"
#include "console.h"

// Variables globales
int8 PIC1State = 0xFF; // Cette variables représentent le masque d'IRQ du premier PIC
int8 PIC2State = 0xFF; // Cette variables représentent le masque d'IRQ du deuxième PIC

// Fonction MakeIDT qui se charge de créer l'IDT et de la charger
void MakeIDT () {
	int i; // Itérateur

	// Initialise toutes les interruptions
	for (i = 0; i < 256; i++) {
		CreateIDTSegment (i, 0x10, (int64) &int_default, 0x8E00);
	}

	// Initialise les PIC
	InitPIC();

	// Exceptions
	CreateIDTSegment (0, 0x10, (int64) &int_0, 0xEE00);
	CreateIDTSegment (1, 0x10, (int64) &int_1, 0xEE00);
	CreateIDTSegment (2, 0x10, (int64) &int_2, 0xEE00);
	CreateIDTSegment (3, 0x10, (int64) &int_3, 0xEE00);
	CreateIDTSegment (4, 0x10, (int64) &int_4, 0xEE00);
	CreateIDTSegment (5, 0x10, (int64) &int_5, 0xEE00);
	CreateIDTSegment (6, 0x10, (int64) &int_6, 0xEE00);
	CreateIDTSegment (7, 0x10, (int64) &int_7, 0xEE00);
	CreateIDTSegment (8, 0x10, (int64) &int_8, 0xEE00);
	CreateIDTSegment (10, 0x10, (int64) &int_10, 0xEE00);
	CreateIDTSegment (11, 0x10, (int64) &int_11, 0xEE00);
	CreateIDTSegment (12, 0x10, (int64) &int_12, 0xEE00);
	CreateIDTSegment (13, 0x10, (int64) &int_13, 0xEE00);
	CreateIDTSegment (14, 0x10, (int64) &int_14, 0xEE00);
	CreateIDTSegment (16, 0x10, (int64) &int_16, 0xEE00);
	CreateIDTSegment (17, 0x10, (int64) &int_17, 0xEE00);
	CreateIDTSegment (18, 0x10, (int64) &int_18, 0xEE00);
	CreateIDTSegment (19, 0x10, (int64) &int_19, 0xEE00);
	CreateIDTSegment (30, 0x10, (int64) &int_30, 0xEE00); 

	// IRQ
	RequestIRQ (0, &int_32); // Horloge

	// On charge l'IDT
	LoadIDT ();
}

// Fonction qui enregistre un nouveau segment dans l'IDT
// Paramètres : - int num 	: index dans l'IDT
//		- int16 seg 	: index dans une table de descripteur (GDT ou LDT)
//		- in64 addr 	: adresse de l'ISR
//		- int16 flags 	: flags
void CreateIDTSegment (int num, int16 seg, int64 addr, int16 flags) {
	idtrec *idt = (idtrec *) 0x10000;

	idt[num].offset0_15 	= (int16) addr;
	idt[num].selector 	= seg;
	idt[num].flags 		= flags;
	idt[num].offset16_31 	= (int16)(addr>>16);
	idt[num].offset32_63 	= (int32)(addr>>32);
	idt[num].zeros 		= 0;
}

// Fonction InitPIC qui intitialise les 2 PICs
void InitPIC () {
	// Initialisation de PIC_MASTER
	outb (PIC_MASTER_COMMAND, 0x11); 	// ICW 1
	delay ();
	outb (PIC_MASTER_DATA, PIC_MASTER_IRQ); // ICW 2
	delay ();
	outb (PIC_MASTER_DATA, 0x04); 		// ICW 3
	delay ();
	outb (PIC_MASTER_DATA, 0x03); 		// ICW 4
	delay ();
	outb (PIC_MASTER_DATA, 0xFF);		// Masquage des interruptions
	delay ();

	// Initialisation de PIC_SLAVE
	outb (PIC_SLAVE_COMMAND, 0x11); 	// ICW 1
	delay ();
	outb (PIC_SLAVE_DATA, PIC_SLAVE_IRQ); 	// ICW 2
	delay ();
	outb (PIC_SLAVE_DATA, 0x02); 		// ICW 3
	delay ();
	outb (PIC_SLAVE_DATA, 0x03); 		// ICW 4
	delay ();
	outb (PIC_SLAVE_DATA, 0xFF);		// Masquage des interruptions
	delay ();
}

// Permet de produire un temps de latence très bref
void delay ()
{	
	goto etiquette;
	etiquette:
	return;	
}

// Analyse les codes d'erreurs des exceptions s'il y en a
void AnalyzeErrorCode (int error) {
	if (error == 0) { // Code d'erreur à 0
		kprintf("    Empty error code", 0x04);
	} else { // Code d'erreur rempli
		if (error & 1) { // Cause de l'erreur est externe au processeur
			kprintf("    The exception source is external to the processor", 0x04);
		} else { // Cause de l'erreur est interne au processeur
			kprintf("    The exception source is internal to the processor", 0x04);
		}
		if (error & 2) { // Le selector indique un gate descriptor situé dans l'IDT
		} else { // Sinon le selector indique une entrée dans la GDT ou la LDT
			if (error & 3) { // Entrée dans la LDT
			} else { // Entrée dans la GDT
			}
		}
	}
}

//Exception #DE (0)
void int_de() {
	kprintf("Exception #DE (0)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #DB (1)
void int_db() {
	kprintf("Exception #DB (1)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #BP (3)
void int_bp() {
	kprintf("Exception #BP (3)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #OF (4)
void int_of() {
	kprintf("Exception #OF (4)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #BR (5)
void int_br() {
	kprintf("Exception #BR (5)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #UD (6)
void int_ud() {
	kprintf("Exception #UD (6)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #NM (7)
void int_nm() {
	kprintf("Exception #NM (7)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #DF (8)
void int_df(int error) {
	kprintf("Exception #DF (8)", 0x04);
	AnalyzeErrorCode (error);
	kprintf("Unending loop", 0x04);
	kprintf("Logram must shutdown.", 0x04);
	for (;;);
}

//Exception #TS (10)
void int_ts(int error) {
	kprintf("Exception #TS (10)", 0x04);
	AnalyzeErrorCode (error);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #NP (11)
void int_np(int error) {
	kprintf("Exception #NP (11)", 0x04);
	AnalyzeErrorCode (error);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #SS (12)
void int_ss(int error) {
	kprintf("Exception #SS (12)", 0x04);
	AnalyzeErrorCode (error);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #GP (13)
void int_gp(int error) {
	kprintf("Exception #GP (13)", 0x04);
	AnalyzeErrorCode (error);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #PF (14)
void int_pf(int error) {
	kprintf("Exception #PF (14)", 0x04);
	if (error & 1) {
		kprintf("    Not-present page", 0x04);
	} else {
		kprintf("    Page protection violation", 0x04);
	}
	if (error & 2) {
		kprintf("    Write access unauthorized", 0x04);
	} else {
		kprintf("    Read access unauthorized", 0x04);
	}
	if (error & 4) {
		kprintf("    Bad acces in user mode", 0x04);
	} else {
		kprintf("    Bad acces in supervisor mode", 0x04);
	}
	if (error & 8) {
		kprintf("    Reading a 1 in a reserved field", 0x04);
	}
	if (error & 16) {
		kprintf("    Instruction fetch", 0x04);
	}
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #MF (16)
void int_mf() {
	kprintf("Exception #MF (16)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #AC (17)
void int_ac(int error) {
	kprintf("Exception #AC (17)", 0x04);
	AnalyzeErrorCode (error);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #MC (18)
void int_mc() {
	kprintf("Exception #MC (18)", 0x04);
	kprintf("Unending loop", 0x04);
	kprintf("Logram must shutdown.", 0x04);
	for (;;);
}

//Exception #XF (19)
void int_xf() {
	kprintf("Exception #XF (19)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Exception #SX (30)
void int_sx() {
	kprintf("Exception #SX (30)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Interruption #NMI
void int_nmi() {
	kprintf("Exception #NMI (2)", 0x04);
	kprintf("Unending loop", 0x04);
	for (;;);
}

//Interruption par défaut
void int_default_() {
	// Vide
}

// Fonction qui assigne une fonction à une IRQ
// Paramètres : - int num 	: numéro d'interruption
//		- void *func 	: fonction à assigner à l'interruption
void RequestIRQ(int num, void *func) {
	int16 port = 0;
	int8 data;
	int8 *state;
	//Enregistrer la fonction de gestion dans l'IDT. Les interruptions d'irq commencent à 0x20
	CreateIDTSegment (0x20 + num, 0x10, (int64) func, 0xEE00);

	//démasquer l'interruption dans le PIC correspondant
	if (num > 7) {
		//PIC 2
		port = PIC_SLAVE_DATA;
		state = &PIC2State;
		num -= 8;	//On décale les interruptions
	} else {
		//PIC 1
		port = PIC_MASTER_DATA;
		state = &PIC1State;
	}

	//le bit à la position <num> doit être à 0 pour autoriser l'interruption
	data = 1<<num;
	data = !data;	//Avant, on avait 00000100, maintenent, on a 11111011
	//ANDer cette valeur sur le port
	*state = *state & data;
	//Sortir la valeur
	outb(port, *state);
}

// Fonction qui masque une IRQ
// Paramètres : - numéro de l'interruption
void MaskIRQ (int num) {
	int16 port = 0;
	int8 data;
	int8 *state;

	//masquer l'interruption dans le PIC correspondant
	if (num > 7) {
		//PIC 2
		port = PIC_SLAVE_DATA;
		state = &PIC2State;
		num -= 8;	//On décale les interruptions
	} else {
		//PIC 1
		port = PIC_MASTER_DATA;
		state = &PIC1State;
	}

	//le bit à la position <num> doit être à 0 pour masquer l'interruption
	data = 1 << num;

	//ORer cette valeur sur le port
	*state = *state | data;
	//Sortir la valeur
	outb(port, *state);
}

