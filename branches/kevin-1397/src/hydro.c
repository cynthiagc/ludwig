/*****************************************************************************
 *
 *  hydro.c
 *
 *  Hydrodynamic quantities: velocity, body force on fluid.
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  (c) 2012-2016 The University of Edinburgh
 *
 *  Contributing authors:
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  Alan Gray (alang@epcc.ed.ac.uk)
 *
 *****************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h> 

#include "kernel.h"
#include "coords_field.h"
#include "util.h"
#include "hydro_s.h"

static int hydro_lees_edwards_parallel(hydro_t * obj);
static int hydro_u_write(FILE * fp, int index, void * self);
static int hydro_u_write_ascii(FILE * fp, int index, void * self);
static int hydro_u_read(FILE * fp, int index, void * self);
static int hydro_u_read_ascii(FILE * fp, int index, void * self);

static __global__
void hydro_field_set(hydro_t * hydro, double * field, const double z[NHDIM]);


/*****************************************************************************
 *
 *  hydro_create
 *
 *  We typically require a halo region for the velocity which is only
 *  one lattice site in width, i.e., nhcomm = 1. This is independent
 *  of the width of the halo region specified for coords object.
 *
 *****************************************************************************/

__host__ int hydro_create(pe_t * pe, cs_t * cs, lees_edw_t * le, int nhcomm,
			  hydro_t ** pobj) {

  int ndevice;
  double * tmp;
  hydro_t * obj = (hydro_t *) NULL;

  assert(pe);
  assert(cs);
  assert(pobj);

  obj = (hydro_t *) calloc(1, sizeof(hydro_t));
  if (obj == NULL) pe_fatal(pe, "calloc(hydro) failed\n");

  obj->pe = pe;
  obj->cs = cs;
  obj->le = le;
  obj->nhcomm = nhcomm;

  cs_nsites(cs, &obj->nsite);
  if (le) lees_edw_nsites(le, &obj->nsite);

  obj->u = (double *) calloc(NHDIM*obj->nsite, sizeof(double));
  if (obj->u == NULL) pe_fatal(pe, "calloc(hydro->u) failed\n");

  obj->f = (double *) calloc(NHDIM*obj->nsite, sizeof(double));
  if (obj->f == NULL) pe_fatal(pe, "calloc(hydro->f) failed\n");

  halo_swap_create_r1(pe, cs, nhcomm, obj->nsite, NHDIM, &obj->halo);
  assert(obj->halo);

  halo_swap_handlers_set(obj->halo, halo_swap_pack_rank1, halo_swap_unpack_rank1);

  /* Allocate target copy of structure (or alias) */

  targetGetDeviceCount(&ndevice);

  if (ndevice == 0) {
    obj->target = obj;
  }
  else {

    targetCalloc((void **) &obj->target, sizeof(hydro_t));

    targetCalloc((void **) &tmp, NHDIM*obj->nsite*sizeof(double));
    copyToTarget(&obj->target->u, &tmp, sizeof(double *)); 

    targetCalloc((void **) &tmp, NHDIM*obj->nsite*sizeof(double));
    copyToTarget(&obj->target->f, &tmp, sizeof(double *)); 

    copyToTarget(&obj->target->nsite, &obj->nsite, sizeof(int));
  }

  *pobj = obj;

  return 0;
}

/*****************************************************************************
 *
 *  hydro_free
 *
 *****************************************************************************/

__host__ int hydro_free(hydro_t * obj) {

  int ndevice;
  double * tmp;

  assert(obj);

  targetGetDeviceCount(&ndevice);

  if (ndevice > 0) {
    copyFromTarget(&tmp, &obj->target->u, sizeof(double *)); 
    targetFree(tmp);
    copyFromTarget(&tmp, &obj->target->f, sizeof(double *)); 
    targetFree(tmp);
    targetFree(obj->target);
  }

  halo_swap_free(obj->halo);
  free(obj->f);
  free(obj->u);
  free(obj);

  return 0;
}

