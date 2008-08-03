;
; bootloader.s
; This file is part of Logram
;
; Copyright (C) 2008 - Denis Steckelmacher
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


; Bootloader de Logram

use16

org 0x7C00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;    Secteur 0, charge les 62 secteurs suivants                  ;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Sect:
        mov ax, cs
        mov ds, ax
        mov es, ax

        mov byte [mDrive], dl

        ;effacer l'écran
        mov ah, 0
        mov al, 3
        int 10h


        ;lire les 62 secteurs suivants du boot à 0x0000:7E00
        mov ax, 0x0000
        mov es, ax

        mov ah, 2
        mov dl, [mDrive]
        mov dh, 0
        mov cx, 2        ;1 = secteur de boot
        mov al, 62
        mov bx, 0x7E00
        int 13h        
        jc diskerror
        jmp Start


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Fonctions utilisées par sect0 ;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
diskerror:
        mov bx, strdiskerror
        call printstring
        .bc:        hlt
        jmp .bc        

printstring:        ;bx = adresse de la chaîne
        push ds        
        push di
        mov ax, 0xB800
        mov ds, ax

        mov di, [cs:addrLine] 
        xor cx, cx                ;compteur de caractères
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

        pop di
        pop ds
ret

printtitle:        ;bx = adresse de la chaîne, affiche le titre de la partition
        push ds        
        push di
        mov ax, 0xB800
        mov ds, ax

        mov di, [cs:addrLine] 
        sub di, 160-6        ;Passe 3 caractères
        xor cx, cx                ;compteur de caractères
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

        pop di
        pop ds
ret


mDrive:                	db        0
addrLine:        	dw        0        ; Numéro de ligne active à l'écran
strdiskerror:        	db        "Error on your Hard Drive !$"

;;;;;;;;;;;;;;;;;;;;
;;;; MBR        ;;;;
;;;;;;;;;;;;;;;;;;;;
times 446-($-$$)        db        0

db        0x80        	;Actif
db        0x00        	;Tête (inutilisé)
dw        0x0000        ;cylindre
db        0xEF        	;Indicateur de système (tiré au hasard, peut être changé)
db        0x00        	;Tête (inutilisé)
dw        0x0000        ;cylindre
dd        64        	;secteur de départ de la partition
dd        16*1024*2 	; Nombre de secteurs (ici, 16 Mo)

;Chaque enregistrement fait 16 octets


;;;;;;;;;;;;;;;;;;;
;;;; Signature ;;;;
;;;;;;;;;;;;;;;;;;;
times 510-($-$$)        db        0
db 0x55, 0xAA


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;    Bootloader de Logram, affiche la liste des partitions       ;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Start:

        ;Passer en mode 3 graphique (80x25 16 couleurs)
        mov ah, 0
        mov al, 3
        int 10h

        ;Afficher le message de bienvenue
        mov bx, msgWelcome
        call printstring
        mov bx, msgNull
        call printstring
        mov bx, msgNull ;on réappelle
        call printstring

        ;Explorer les partitions
        xor cx, cx
        .For:
                push cx

                ;bx = adresse en mémoire de l'enregistrement de partition
                mov di, cx
                shl di, 4  ; *16
                add di, 0x7C00+446

                ;Afficher son numéro
                mov bx, cx
                shl bx, 2
                add bx, msgNos
                call printstring 

                ;Vérifier si elle est active
                cmp byte [di], 0x80
                jne .EndFor

                ;Afficher des informations
                mov al, [di+4]        ;Indicateur de système
                cmp al, 0xEF        ;Logram
                jne .PasLogram
                        mov bx, msgLogram
                        jmp .FinIf
                .PasLogram:
                cmp al, 0x00        ;Code d'une partition de type NTFS (à trouver)
                jne .PasWindows
                        mov bx, msgWindows
                        jmp .FinIf
                .PasWindows:
                cmp al, 0x00        ;Code d'une partition de type ext3 (à trouver)
                jne .PasGrub
                        mov bx, msgGrub
                        jmp .FinIf
                .PasGrub:
                        mov bx, msgAutre                
                .FinIf:
                call printtitle

                ;Et on boucle le tout
                .EndFor:

                pop cx
                inc cx
                cmp cx, 4
                jne .For

        ;Demander le choix
        mov bx, msgNull
        call printstring
        mov bx, msgChoix
        call printstring

	; Afficher le curseur
	call showCursor

        .Bcl:

        mov ah, 0
        int 16h

        ;On vérifie l'entrée
        sub al, "0"

        cmp al, 1
        jl .Bcl
        cmp al, 4
        ja .Bcl

        ;Ok, on récupère l'entrée
        and ax, 0x00FF
        mov cx, ax
        dec cx                ;cx = numéro d'entrée
        
        ;On charge le secteur

        shl cx, 4        ; *16
        mov di, cx
        add di, 0x7C00+446
        mov ebx, [di+8]        ;ebx = numéro de secteur

        call loadsect

	; Met quelques valeurs dans les registres
	mov eax, ebx
	mov dl, [cs:mDrive]

        jmp 0x1000:0x0000 ;On saute dans la DBR

; Affiche le curseur à l'écran
showCursor:
	pusha
 
	mov bx, [cs:addrLine] ; ebx contient la ligne active
	mov ax, bx
	xor dx, dx
	mov cx, 160
	div cx
 
	mov [yCursor], al ; Stocke le résultat dans yCursor
 
	mov ax, dx
	mov cx, 2
	div cx
 
	mov [xCursor], al ; Stocke le résultat dans xCursor
 
	; Envoie les données
	mov ah, 2
	mov bh, 0
	mov dh, [yCursor]
	mov dl, [xCursor]
	int 10h
 
	popa
	ret

;Charge à 0x1000:0x0000 le secteur passé dans bx. Il faut calculer son adresse CHS à partir du LBA
loadsect:
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
        mov ax, 0x1000
        mov es, ax


        mov ah, 2
        mov dl, [cs:mDrive]
        mov dh, [cs:H]
        mov cl, [cs:C+1]
        and cl, 11000000b
        or cl, [cs:S]
        mov ch, [cs:C]
        mov al, 1
        mov bx, 0x0000
        int 13h
        jc diskerror

	popa

ret

S:                 	db         	0

A:                	dd         	0

C:                 	dw         	0

H:                 	db         	0

Tmp:			dd		0


        

msgWelcome      db      "Bonjour et bienvenue dans le bootloader de Logram. Les partitions valides sont affichees ci-dessous. Choisissez celle que vous voulez.$"
msgNos         	db      "1: $2: $3: $4: $"
msgLogram      	db      "Logram$"
msgWindows     	db      "Windows$"
msgGrub      	db      "Grub$"
msgAutre        db      "Autre$"
msgChoix        db      "Numero de la partition (0-4) : $"

msgNull        	db      "$"
xCursor		db	0
yCursor		db	0
