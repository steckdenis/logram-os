/*
 * drivers.h
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
 
/* En-tête pour les pilotes */

#ifndef DRIVERS_H
#define DRIVERS_H

// Define

// Types de pilote
#define DRIVERCLASS_CHAR	1
#define	DRIVERCLASS_BLOCK	2
#define DRIVERCLASS_FS		3
#define DRIVERCLASS_NET		4
#define DRIVERCLASS_DISPLAY	5
#define DRIVERCLASS_ANALOG	6
#define DRIVERCLASS_MISC	7

// IOCTL (chaque classe de pilote a aussi ses propres ioctls)
#define IOCTL_ERROR_SUCCES	0	// Tout s'est bien passé
#define IOCTL_ERROR_UNIMPLEMENTED 1	// Le message n'est pas pris en charge

// Structures
typedef struct {
	int16 	size;		// Taille de la structure
	int	driverClass;	// Classe du pilote
	int16	id;		// ID du pilote (utilisé par GetDriverByID)
	lchar	driverName [8];	// Nom du pilote, complété à droite par des espaces (utilisé par GetDriverByName)
	void 	*ext;		// Adresse de l'extension qui contient le pilote
} DEVICE, *lpDEVICE;	// Structure de retour de Initdriver (32 octets)

#endif // DRIVERS_H

