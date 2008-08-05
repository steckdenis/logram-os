/*
 * ide_driver.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - Bruno Régaldo
 *		      - Denis Steckelmacher
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

#ifndef IDE_DRIVER_H
#define IDE_DRIVER_H

/*  Defines propres au pilote de disque, accessible à tout le système*/

//Commandes ioctl
#define	IOCTL_BLOCK_SIZE	0x01	//Retourne le nombre d'octets dans un bloc
#define IOCTL_BLOCK_COUNT	0x02	//Retourne le nombre total de blocs sur le disque
#define IOCTL_BLOCK_CACHE	0x03	//Définis la taille du cache (0 pour désactiver)

#endif // IDE_DRIVER_H


