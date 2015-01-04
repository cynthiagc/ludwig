/*****************************************************************************
 *
 *  subgrid.h
 *
 *  $Id: subgrid.h,v 1.2 2010-10-15 12:40:03 kevin Exp $
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  (c) 2010-2015 The University of Edinburgh
 *
 *****************************************************************************/

#ifndef SUBGRID_H
#define SUBGRID_H

#include "coords.h"
#include "colloids.h"
#include "hydro.h"

int subgrid_update(coords_t * cs, colloids_info_t * cinfo, hydro_t * hydro);
int subgrid_force_from_particles(coords_t * cs, colloids_info_t * cinfo,
				 hydro_t * hydro);
int subgrid_on_set(void);
int subgrid_on(int * flag);

#endif
