SHELL = /bin/bash

CC = ./pochoir
ICC = icc

# CFLAGS = -restrict -O3 -no-prec-div -I$(ICC_INSTALL_DIR)/include -L$(ICC_INSTALL_DIR)/lib -lm -std=c99
CFLAGS = -restrict -no-prec-div 
#debugging
#POFLAGS = -debug
DEBUGFLAGS = -g3 -restrict -O0 -no-prec-div -std=c++0x
POFLAGS = -O3 -unroll-aggressive -funroll-loops -scalar-rep -ipo -xHOST -fno-alias -std=c++0x
REPORT_FLAGS = -par-report3 -openmp-report1 -vec-report3
MY_OPTS = -DMYOPT

DEFAULT_OPTIONS = $(CFLAGS) $(REPORT_FLAGS) $(MY_OPTS)
OMP_OPTIONS = -D_OPENMP -openmp
NOVEC_OPTIONS = -no-vec
VEC_OPTIONS = -DVECTORIZE
SOA_OPTIONS = -DSOA

TBB_INSTALL_DIR = /opt/intel/tbb/latest
TBB_OPTIONS = -I$(TBB_INSTALL_DIR)/include -L$(TBB_INSTALL_DIR)/lib/intel64 -ltbb
all: lbm_orig_novec.exe lbm_orig_vec.exe lbm_omp_novec.exe lbm_omp_vec.exe lbm_cilk_co_novec.exe lbm_cilk_co_vec.exe lbm_tbb_co_novec.exe lbm_tbb_co_vec.exe lbm_orig_soa_novec.exe lbm_orig_soa_vec.exe lbm_omp_soa_novec.exe lbm_omp_soa_vec.exe lbm_cilk_co_soa_novec.exe lbm_cilk_co_soa_vec.exe lbm_tbb_co_soa_novec.exe lbm_tbb_co_soa_vec.exe

INCLUDE := lbm_1d_array.h config.h lbm.h lbm_co.h main.h 
ORIG_SRC := main_orig.c lbm_orig.c
CILK_CO_SRC := main_co.cpp lbm_co.c
TBB_CO_SRC := main_co_tbb.cpp lbm_co.c
PO_SRC := main_po.cpp lbm_po.c
XO_SRC := main_xo.cpp lbm_xo.c
POCHOIR_SRC := main_tang.cpp lbm_tang.cpp

lbm_orig_novec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_orig_soa_novec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_orig_vec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_orig_soa_vec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_omp_novec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(OMP_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_omp_soa_novec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(OMP_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_omp_vec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(OMP_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_omp_soa_vec.exe: $(INCLUDE) $(ORIG_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(OMP_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(ORIG_SRC) >& COMPILE_LOG/$@.report

lbm_cilk_co_novec.exe: $(INCLUDE) $(CILK_CO_SRC)
#	$(ICC) $(DEBUGFLAGS) $(NOVEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) 
	$(CC) $(DEFAULT_OPTIONS) $(NOVEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) 
#	$(ICC) $(DEFAULT_OPTIONS) $(NOVEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) >& COMPILE_LOG/$@.report

lbm_cilk_co_soa_novec.exe: $(INCLUDE) $(CILK_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(NOVEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) 
#	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(NOVEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) >& COMPILE_LOG/$@.report

lbm_cilk_co_vec.exe: $(INCLUDE) $(CILK_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(VEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) 
#	$(ICC) $(DEFAULT_OPTIONS) $(VEC_OPTIONS) -I${CILK_HEADER_PATH} -o BIN/$@ $(CILK_CO_SRC) >& COMPILE_LOG/$@.report

lbm_cilk_co_soa_vec.exe: $(INCLUDE) $(CILK_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(CILK_CO_SRC) >& COMPILE_LOG/$@.report

lbm_tbb_co_novec.exe: $(INCLUDE) $(TBB_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(TBB_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(TBB_CO_SRC) >& COMPILE_LOG/$@.report

lbm_tbb_co_soa_novec.exe: $(INCLUDE) $(TBB_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(TBB_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(TBB_CO_SRC) >& COMPILE_LOG/$@.report

lbm_tbb_co_vec.exe: $(INCLUDE) $(TBB_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(TBB_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(TBB_CO_SRC) >& COMPILE_LOG/$@.report

lbm_tbb_co_soa_vec.exe: $(INCLUDE) $(TBB_CO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(SOA_OPTIONS) $(TBB_OPTIONS) $(VEC_OPTIONS) -o BIN/$@ $(TBB_CO_SRC) >& COMPILE_LOG/$@.report

lbm_po_novec.exe: $(INCLUDE) $(PO_SRC)
	$(ICC) $(DEFAULT_OPTIONS) $(NOVEC_OPTIONS) -o BIN/$@ $(PO_SRC) >& COMPILE_LOG/$@.report

lbm_xo_novec.exe: $(INCLUDE) $(XO_SRC)
	./pochoir $(POFLAGS) -c lbm_xo.cpp -std=c++0x
	$(ICC) $(DEFAULT_OPTIONS) $(NOVEC_OPTIONS) -DNDEBUG -std=c++0x -Wall -Werror -o BIN/$@ main_xo.cpp lbm_xo_pochoir.cpp >& COMPILE_LOG/$@.report

lbm_tang: $(INCLUDE) $(POCHOIR_SRC)
	${CC} ${POFLAGS} ${DEFAULT_OPTIONS} ${NOVEC_OPTIONS} -DNDEBUG -Wall -Werror -o BIN/$@ ${POCHOIR_SRC}
##	${CC} ${DEBUGFLAGS} -debug ${NOVEC_OPTIONS} -DDEBUG -Wall -Werror -o BIN/$@ main_xxoo.cpp lbm_xxoo.cpp
##	./pochoir $(POFLAGS) -c lbm_xxoo.cpp
##	$(ICC) $(DEFAULT_OPTIONS) $(NOVEC_OPTIONS) -DNDEBUG -std=c++0x -Wall -Werror -o BIN/$@ main_xxoo.cpp lbm_xxoo_pochoir.cpp 

clean:
	-rm *.o BIN/* COMPILE_LOG/* OUT/*

## You need to provide at least 3 more targets (xtune_clean, xtune_build, xtune_test) for using XTune
xtune_clean: clean

xtune_build: lbm_cilk_co_vec.exe 

xtune_test: 
	./run_ref.csh lbm_cilk_co_vec.exe 8 $@.out
#	./run_train.csh lbm_cilk_co_vec.exe 8 $@.out
