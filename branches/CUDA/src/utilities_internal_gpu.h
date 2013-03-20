/*****************************************************************************
 * 
 * utilities_internal_gpu.h
 * 
 * Alan Gray
 *
 *****************************************************************************/

#ifndef UTILITIES_INTERNAL_GPU_H
#define UTILITIES_INTERNAL_GPU_H

#include "common_gpu.h"
#include "colloids.h"

/* declarations for required external (host) routines */
extern "C" void fluid_body_force(double f[3]);
extern "C" char site_map_get_status(int,int,int);
extern "C" char site_map_get_status_index(int);
//extern "C" void * colloid_at_site_index(int index);
extern "C" colloid_t * colloid_at_site_index(int index);



/* forward declarations of host routines internal to this module */
static void calculate_data_sizes(void);
static void allocate_memory_on_gpu(void);
static void free_memory_on_gpu(void);
int get_linear_index(int ii,int jj,int kk,int N[3]);


/* forward declarations of accelerator routines internal to this module */

__device__ static void get_coords_from_index_gpu_d(int *ii,int *jj,int *kk,
						   int index,int N[3]);
__device__ static int get_linear_index_gpu_d(int ii,int jj,int kk,int N[3]);


#endif