/*
 * stage2.h
 * This file is part of Logram
 *
 * Copyright (C) 2008 - royalbru
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
	Fichier d'en-tête de stage2
*/

#ifndef STAGE2_H
#define STAGE2_H

// Defines

// Disque Maître ou Esclave
#define MASTER_DISK	1
#define SLAVE_DISK	2
// Protocoles identifiés par stage2
#define ATA_DISK	3	// Disque utilisant le protocole ATA
#define ATAPI_DISK	4	// Disque utilisant le protocole ATAPI
// Ensemble de define pour le mini driver ATA
#define ATA_DATA                        0x00
#define ATA_ERROR                       0x01
#define ATA_PRECOMP                     0x01
#define ATA_SECTOR_COUNT                0x02
#define ATA_SECTOR_NUMBER               0x03
#define ATA_CYL_LSB                     0x04
#define ATA_CYL_MSB                     0x05
#define ATA_DRIVE                       0x06
#define         ATA_D_IBM               0xa0    /* bits that must be set */
#define         ATA_D_LBA               0x40    /* use LBA ? */
#define         ATA_D_MASTER            0x00    /* select master */
#define         ATA_D_SLAVE             0x10    /* select slave */
#define ATA_STATUS                      0x07
#define         ATA_S_ERROR             0x01    /* error */
#define         ATA_S_INDEX             0x02    /* index */
#define         ATA_S_CORR              0x04    /* data corrected */
#define         ATA_S_DRQ               0x08    /* data request */
#define         ATA_S_DSC               0x10    /* drive Seek Completed */
#define         ATA_S_DWF               0x20    /* drive write fault */
#define         ATA_S_DRDY              0x40    /* drive ready */
#define         ATA_S_BSY               0x80    /* busy */

#define ATA_CMD                         0x07
#define         ATA_C_ATA_IDENTIFY      0xec    /* get ATA params */
#define         ATA_C_ATAPI_IDENTIFY    0xa1    /* get ATAPI params*/
#define         ATA_C_READ              0x20    /* read command */
#define         ATA_C_WRITE             0x30    /* write command */
#define         ATA_C_READ_MULTI        0xc4    /* read multi command */
#define         ATA_C_WRITE_MULTI       0xc5    /* write multi command */
#define         ATA_C_SET_MULTI         0xc6    /* set multi size command */
#define         ATA_C_PACKET_CMD        0xa0    /* set multi size command */

#define ATA_ALTPORT                     0x206   /* (R) alternate Status register */
#define ATA_DEVICE_CONTROL              0x206   /* (W) device control register */
#define         ATA_A_nIEN              0x02    /* disable interrupts */
#define         ATA_A_RESET             0x04    /* RESET controller */
#define         ATA_A_4BIT              0x08    /* 4 head bits */
// Pour la pagination
#define PAGE_PRESENT		0x00000001
#define PAGE_WRITE		0x00000002
#define PAGE_USER		0x00000004
#define PAGE_WRITETROUGH	0x00000008
#define PAGE_CACHEDISABLE	0x00000010
#define PAGE_ACCESSED		0x00000020
#define PAGE_DIRTY		0x00000040
#define PAGE_PAGESIZE		0x00000080
#define PAGE_PAT		0x00000080
#define PAGE_GLOBAL		0x00000100
#define PAGE_NOTFULL		0x00000200
#define PAGE_NX			0x8000000000000000

// Prototypes des fonctions contenues dans le fichier stage2.c
void 	print			(const char *str);
void 	FindDevice		();
void	LoadFSLInfos		();
void	LoadKernel		();
void	ReadSector		(int64 sector, void *buf);
void 	ATA_ReadSector 		(int64 sector, void *buf);
void 	ATAPI_ReadSector	(int64 sector, void *buf);
void 	FSL_Read 		(int64 baseBlock, void *buf);
int64 	FSL_Open 		(int64 baseBlock, const lchar *name);
void 	CreatePublicTable 	();
void 	udelay 			(int delay);
int 	strcmp 			(const lchar *s1, const lchar *s2);
void 	Quit 			();

// Fonction contenue dans le fichier assembleur de stage2
void 	JmpToKernel 		();

#endif // STAGE2_H

