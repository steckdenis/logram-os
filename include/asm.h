/*
 * asm.h
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
 
/* Fonctions en assembleur */

#ifndef ASM_H
#define ASM_H

static __inline__ void outb(unsigned short port, unsigned char val)
{
   	asm volatile("outb %0,%1"::"a"(val), "Nd" (port));
}

static __inline__ void outw(unsigned short port, unsigned short val)
{
   	asm volatile("outw %0,%1"::"a"(val), "Nd" (port));
}

static __inline__ void outd(unsigned short port, unsigned int val)
{
   	asm volatile("outl %0,%1"::"a"(val), "Nd" (port));
}



static __inline__ unsigned char inb(unsigned short port)
{
   	unsigned char ret;
   	asm volatile ("inb %1,%0":"=a"(ret):"Nd"(port));
   	return ret;
}

static __inline__ unsigned short inw(unsigned short port)
{
   	unsigned short ret;
   	asm volatile ("inw %1,%0":"=a"(ret):"Nd"(port));
   	return ret;
}

static __inline__ unsigned int ind(unsigned short port)
{
   	unsigned int ret;
   	asm volatile ("inl %1,%0":"=a"(ret):"Nd"(port));
   	return ret;
}

static __inline__ void iowait()
{
   	outb(0x80, 0);
}

#endif // ASM_H

