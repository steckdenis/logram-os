;
; kernels.s
; This file is part of Logram
;
; Copyright (C) 2008 - Denis Steckelmacher
;		     - royalbru
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

; Entrée et code de kernel.ext en assembleur.

bits 64 ; Nous codons en 64 bits

global LoadGDT

section .text

; Fonction LoadGDT qui charge la GDT identifiée par la structure pGDT64
LoadGDT:
	lgdt [pGDT64] ; Charge la GDT
	; Sélectionne le segment de données pour ds
	mov ax, 0x18
	mov ds, ax
	ret

section .data
; Structure pGDT64 qui sera chargée dans le GDTR
pGDT64:
	dw	0xFFFF
	dq	0x30000
