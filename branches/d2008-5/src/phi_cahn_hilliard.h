/*****************************************************************************
 *
 *  phi_cahn_hilliard.h
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  (c) The University of Edinburgh 2008
 *
 *  $Id: phi_cahn_hilliard.h,v 1.3 2008-12-03 20:36:45 kevin Exp $
 *
 *****************************************************************************/

#ifndef _PHICAHNHILLIARD
#define _PHICAHNHILLIARD

void phi_cahn_hilliard(void);
void phi_ch_set_upwind_order(int);

double phi_ch_get_mobility(void);
void   phi_ch_set_mobility(const double);
void   phi_ch_op_set_mobility(const double, const int);

#endif
