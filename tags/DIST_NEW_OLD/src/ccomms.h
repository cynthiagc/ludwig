/*****************************************************************************
 *
 *  ccomms.h
 *
 *  Colloid halo communications.
 *
 *  $Id: ccomms.h,v 1.4 2009-11-03 17:29:04 kevin Exp $
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *
 *****************************************************************************/

#ifndef _CCOMMS_H
#define _CCOMMS_H

enum message_type {CHALO_TYPE1 = 0, CHALO_TYPE2 = 1, CHALO_TYPE6 = 2,
                   CHALO_TYPE7 = 3, CHALO_TYPE8 = 4};

void CCOM_init_halos(void);
void CCOM_halo_particles(void);
void CCOM_halo_sum(const int);

#endif