/*****************************************************************************
 *
 *  hydro_memcpy
 *
 *****************************************************************************/

__host__ int hydro_memcpy(hydro_t * obj, int flag) {

  int ndevice;
  double * tmpu;
  double * tmpf;

  assert(obj);

  targetGetDeviceCount(&ndevice);

  if (ndevice == 0) {
    /* Ensure we alias */
    assert(obj->target == obj);
  }
  else {
    copyFromTarget(&tmpf, &obj->target->f, sizeof(double *));
    copyFromTarget(&tmpu, &obj->target->u, sizeof(double *));

    switch (flag) {
    case cudaMemcpyHostToDevice:
      copyToTarget(tmpu, obj->u, NHDIM*obj->nsite*sizeof(double));
      copyToTarget(tmpf, obj->f, NHDIM*obj->nsite*sizeof(double));
      copyToTarget(&obj->target->nsite, &obj->nsite, sizeof(int));
      break;
    case cudaMemcpyDeviceToHost:
      copyFromTarget(obj->f, tmpf, NHDIM*obj->nsite*sizeof(double));
      copyFromTarget(obj->u, tmpu, NHDIM*obj->nsite*sizeof(double));
      break;
    default:
      pe_fatal(obj->pe, "Bad flag in hydro_memcpy\n");
    }
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_halo
 *
 *****************************************************************************/

__host__ int hydro_u_halo(hydro_t * obj) {

  assert(obj);

  hydro_halo_swap(obj, HYDRO_U_HALO_TARGET);

  return 0;
}

/*****************************************************************************
 *
 *  hydro_halo_swap
 *
 *****************************************************************************/

__host__ int hydro_halo_swap(hydro_t * obj, hydro_halo_enum_t flag) {

  double * data;

  assert(obj);

  switch (flag) {
  case HYDRO_U_HALO_HOST:
    halo_swap_host_rank1(obj->halo, obj->u, MPI_DOUBLE);
    break;
  case HYDRO_U_HALO_TARGET:
    copyFromTarget(&data, &obj->target->u, sizeof(double *));
    halo_swap_packed(obj->halo, data);
    break;
  default:
    assert(0);
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_init_io_info
 *
 *  There is no read for the velocity; this should come from the
 *  distribution.
 *
 *****************************************************************************/

__host__ int hydro_init_io_info(hydro_t * obj, int grid[3], int form_in,
				int form_out) {

  io_info_arg_t args;

  assert(obj);
  assert(grid);
  assert(obj->info == NULL);

  args.grid[X] = grid[X];
  args.grid[Y] = grid[Y];
  args.grid[Z] = grid[Z];

  io_info_create(obj->pe, obj->cs, &args, &obj->info);
  if (obj->info == NULL) pe_fatal(obj->pe, "io_info_create(hydro) failed\n");

  io_info_set_name(obj->info, "Velocity field");
  io_info_write_set(obj->info, IO_FORMAT_BINARY, hydro_u_write);
  io_info_write_set(obj->info, IO_FORMAT_ASCII, hydro_u_write_ascii);
  io_info_read_set(obj->info, IO_FORMAT_BINARY, hydro_u_read);
  io_info_read_set(obj->info, IO_FORMAT_ASCII, hydro_u_read_ascii);
  io_info_set_bytesize(obj->info, NHDIM*sizeof(double));

  io_info_format_set(obj->info, form_in, form_out);
  io_info_metadata_filestub_set(obj->info, "vel");

  return 0;
}

/*****************************************************************************
 *
 *  hydro_io_info
 *
 *****************************************************************************/

__host__ int hydro_io_info(hydro_t * obj, io_info_t ** info) {

  assert(obj);
  assert(obj->info); /* Should have been initialised */

  *info = obj->info;

  return 0;
}

/*****************************************************************************
 *
 *  hydro_f_local_set
 *
 *****************************************************************************/

__host__ __device__
int hydro_f_local_set(hydro_t * obj, int index, const double force[3]) {

  int ia;

  assert(obj);

  for (ia = 0; ia < 3; ia++) {
    obj->f[addr_rank1(obj->nsite, NHDIM, index, ia)] = force[ia];
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_f_local
 *
 *****************************************************************************/

__host__ __device__
int hydro_f_local(hydro_t * obj, int index, double force[3]) {

  int ia;

  assert(obj);

  for (ia = 0; ia < 3; ia++) {
    force[ia] = obj->f[addr_rank1(obj->nsite, NHDIM, index, ia)];
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_f_local_add
 *
 *  Accumulate (repeat, accumulate) the fluid force at site index.
 *
 *****************************************************************************/

__host__ __device__
int hydro_f_local_add(hydro_t * obj, int index, const double force[3]) {

  int ia;

  assert(obj);

  for (ia = 0; ia < 3; ia++) {
    obj->f[addr_rank1(obj->nsite, NHDIM, index, ia)] += force[ia]; 
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_set
 *
 *****************************************************************************/

__host__ __device__
int hydro_u_set(hydro_t * obj, int index, const double u[3]) {

  int ia;

  assert(obj);

  for (ia = 0; ia < 3; ia++) {
    obj->u[addr_rank1(obj->nsite, NHDIM, index, ia)] = u[ia];
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u
 *
 *****************************************************************************/

__host__ __device__
int hydro_u(hydro_t * obj, int index, double u[3]) {

  int ia;

  assert(obj);

  for (ia = 0; ia < 3; ia++) {
    u[ia] = obj->u[addr_rank1(obj->nsite, NHDIM, index, ia)];
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_zero
 *
 *****************************************************************************/

__host__ int hydro_u_zero(hydro_t * obj, const double uzero[3]) {

  dim3 nblk, ntpb;
  double * u = NULL;

  assert(obj);

  copyFromTarget(&u, &obj->target->u, sizeof(double *));

  kernel_launch_param(obj->nsite, &nblk, &ntpb);
  __host_launch(hydro_field_set, nblk, ntpb, obj->target, u, uzero);
  targetDeviceSynchronise();

  return 0;
}


/*****************************************************************************
 *
 *  hydro_f_zero
 *
 *****************************************************************************/

__host__ int hydro_f_zero(hydro_t * obj, const double fzero[3]) {

  dim3 nblk, ntpb;
  double * f;

  assert(obj);
  assert(obj->target);

  copyFromTarget(&f, &obj->target->f, sizeof(double *)); 

  kernel_launch_param(obj->nsite, &nblk, &ntpb);
  __host_launch(hydro_field_set, nblk, ntpb, obj->target, f, fzero);
  targetDeviceSynchronise();

  return 0;
}

/*****************************************************************************
 *
 *  hydro_field_set
 *
 *****************************************************************************/

static __global__
void hydro_field_set(hydro_t * hydro, double * field, const double z[NHDIM]) {

  int kindex;

  assert(hydro);
  assert(field);

  __target_simt_parallel_for(kindex, hydro->nsite, 1) {
    int ia;
    for (ia = 0; ia < NHDIM; ia++) {
      field[addr_rank1(hydro->nsite, NHDIM, kindex, ia)] = z[ia];
    }
  }

  return;
}

/*****************************************************************************
 *
 *  hydro_lees_edwards
 *
 *  Compute the 'look-across-the-boundary' values of the velocity field,
 *  and update the velocity buffer region accordingly.
 *
 *  The communication might be improved:
 *  - only one buffer either side of the planes needs to be set?
 *  - only one communication per y sub domain if more than one buffer?
 *
 *****************************************************************************/

__host__ int hydro_lees_edwards(hydro_t * obj) {

  int nhalo;
  int nlocal[3]; /* Local system size */
  int nxbuffer;  /* Buffer planes */
  int ib;        /* Index in buffer region */
  int ib0;       /* buffer region offset */
  int ic;        /* Index corresponding x location in real system */

  int jc, kc, ia, index0, index1, index2;

  double dy;     /* Displacement for current ic->ib pair */
  double fr;     /* Fractional displacement */
  int jdy;       /* Integral part of displacement */
  int j1, j2;    /* j values in real system to interpolate between */

  double ule[3]; /* +/- velocity jump at plane */

  assert(obj);

  if (obj->le == NULL) return 0;

  if (cart_size(Y) > 1) {
    hydro_lees_edwards_parallel(obj);
  }
  else {

    cs_nhalo(obj->cs, &nhalo);
    cs_nlocal(obj->cs, nlocal);
    lees_edw_nxbuffer(obj->le, &nxbuffer);

    ib0 = nlocal[X] + nhalo + 1;

    for (ib = 0; ib < nxbuffer; ib++) {

      ic = lees_edw_ibuff_to_real(obj->le, ib);
      lees_edw_buffer_du(obj->le, ib, ule);

      lees_edw_buffer_dy(obj->le, ib, 1.0, &dy);
      dy = fmod(dy, L(Y));
      jdy = floor(dy);
      fr  = dy - jdy;

      for (jc = 1 - nhalo; jc <= nlocal[Y] + nhalo; jc++) {

	/* Actually required here is j1 = jc - jdy - 1, but there's
	 * horrible modular arithmetic for the periodic boundaries
	 * to ensure 1 <= j1,j2 <= nlocal[Y] */

	j1 = 1 + (jc - jdy - 2 + 2*nlocal[Y]) % nlocal[Y];
	j2 = 1 + j1 % nlocal[Y];

	/* If nhcomm < nhalo, we could use nhcomm here in the kc loop.
	 * (As j1 and j2 are always in the domain proper, jc can use nhalo.) */

	/* Note +/- nhcomm */
	for (kc = 1 - obj->nhcomm; kc <= nlocal[Z] + obj->nhcomm; kc++) {
	  index0 = lees_edw_index(obj->le, ib0 + ib, jc, kc);
	  index1 = lees_edw_index(obj->le, ic, j1, kc);
	  index2 = lees_edw_index(obj->le, ic, j2, kc);
	  for (ia = 0; ia < 3; ia++) {
	    obj->u[addr_rank1(obj->nsite, NHDIM, index0, ia)] = ule[ia] +
	      obj->u[addr_rank1(obj->nsite, NHDIM, index1, ia)]*fr +
	      obj->u[addr_rank1(obj->nsite, NHDIM, index2, ia)]*(1.0 - fr);
	  }
	}
      }
    }
  }

  return 0;
}

/*****************************************************************************
 *
 *  hydro_lees_edwards_parallel
 *
 *  The Lees Edwards transformation for the velocity field in parallel.
 *  This is a linear interpolation.
 *
 *  Note that we communicate with up to 3 processors in each direction;
 *  this avoids having to update the halos completely.
 *
 *****************************************************************************/

static int hydro_lees_edwards_parallel(hydro_t * obj) {

  int nlocal[3];           /* Local system size */
  int noffset[3];          /* Local starting offset */
  int nxbuffer;            /* Number of buffer planes */
  int ib;                  /* Index in buffer region */
  int ib0;                 /* buffer region offset */
  int ic;                  /* Index corresponding x location in real system */
  int jc, kc, j1, j2;
  int n1, n2, n3;
  double dy;               /* Displacement for current ic->ib pair */
  double fr;               /* Fractional displacement */
  int jdy;                 /* Integral part of displacement */
  int index, ia;
  int nhalo;
  double ule[3];

  int nsend;
  int nrecv;
  int      nrank_s[3];     /* send ranks */
  int      nrank_r[3];     /* recv ranks */
  const int tag0 = 1256;
  const int tag1 = 1257;
  const int tag2 = 1258;

  double * sbuf = NULL;   /* Send buffer */
  double * rbuf = NULL;   /* Interpolation buffer */

  MPI_Comm    le_comm;
  MPI_Request request[6];
  MPI_Status  status[3];

  assert(obj);

  cs_nhalo(obj->cs, &nhalo);
  cs_nlocal(obj->cs, nlocal);
  cs_nlocal_offset(obj->cs, noffset);
  ib0 = nlocal[X] + nhalo + 1;

  lees_edw_comm(obj->le, &le_comm);
  lees_edw_nxbuffer(obj->le, &nxbuffer);

  /* Allocate the temporary buffer */

  nsend = NHDIM*nlocal[Y]*(nlocal[Z] + 2*nhalo);
  nrecv = NHDIM*(nlocal[Y] + 2*nhalo + 1)*(nlocal[Z] + 2*nhalo);

  sbuf = (double *) calloc(nsend, sizeof(double));
  rbuf = (double *) calloc(nrecv, sizeof(double));
 
  if (sbuf == NULL) pe_fatal(obj->pe, "hydro: malloc(le sbuf) failed\n");
  if (rbuf == NULL) pe_fatal(obj->pe, "hydro: malloc(le rbuf) failed\n");


  /* One round of communication for each buffer plane */

  for (ib = 0; ib < nxbuffer; ib++) {

    ic = lees_edw_ibuff_to_real(obj->le, ib);
    lees_edw_buffer_du(obj->le, ib, ule);

    /* Work out the displacement-dependent quantities */

    lees_edw_buffer_dy(obj->le, ib, 1.0, &dy);
    dy = fmod(dy, L(Y));
    jdy = floor(dy);
    fr  = dy - jdy;

    /* First j1 required is j1 = jc - jdy - 1 with jc = 1 - nhalo.
     * Modular arithmetic ensures 1 <= j1 <= N_total(Y). */

    jc = noffset[Y] + 1 - nhalo;
    j1 = 1 + (jc - jdy - 2 + 2*N_total(Y)) % N_total(Y);

    lees_edw_jstart_to_mpi_ranks(obj->le, j1, nrank_s, nrank_r);

    /* Local quantities: given a local starting index j2, we receive
     * n1 + n2 sites into the buffer, and send n1 sites starting with
     * j2, and the remaining n2 sites from starting position nhalo. */

    j2 = 1 + (j1 - 1) % nlocal[Y];

    n1 = (nlocal[Y] - j2 + 1)*(nlocal[Z] + 2*nhalo);
    n2 = imin(nlocal[Y], j2 + 2*nhalo)*(nlocal[Z] + 2*nhalo);
    n3 = imax(0, j2 - nlocal[Y] + 2*nhalo)*(nlocal[Z] + 2*nhalo);

    assert((n1+n2+n3) == (nlocal[Y] + 2*nhalo + 1)*(nlocal[Z] + 2*nhalo));

    /* Post receives, sends and wait for receives. */

    MPI_Irecv(rbuf, NHDIM*n1, MPI_DOUBLE, nrank_r[0], tag0, le_comm, request);
    MPI_Irecv(rbuf + NHDIM*n1, NHDIM*n2, MPI_DOUBLE, nrank_r[1], tag1,
	      le_comm, request + 1);
    MPI_Irecv(rbuf + NHDIM*(n1 + n2), NHDIM*n3, MPI_DOUBLE, nrank_r[2], tag2,
	      le_comm, request + 2);

    /* Load send buffer */

    for (jc = 1; jc <= nlocal[Y]; jc++) {
      for (kc = 1 - nhalo; kc <= nlocal[Z] + nhalo; kc++) {
	index = lees_edw_index(obj->le, ic, jc, kc);
	for (ia = 0; ia < NHDIM; ia++) {
	  j1 = (jc - 1)*NHDIM*(nlocal[Z] + 2*nhalo) + NHDIM*(kc + nhalo - 1) + ia;
	  assert(j1 >= 0 && j1 < nsend);
	  sbuf[j1] = obj->u[addr_rank1(obj->nsite, NHDIM, index, ia)];
	}
      }
    }

    j1 = (j2 - 1)*NHDIM*(nlocal[Z] + 2*nhalo);
    MPI_Issend(sbuf + j1, NHDIM*n1, MPI_DOUBLE, nrank_s[0], tag0,
	       le_comm, request + 3);
    MPI_Issend(sbuf     , NHDIM*n2, MPI_DOUBLE, nrank_s[1], tag1,
	       le_comm, request + 4);
    MPI_Issend(sbuf     , NHDIM*n3, MPI_DOUBLE, nrank_s[2], tag2,
	       le_comm, request + 5);

    MPI_Waitall(3, request, status);

    /* Perform the actual interpolation from temporary buffer to
     * buffer region. */

    for (jc = 1 - nhalo; jc <= nlocal[Y] + nhalo; jc++) {

      j1 = (jc + nhalo - 1    )*(nlocal[Z] + 2*nhalo);
      j2 = (jc + nhalo - 1 + 1)*(nlocal[Z] + 2*nhalo);

      for (kc = 1 - nhalo; kc <= nlocal[Z] + nhalo; kc++) {
	index = lees_edw_index(obj->le, ib0 + ib, jc, kc);
	for (ia = 0; ia < NHDIM; ia++) {
	  obj->u[addr_rank1(obj->nsite, NHDIM, index, ia)] = ule[ia]
	    + fr*rbuf[NHDIM*(j1 + kc + nhalo - 1) + ia]
	    + (1.0 - fr)*rbuf[NHDIM*(j2 + kc + nhalo - 1) + ia];
	}
      }
    }

    MPI_Waitall(3, request + 3, status);
  }

  free(sbuf);
  free(rbuf);

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_write
 *
 *****************************************************************************/

static int hydro_u_write(FILE * fp, int index, void * arg) {

  int n;
  double u[3];
  hydro_t * obj = (hydro_t*) arg;

  assert(fp);
  assert(obj);

  hydro_u(obj, index, u);
  n = fwrite(u, sizeof(double), NHDIM, fp);
  if (n != NHDIM) fatal("fwrite(hydro->u) failed\n");

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_write_ascii
 *
 *****************************************************************************/

static int hydro_u_write_ascii(FILE * fp, int index, void * arg) {

  int n;
  double u[3];
  hydro_t * obj = (hydro_t *) arg;

  assert(fp);
  assert(obj);

  hydro_u(obj, index, u);

  n = fprintf(fp, "%22.15e %22.15e %22.15e\n", u[X], u[Y], u[Z]);

  /* Expect total of 69 characters ... */
  if (n != 69) fatal("fprintf(hydro->u) failed\n");

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_read
 *
 *****************************************************************************/

int hydro_u_read(FILE * fp, int index, void * self) {

  int n;
  double u[3];
  hydro_t * obj = (hydro_t *) self;

  assert(fp);
  assert(obj);

  n = fread(u, sizeof(double), NHDIM, fp);
  if (n != NHDIM) fatal("fread(hydro->u) failed\n");

  hydro_u_set(obj, index, u);

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_read_ascii
 *
 *****************************************************************************/

static int hydro_u_read_ascii(FILE * fp, int index, void * self) {

  int n;
  double u[3];
  hydro_t * obj = (hydro_t *) self;

  assert(fp);
  assert(obj);

  n = fscanf(fp, "%le %le %le", &u[X], &u[Y], &u[Z]);
  if (n != NHDIM) pe_fatal(obj->pe, "fread(hydro->u) failed\n");

  hydro_u_set(obj, index, u);

  return 0;
}

/*****************************************************************************
 *
 *  hydro_u_gradient_tensor
 *
 *  Return the velocity gradient tensor w_ab = d_b u_a at
 *  the site (ic, jc, kc).
 *
 *  The differencing is 2nd order centred.
 *
 *  This must take account of the Lees Edwards planes in  the x-direction.
 *
 *****************************************************************************/

__host__ int hydro_u_gradient_tensor(hydro_t * obj, int ic, int jc, int kc,
				     double w[3][3]) {

  int im1, ip1;
  double tr;

  assert(obj);

  im1 = lees_edw_ic_to_buff(obj->le, ic, -1);
  im1 = lees_edw_index(obj->le, im1, jc, kc);
  ip1 = lees_edw_ic_to_buff(obj->le, ic, +1);
  ip1 = lees_edw_index(obj->le, ip1, jc, kc);

  w[X][X] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, X)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, X)]);
  w[Y][X] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, Y)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, Y)]);
  w[Z][X] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, Z)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, Z)]);

  im1 = lees_edw_index(obj->le, ic, jc - 1, kc);
  ip1 = lees_edw_index(obj->le, ic, jc + 1, kc);

  w[X][Y] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, X)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, X)]);
  w[Y][Y] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, Y)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, Y)]);
  w[Z][Y] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, Z)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, Z)]);

  im1 = lees_edw_index(obj->le, ic, jc, kc - 1);
  ip1 = lees_edw_index(obj->le, ic, jc, kc + 1);

  w[X][Z] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, X)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, X)]);
  w[Y][Z] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, Y)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, Y)]);
  w[Z][Z] = 0.5*(obj->u[addr_rank1(obj->nsite, NHDIM, ip1, Z)] -
		 obj->u[addr_rank1(obj->nsite, NHDIM, im1, Z)]);

  /* Enforce tracelessness */

  tr = (1.0/3.0)*(w[X][X] + w[Y][Y] + w[Z][Z]);
  w[X][X] -= tr;
  w[Y][Y] -= tr;
  w[Z][Z] -= tr;

  return 0;
}

