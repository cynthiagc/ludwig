/*****************************************************************************
 *
 *  coords.h
 *
 *  $Id: coords.h,v 1.4 2010-10-15 12:40:02 kevin Exp $
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  (c) 2010-2015 The University of Edinburgh
 *
 *****************************************************************************/

#ifndef COORDS_H
#define COORDS_H

#include "pe.h"

typedef struct coords_s coords_t;

#define NSYMM 6      /* Elements for general symmetric tensor */

enum cartesian_directions {X, Y, Z};
enum cartesian_neighbours {FORWARD, BACKWARD};
enum upper_triangle {XX, XY, XZ, YY, YZ, ZZ};

/* Host interface */

int coords_create(pe_t * pe, coords_t ** pcoord);
int coords_free(coords_t ** pcoord);
int coords_retain(coords_t * cs);

int coords_decomposition_set(coords_t * cs, const int irequest[3]);
int coords_periodicity_set(coords_t * cs, const int iper[3]);
int coords_ntotal_set(coords_t * cs, const int ntotal[3]);
int coords_nhalo_set(coords_t * cs, int nhalo);
int coords_reorder_set(coords_t * cs, int reorder);
int coords_commit(coords_t * cs);
int coords_info(coords_t * cs);
int coords_cart_comm(coords_t * cs, MPI_Comm * comm);
int coords_periodic_comm(coords_t * cs, MPI_Comm * comm);
int coords_cart_neighb(coords_t * cs, int forwback, int dim);

/* Host / device interface */

int coords_cartsz(coords_t * cs, int cartsz[3]);
int coords_cart_coords(coords_t * cs, int coords[3]);

int coords_lmin(coords_t * cs, double lmin[3]);
int coords_ltot(coords_t * cs, double ltot[3]);
int coords_periodic(coords_t * cs, int period[3]);

int coords_nlocal(coords_t * cs, int n[3]);
int coords_nlocal_offset(coords_t * cs, int n[3]);
int coords_nhalo(coords_t *cs, int * nhalo);
int coords_index(coords_t * cs, int ic, int jc, int kc);

int coords_ntotal(coords_t * cs, int ntotal[3]);
int coords_nsites(coords_t * cs, int * nsites);
int coords_minimum_distance(coords_t * cs, const double r1[3],
			    const double r2[3], double r12[3]);
int coords_index_to_ijk(coords_t * cs, int index, int coords[3]);
int coords_strides(coords_t * cs, int * xs, int * ys, int * zs);

/* A "class" function */

int coords_cart_shift(MPI_Comm comm, int dim, int direction, int * rank);

#endif
