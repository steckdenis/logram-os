;
; stage1.s
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

; DBR de Logram

; Main, fonction d'entrée de stage1
main:
	; Récupère les valeurs des registres que le bootloader a stocké
	mov [cs:mDrive], dl
	mov [cs:CurrSector], eax
	inc dword [cs:CurrSector]

	; Efface l'écran
	mov ah, 0
	mov al, 3
	int 10h

	; Affiche le message de bienvenue
	mov bx, strstart
	call printstring

	; Charge Stage2 en mémoire
	mov ebx, [cs:CurrSector]
	call loadStage2

	; Vérifie si le long mode est supporté
	mov   eax, 80000000h ; Extended-function 8000000h.
	cpuid                ; Is largest extended function
	cmp   eax, 80000000h ; any function > 80000000h?
	jbe   near no_long_mode   ; If not, no long mode.
	mov   eax, 80000001h ; Extended-function 8000001h.
	cpuid                ; Now EDX = extended-features flags.
	bt    edx, 29        ; Test if long mode is supported.
	jnc   no_long_mode   ; Exit if not supported.

	; Initialiser correctement l'ordinateur pour le mode protégé
	; 1: activer la ligne d'adresses A20
	in al, 0x92
	or al, 10b
	out 0x92, al

	;Attendre que la ligne a20 soit bien initialisée
	push ds
	xor	ax,ax			; segment 0x0000
	mov	ds,ax
	dec	ax			; segment 0xffff (HMA)
	mov	es,ax
	a20_wait:
	inc	ax			; unused memory location <0xfff0
	mov	[ds:0x200],ax		   ; we use the "int 0x80" vector
	cmp	ax,[es:0x210]		   ; and its corresponding HMA addr
	je	a20_wait		; loop until no longer aliased
	pop ds

	; 2: initialiser le contrôleur d'interruptions et désactiver les IRQ
	mov	al,0x11 	       ; initialization sequence
	out	0x20,al 	       ; send it to 8259A-1
	out	0x80,al
	out	0xA0,al 	       ; and to 8259A-2
	out	0x80,al
	mov	al,0x40 	       ; start of hardware int's (0x20)
	out	0x21,al
	out	0x80,al
	mov	al,0x48 	       ; start of hardware int's 2 (0x28)
	out	0xA1,al
	out	0x80,al
	mov	al,0x04 	       ; 8259-1 is master
	out	0x21,al
	out	0x80,al
	mov	al,0x02 	       ; 8259-2 is slave
	out	0xA1,al
	out	0x80,al
	mov	al,0x01 	       ; 8086 mode for both
	out	0x21,al
	out	0x80,al
	out	0xA1,al
	out	0x80,al
	mov	al,0xFF 	       ; mask off all interrupts for now
	out	0xA1,al
	out	0x80,al
	mov	al,0xFB 	       ; mask all irq's but irq2 which
	out	0x21,al 	       ; is cascaded

	mov ax, cs
	mov ds, ax

	lgdt [pGDT32]
	lidt [pIDT32]

	mov eax, 000000011h
	mov cr0, eax

	dec dword [cs:CurrSector]
	mov ebx, [cs:CurrSector]

	; Saute dans stage2
	jmp 0x0008:0x0000

; Est éxécuté lorsque le mode long n'est pas supporté
no_long_mode:
	; Affiche le message d'erreur
	mov bx, strlongmode
	call printstring
	.bc:	hlt
	jmp .bc

; GDT pour le mode protégé
pGDT32: 			; Used by LGDT.
	dw 5 * 8		; GDT limit ...
	dd GDT32 + 0x10000	; and 32-bit GDT base
	dw 0

; IDT pour le mode protégé
pIDT32: 		; Used by LIDT.
	dw 512		; IDT limit ...
	dd 0		; and 32-bit IDT base

; Contenu de la GDT du mode protégé
GDT32:
	SegNull:
		dw 0x0000		; Limite 0-15
		dw 0x0000 		; Base Addr 0-15
		db 0x00			; Base Addr 16-23
		dw 0000000000000000b	; Flags
		db 0x00			; Base Addr 24-31
	Code32: ; Code 32 bits qui commence à 0x20000
		dw 0xFFFF		; Limite 0-15
		dw 0x0000 		; Base Addr 0-15
		db 0x02			; Base Addr 16-23
		dw 1100111110011110b	; Flags
		db 0x00			; Base Addr 24-31
	Code64: ; Code 64 bits (pas de base)
		dw 0xFFFF		; Limite 0-15
		dw 0x0000 		; Base Addr 0-15 
		db 0x00			; Base Addr 16-23 
		dw 0110000010011100b	; Flags
		db 0x00			; Base Addr 24-31
	mStack: ; Pile
		dw 0xFFFF		; Limite 0-15
		dw 0x0000 		; Base Addr 0-15 
		db 0x00			; Base Addr 16-23
		dw 1000000010010010b	; Flags
		db 0x00			; Base Addr 24-31
	Data32: ; Données du Stage2 en mode protégé
		dw 0xFFFF		; Limite 0-15
		dw 0x0000 		; Base Addr 0-15
		db 0x02			; Base Addr 16-23
		dw 1000111110010010b	; Flags
		db 0x00			; Base Addr 24-31
	General: ; Segment général
		dw 0xFFFF		; Limite 0-15
		dw 0x0000 		; Base Addr 0-15 
		db 0x00			; Base Addr 16-23
		dw 1000111110010010b	; Flags
		db 0x00 		; Base Addr 24-31

; Affiche une chaîne à l'écran en mode réel
printstring:	;bx = adresse de la chaîne
	push ds	
	mov ax, 0xB800
	mov ds, ax

	mov di, [cs:addrLine] 
	xor cx, cx		;compteur de caractères
	.bc:
		mov al, [cs:bx]
		cmp al, "$"
		jz .fin

		mov byte [di], al
		add di, 2
		inc bx
		inc cx
		jmp .bc
	.fin:
	add word [cs:addrLine], 160

	pop ds
	ret

; Charge Stage2 en mémoire
loadStage2:
        pusha

        ;S = LBAAdress % 63

        ;A = LBAAdress / 63

        ;C = A >> 8

        ;H = (byte) A

	xor edx, edx
        mov eax, ebx
        mov ebx, 63
        div ebx
        mov byte [cs:S], dl
        inc byte [cs:S]
        mov dword [cs:A], eax

        shr eax, 8
        mov word [cs:C], ax

        mov al, [cs:A]
        mov byte [cs:H], al

        ;lire sur le disque
        mov ax, 0x2000
        mov es, ax


        mov ah, 2
        mov dl, [cs:mDrive]
        mov dh, [cs:H]
        mov cl, [cs:C+1]
        and cl, 11000000b
        or cl, [cs:S]
        mov ch, [cs:C]
        mov al, 62
        mov bx, 0x0000
        int 13h
        jc diskerror

	popa

ret

; En cas d'erreur de disque
diskerror:
        mov bx, strdiskerror
        call printstring
        .bc:        hlt
        jmp .bc 

; Variables utilisées en mode réel
strstart:	db	"Welcome to Logram v0.0.8.0, loading stage2...$"
strlongmode:	db	"Your processor can't support Long mode !$"
mDrive:        	db     	0
CurrSector	dd	0
addrLine:	dw	0	; Numéro de ligne active à l'écran
S:              db      0
A:              dd      0
C:              dw      0
H:              db      0
strdiskerror:  	db     	"Error on your Hard Drive !$"
