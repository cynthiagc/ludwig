/*****************************************************************************
 *
 *  colloids_s.h
 *
 *  $Id: 
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  (c) 2012-2016 The University of Edinburgh
 *
 *****************************************************************************/

#ifndef LUDWIG_COLLOIDS_S_H
#define LUDWIG_COLLOIDS_S_H

#include "pe.h"
#include "coords.h"
#include "colloids.h"


struct colloids_info_s {
  int nhalo;                  /* Halo extent in cell list */
  int ntotal;                 /* Total, physical, number of colloids */
  int nallocated;             /* Number colloid_t allocated */
  int ncell[3];               /* Number of cells (excluding  2*halo) */
  int str[3];                 /* Strides for cell list */
  int nsites;                 /* Total number of map sites */
  int ncells;                 /* Total number of cells */
  double rho0;                /* Mean density (usually matches fluid) */
  double drmax;               /* Maximum movement per time step */

  colloid_t ** clist;         /* Cell list pointers */
  colloid_t ** map_old;       /* Map (previous time step) pointers */
  colloid_t ** map_new;       /* Map (current time step) pointers */
  colloid_t * headall;        /* All colloid list (incl. halo) head */
  colloid_t * headlocal;      /* Local list (excl. halo) head */

  pe_t * pe;                  /* Parallel environment */
  cs_t * cs;                  /* Coordinate system */
  colloids_info_t * target;   /* Copy of this structure on target */ 
};


#endif
