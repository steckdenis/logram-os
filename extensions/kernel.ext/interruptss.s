;
; interruptss.s
; This file is part of Logram
;
; Copyright (C) 2008 - royalbru
;
; Logram is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; Logram is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with Logram; if not, write to the Free Software
; Foundation, Inc., 51 Franklin St, Fifth Floor, 
; Boston, MA  02110-1301  USA
;

; Fichier asm de la gestion des interruptions

bits 64 ; Nous codons en 64 bits

; Fonctions externes
extern int_de		; 0
extern int_db		; 1
extern int_nmi		; 2
extern int_bp		; 3
extern int_of		; 4
extern int_br		; 5
extern int_ud		; 6
extern int_nm		; 7
extern int_df		; 8
extern int_ts		; 10
extern int_np		; 11
extern int_ss		; 12
extern int_gp		; 13
extern int_pf		; 14
extern int_mf		; 16
extern int_ac		; 17
extern int_mc		; 18
extern int_xf		; 19
extern int_sx		; 30
extern int_clock	; 32
extern int_default_

; Prototypes
global LoadIDT
global int_0
global int_1
global int_2
global int_3
global int_4
global int_5
global int_6
global int_7
global int_8
global int_10
global int_11
global int_12
global int_13
global int_14
global int_16
global int_17
global int_18
global int_19
global int_30
global int_32
global int_48
global int_default

global currThread
extern firstThread

; Macros
%macro PUSHALL 	0
		push rax
		push rbx
		push rcx
		push rdx
		push rsi
		push rdi
		push rbp
		push rsp
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15
		xor rax, rax
		mov ax, ds
		push rax
		pushfq
%endmacro

%macro POPALL	0
		popfq
		pop rax
		mov ds, ax
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rsp
		pop rbp
		pop rdi
		pop rsi
		pop rdx
		pop rcx
		pop rbx
		pop rax
%endmacro

section .text

; Fonction qui charge la structure ptrIDT dans le IDTR
LoadIDT:
	lidt [ptrIDT]
	ret
	

; Structure qui sera chargée dans le IDTR
ptrIDT:
	dw 4096
	dq 0x10000

; Interruption horloge : commutation des threads
;	L'important dans cette fonction, c'est de ne détruire aucun registre, car on a l'équilibre de deux threads sur les bras !
int_32:
	mov rax, rax
	jmp .tst
	mov rbx, rbx
	.tst:
	cli
	PUSHALL
	mov rax, rsp 	;on met dans rax l'adresse de la structure qui contient les push
	call int_clock	;et on appelle int_nmi
	POPALL
	sti
iretq

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;								;;
;;				ISR				;;
;;								;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

int_0:
	PUSHALL
	call int_de
	POPALL
	add qword [rsp], 2
	iretq

int_1:
	PUSHALL
	call int_db
	POPALL
	iretq

int_3:
	PUSHALL
	call int_bp
	POPALL
	iretq

int_4:
	PUSHALL
	call int_of
	POPALL
	iretq

int_5:
	PUSHALL
	call int_br
	POPALL
	iretq

int_6:
	PUSHALL
	call int_ud
	POPALL
	iretq

int_7:
	PUSHALL
	call int_nm
	POPALL
	iretq

int_8:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_df
	POPALL
	add rsp, 8
	iretq

int_10:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_ts
	POPALL
	add rsp, 8
	iretq

int_11:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_np
	POPALL
	add rsp, 8
	iretq

int_12:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_ss
	POPALL
	add rsp, 8
	iretq

int_13:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_gp
	POPALL
	add rsp, 8
	iretq

int_14:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_pf
	POPALL
	add rsp, 8
	iretq

int_16:
	PUSHALL
	call int_mf
	POPALL
	iretq

int_17:
	PUSHALL
	mov qword rdi, [rsp + 144]
	call int_ac
	POPALL
	add rsp, 8
	iretq

int_18:
	PUSHALL
	call int_mc
	POPALL
	iretq

int_19:
	PUSHALL
	call int_xf
	POPALL
	iretq

int_30:
	PUSHALL
	call int_sx
	POPALL
	iretq

int_2:
	PUSHALL
	call int_nmi
	POPALL
	iretq

int_default:
	PUSHALL
	call int_default_
	POPALL
	iretq

