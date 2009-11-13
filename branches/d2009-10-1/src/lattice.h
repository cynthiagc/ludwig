/***************************************************************************
 *
 *  hydrodynamics.h
 *
 *  $Id: lattice.h,v 1.7 2009-05-29 06:59:23 kevin Exp $
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  (c) 2008 The University of Edinburgh
 *
 ***************************************************************************/

#ifndef HYDRODYNAMICS_H_
#define HYDRODYNAMICS_H_

void hydrodynamics_init(void);
void hydrodynamics_halo_u(void);
void hydrodynamics_finish(void);
void hydrodynamics_set_force_local(const int, const double *);
void hydrodynamics_add_force_local(const int, const double *);
void hydrodynamics_get_force_local(const int, double *);
void hydrodynamics_set_velocity(const int, const double *);
void hydrodynamics_get_velocity(const int, double *);
void hydrodynamics_zero_force(void);
void hydrodynamics_leesedwards_transformation(void);
void hydrodynamics_stats(void);
void hydrodynamics_velocity_gradient_tensor(const int, const int, const int,
					    double w[3][3]);

extern struct io_info_t * io_info_velocity_;
#endif