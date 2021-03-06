###############################################################################
#
# Makefile for LUDWIG
#
# Preprocessor flags.
#
# Please use the following for any production run
#
# -DNDEBUG               Switch off standard C assert()
#                        The assertions can make the code much slower.
#
# make serial            for serial code (default model is D3Q19)
# make mpi               for parallel code (default model is D3Q19)
#
# make serial-d2q9       etc for serial D2Q9 or D3Q15 or D3Q19
# make mpi-d2q9          etc for parallel ditto
#
# Compiler flags.
#
# Appropriate flags for your system should be set in the parent
# directory in the file config.mk. See ../Makefile.mk
#
# Edinburgh Soft Matter and Statistical Physics Group and
# Edinburgh Parallel Computing Centre
#
# Kevin Stratford (kevin@epcc.ed.ac.uk)
# (c) 2010-2018 The University of Edinburgh
#
###############################################################################

include ../Makefile.mk

MAIN = main
EXECUTABLE = Ludwig.exe
LIBRARY = libludwig.a

OPTS = -DNP_D3Q6
LIBS = -L../target -ltarget -lm
INC = -I. -I ../target

###############################################################################
#
# Compilation with PETSc is enabled by setting HAVE_PETSC.
#
# For explicit libraries and includes the correct paths are obtained with
# 'make getincludedirs' and 'make getlinklibs' in $PETSC_DIR.
#
###############################################################################

HAVE_PETSC =

ifdef HAVE_PETSC
OPTS += -DPETSC
INC  += $(PETSC_INC)
LIBS += $(PETSC_LIB)
endif 

###############################################################################
#
#  Files
#
###############################################################################

OBJS = \
     coords.o leesedwards.o \
     physics.o physics_rt.o advection.o advection_rt.o advection_bcs.o \
     blue_phase.o blue_phase_init.o blue_phase_rt.o \
     blue_phase_beris_edwards.o \
     brazovskii.o brazovskii_rt.o \
     colloid_io.o colloids_init.o \
     colloid.o colloid_link.o colloids_halo.o colloid_io_rt.o \
     colloid_sums.o bbl.o build.o collision.o collision_rt.o \
     colloids.o colloids_rt.o lubrication.o \
     coords_field.o coords_rt.o \
     control.o distribution_rt.o \
     driven_colloid.o driven_colloid_rt.o \
     ewald.o field.o field_grad.o \
     field_phi_init.o field_phi_init_rt.o \
     fe_electro.o fe_electro_symmetric.o fe_lc_stats.o \
     gradient_rt.o \
     gradient_2d_5pt_fluid.o gradient_2d_tomita_fluid.o \
     gradient_3d_7pt_fluid.o gradient_3d_7pt_solid.o \
     gradient_3d_27pt_fluid.o gradient_3d_27pt_solid.o \
     halo_swap.o hydro.o hydro_rt.o interaction.o io_harness.o \
     kernel.o leesedwards_rt.o leslie_ericksen.o \
     lc_droplet.o lc_droplet_rt.o memory.o model.o model_le.o map.o \
     noise.o pair_lj_cut.o pair_ss_cut.o pair_yukawa.o \
     angle_cosine.o bond_fene.o \
     phi_cahn_hilliard.o phi_force.o phi_force_colloid.o \
     phi_force_stress.o phi_lb_coupler.o \
     phi_stats.o \
     polar_active.o polar_active_rt.o \
     psi.o psi_rt.o psi_stats.o psi_sor.o psi_init.o \
     psi_force.o psi_colloid.o propagation.o \
     nernst_planck.o \
     psi_petsc.o psi_gradients.o \
     pe.o pe_fenv.o ran.o runtime.o \
     surfactant.o surfactant_rt.o \
     symmetric_rt.o subgrid.o \
     stats_calibration.o stats_colloid.o \
     stats_distribution.o stats_free_energy.o stats_rheology.o \
     stats_sigma.o stats_symmetric.o \
     stats_surfactant.o stats_turbulent.o stats_velocity.o  \
     symmetric.o timer.o util.o wall.o wall_rt.o wall_ss_cut.o ludwig.o


###############################################################################
#
#  Targets
#
#  Typing "make" will give some instructions.
#
###############################################################################

default:
	$(MAKE) -s verbose

verbose:
	echo -e "LUDWIG make options ...\n"\
	    "\t make serial \t\t builds serial implementation (D3Q19)\n"\
	    "\t make serial-d2q9 etc \t builds specific serial version\n" \
	    "\t make mpi \t\t builds MPI implementation (D3Q19)\n" \
	    "\t make mpi-d2q9 etc \t builds specific parallel version\n" \
            "\t Check the Makefile for further details\n"	

code:	$(MAIN).o
	$(CC) $(LDFLAGS) -o $(EXECUTABLE) $(MAIN).o $(LIBRARY) $(LIBS)

# Serial versions

serial:
	$(MAKE) serial-d3q19	

serial-d2q9:
	$(MAKE) serial-model "LB=-D_D2Q9_" "LBOBJ=d2q9.o"

serial-d3q15:
	$(MAKE) serial-model "LB=-D_D3Q15_" "LBOBJ=d3q15.o"

serial-d3q19:
	$(MAKE) serial-model "LB=-D_D3Q19_" "LBOBJ=d3q19.o"

serial-model:
	$(MAKE) lib
	$(MAKE) code "INC=$(INC) -I../mpi_s" "LIBS=$(LIBS) -L../mpi_s -lmpi"


# MPI versions

mpi:
	$(MAKE) mpi-d3q19

mpi-d2q9:
	$(MAKE) mpi-model "LB=-D_D2Q9_" "LBOBJ=d2q9.o"

mpi-d3q15:
	$(MAKE) mpi-model "LB=-D_D3Q15_" "LBOBJ=d3q15.o"

mpi-d3q19:
	$(MAKE) mpi-model "LB=-D_D3Q19_" "LBOBJ=d3q19.o"

mpi-model:
	$(MAKE) libmpi
	$(MAKE) code "CC=$(MPICC)" "INC=$(INC) $(MPI_INCL)" "LIBS=$(LIBS) $(MPI_LIBS)"

# Libraries

libar:	$(OBJS) $(LBOBJ) 
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS) $(LBOBJ)

lib:
	$(MAKE) libar "INC=$(INC) -I../mpi_s" "LIBS= -L../mpi_s -lmpi $(LIBS)"
libmpi:
	$(MAKE) libar "CC=$(MPICC)" "INC=$(INC) $(MPI_INCL)"

###############################################################################
#
#  Compilation rules
#
###############################################################################


#
#  Default rule for compilation in the current directory.
#
.c.o :
	$(CC) $(LB) $(OPTS) $(CFLAGS) $(INC) -c $?



.PHONY : clean
clean:
	rm -f d2q9.o d3q15.o d3q19.o
	rm -f $(OBJS) $(TARGETDP_OBJS) $(EXECUTABLE) $(LIBRARY) $(MAIN).o
