/*
 * interrupts.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Bruno Régaldo
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
	Fichier d'en-tête du fichier interrupts.c.
*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

// Structures
typedef struct {
	int16 offset0_15;	// Adresse de la fonction (bits 0 à 15)
	int16 selector;		// Selecteur du segment de code
	int16 flags;		// Flags
	int16 offset16_31;	// Adresse de la fonction (bits 16 à 31)
	int32 offset32_63;	// Adresse de la fonction (bits 32 à 63)
	int32 zeros;		// Doit être à zéro
} __attribute__((__packed__)) idtrec; // Enregistrement de l'IDT

// Defines

// Ports des PIC
#define PIC_MASTER_COMMAND 	0x20
#define PIC_MASTER_DATA 	0x21
#define PIC_SLAVE_COMMAND 	0xA0
#define PIC_SLAVE_DATA 		0xA1

// Offset des IRQ
#define PIC_MASTER_IRQ 		0x20
#define PIC_SLAVE_IRQ 		0x28

// Prototypes
void 	MakeIDT 	();
void	CreateIDTSegment(int num, int16 seg, int64 addr, int16 flags);
void	InitPIC		();
void 	RequestIRQ	(int num, void *func);
void 	MaskIRQ 	(int num);
void 	delay 		();
void 	AnalyzeErrorCode(int error);
void 	int_default_	();
void 	int_de		();		// 0
void 	int_db		();		// 1
void 	int_nmi		();		// 2
void 	int_bp		();		// 3
void 	int_of		();		// 4
void 	int_br		();		// 5
void 	int_ud		();		// 6
void 	int_nm		();		// 7
void 	int_df		(int error);	// 8
void 	int_ts		(int error);	// 10
void 	int_np		(int error);	// 11
void 	int_ss		(int error);	// 12
void 	int_gp		(int error);	// 13
void 	int_pf		(int error);	// 14
void 	int_mf		();		// 16
void 	int_ac		(int error);	// 17
void 	int_mc		();		// 18
void 	int_xf		();		// 19
void 	int_sx		();		// 30
void 	int_clock	();		// 32
void 	int_0		();
void 	int_1		();
void 	int_2		();
void 	int_3		();
void 	int_4		();
void 	int_5		();
void 	int_6		();
void 	int_7		();
void 	int_8		();
void 	int_10		();
void 	int_11		();
void 	int_12		();
void 	int_13		();
void 	int_14		();
void 	int_16		();
void 	int_17		();
void 	int_18		();
void 	int_19		();
void 	int_30		();
void 	int_32		();
void 	int_48		();
void 	int_default	();

#endif // INTERRUPTS_H

