;
; stage2s.s
; This file is part of Logram
;
; Copyright (C) 2008 - Bruno Régaldo
;		     - Denis Steckelmacher
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

; Stage2 de Logram

bits 32 ; On code en 32 bits

; Fonction start de stage en C
extern start

global main ; Rend notre fonction main repérable par ld
global JmpToKernel ; Rend notre fonction JmpToKernel repérable par stage2.c

section .bss
extern logramSect
extern port
extern protocol
extern master

section .text

; Main, fonction d'entrée de stage2
main:
	mov ax, 32
	mov ds, ax
	mov ax, 24
	mov ss, ax

	mov [logramSect], ebx

	; Efface le flag de direction
	cld

	; Appeler la fonction start de stage2
	call start

JmpToKernel:
	; Activer la pagination 64 bits
	mov eax, cr4
	bts eax, 5
	mov cr4, eax

	; Enregistrer les tables de pagin
 	mov eax, 0x50000
	mov cr3, eax

	; Activer le mode long
	mov ecx, 0xC0000080 ; Registre EFER
	rdmsr
	bts eax, 11; Pour le flag NX
	bts eax, 8 ; Pour le mode long
	bts eax, 0 ; Pour les syscalls
	wrmsr

	; Activer la pagination
	mov eax, cr0
	bts eax, 31 ; Pagination
	bts eax, 16 ; Write Protect
	mov cr0, eax

	; Pour que le noyau sache qu'il démarre, il faut certaines valeurs dans des registres
	mov eax, [logramSect]
	mov bx, [port]
	mov ch, [master]
	mov cl, [protocol]
	
	; Sauter dans Kernel.ext
	jmp dword 0x0010:0x802000
