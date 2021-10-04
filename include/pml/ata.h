/* ata.h -- This file is part of PML.
   Copyright (C) 2021 XNSC

   PML is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   PML is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PML. If not, see <https://www.gnu.org/licenses/>. */

#ifndef __PML_ATA_H
#define __PML_ATA_H

/*! @file */

#include <pml/cdefs.h>

#define ATA_VENDOR_ID           0x8086    /*!< PCI vendor ID for PIIX3 IDE */
#define ATA_DEVICE_ID           0x7010    /*!< PCI device ID for PIIX3 IDE */

#define ATA_SR_ERR              (1 << 0)  /*!< Error occurred */
#define ATA_SR_IDX              (1 << 1)  /*!< Index (always zero) */
#define ATA_SR_CORR             (1 << 2)  /*!< Corrected data (always zero) */
#define ATA_SR_DRQ              (1 << 3)  /*!< Data transfer requested */
#define ATA_SR_SRV              (1 << 4)  /*!< Overlapped service request */
#define ATA_SR_DF               (1 << 5)  /*!< Drive fault */
#define ATA_SR_RDY              (1 << 6)  /*!< Drive is ready */
#define ATA_SR_BSY              (1 << 7)  /*!< Preparing to send/receive data */

#define ATA_ER_AMNF             (1 << 0)  /*!< Address mark not found */
#define ATA_ER_TK0NF            (1 << 1)  /*!< Track zero not found */
#define ATA_ER_ABRT             (1 << 2)  /*!< Aborted command */
#define ATA_ER_MCR              (1 << 3)  /*!< Media change request */
#define ATA_ER_IDNF             (1 << 4)  /*!< ID not found */
#define ATA_ER_MC               (1 << 5)  /*!< Media changed */
#define ATA_ER_UNC              (1 << 6)  /*!< Uncorrectable data error */
#define ATA_ER_BBK              (1 << 7)  /*!< Bad block detected */

#endif
