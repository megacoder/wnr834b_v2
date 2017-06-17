/*
 * HND SiliconBackplane Gigabit Ethernet core software interface.
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: hndgige.h,v 1.1.1.1 2010/03/05 07:31:06 reynolds Exp $
 */

#ifndef _hndgige_h_
#define _hndgige_h_

extern void sb_gige_init(sb_t *sbh, uint32 unit, bool *rgmii);

#endif /* _hndgige_h_ */