/*****************************************************************************
 *
 *  hydro_correct_momentum
 *
 *****************************************************************************/

__host__ int hydro_correct_momentum(hydro_t * hydro) {

  int ic, jc, kc, index;
  int nlocal[3];

  double f[3];
  double flocal[3] = {0.0, 0.0, 0.0};
  double fsum[3];
  double rv; 
  
  MPI_Comm comm;

  if (hydro == NULL) return 0;

  cs_nlocal(hydro->cs, nlocal);
  cs_cart_comm(hydro->cs, &comm);

  /* Compute force without correction. */
  
  for (ic = 1; ic <= nlocal[X]; ic++) {
    for (jc = 1; jc <= nlocal[Y]; jc++) {
      for (kc = 1; kc <= nlocal[Z]; kc++) {
    
        index = cs_index(hydro->cs, ic, jc, kc);
        hydro_f_local(hydro, index, f); 

        flocal[X] += f[X];
        flocal[Y] += f[Y];
        flocal[Z] += f[Z];
    
      }   
    }   
  }
    
  /* calculate the total force per fluid node */

  MPI_Allreduce(flocal, fsum, 4, MPI_DOUBLE, MPI_SUM, comm);

  rv = 1.0/(L(X)*L(Y)*L(Z));
  f[X] = -fsum[X]*rv;
  f[Y] = -fsum[Y]*rv;
  f[Z] = -fsum[Z]*rv;

  /* Now add correction */

  for (ic = 1; ic <= nlocal[X]; ic++) {
    for (jc = 1; jc <= nlocal[Y]; jc++) {
      for (kc = 1; kc <= nlocal[Z]; kc++) {

        index = cs_index(hydro->cs, ic, jc, kc);
        hydro_f_local_add(hydro, index, f); 

      }   
    }   
  }
  
  return 0;
}
