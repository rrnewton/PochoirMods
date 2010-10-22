POCHOIR_LIB := /home/yuantang/Git/Pochoir/ExecSpec_refine/
POCHOIR_HEADER := expr_array.hpp expr_walk.hpp expr_range.hpp expr_common.hpp expr_stencil.hpp expr_wrapper.hpp expr_iter.hpp expr_walk_recursive.hpp expr_walk_loops.hpp
PP_FILE := PMain.hs PMainParser.hs PBasicParser.hs PData.hs PParser2.hs
#OPT_FLAGS=-funroll-loops -fstrict-aliasing -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload -funsafe-loop-optimizations -Wunsafe-loop-optimizations -fdump-tree-original -fdump-tree-optimized
#OPT_FLAGS=-funroll-loops -fstrict-aliasing -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload -funsafe-loop-optimizations -Wunsafe-loop-optimizations
OPT_FLAGS=-funroll-loops -fstrict-aliasing -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload -funsafe-loop-optimizations 
DEBUG := 0
GCC := /home/yuantang/tool_dest/GCC/gcc_4.5/bin/g++
CC=icc
ifeq (${CC}, icc)
ifeq (${DEBUG}, 1)
CFLAGS := -DDEBUG -O0 -g3 -std=c++0x -include cilk_stub.h -I${INTEL_CILK_HEADER} -I${POCHOIR_LIB}
#CFLAGS := -DDEBUG -O0 -g3 -std=c++0x -I${INTEL_CILK_HEADER}
else
# CFLAGS := -O3 -DNDEBUG -std=c++0x -Wall -Werror -xSSE4.2 -ipo -I${INTEL_CILK_HEADER} 
CFLAGS := -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo -I${INTEL_CILK_HEADER} -I${POCHOIR_LIB}
endif
else
ifeq (${DEBUG}, 1)
CFLAGS := -DDEBUG -O0 -g3 -std=c++0x
else
CFLAGS := -O3 -DNDEBUG -std=c++0x ${OPT_FLAGS}
endif
endif
pp : ${PP_FILE} 
	ghc -o pp -O --make PMain.hs
3dfd : tb_3dfd.cpp
	./pp tb_3dfd.cpp
	${CC} -o 3dfd ${CFLAGS} tb_3dfd.cpp
	${CC} -o 3dfd_pochoir ${CFLAGS} tb_3dfd_pochoir.cpp
life : tb_life.cpp
	./pp tb_life.cpp
	${CC} -o life ${CFLAGS} tb_life.cpp
	${CC} -o life_pochoir ${CFLAGS} tb_life_pochoir.cpp
heat_P : tb_heat_2D_P.cpp
	./pp tb_heat_2D_P.cpp
	${CC} -o heat_2D_P ${CFLAGS} tb_heat_2D_P.cpp
	${CC} -o heat_2D_P_pochoir ${CFLAGS} tb_heat_2D_P_pochoir.cpp
heat_NP : tb_heat_2D_NP.cpp
	./pp tb_heat_2D_NP.cpp
	${CC} -o heat_2D_NP ${CFLAGS} tb_heat_2D_NP.cpp
	${CC} -o heat_2D_NP_pochoir ${CFLAGS} tb_heat_2D_NP_pochoir.cpp
clean: 
	rm *.o
