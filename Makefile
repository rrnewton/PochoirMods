TB_PP_FILE := tb_pochoir_pochoir.cpp
POCHOIR_HEADER := pochoir_array.hpp pochoir_walk.hpp pochoir_range.hpp pochoir_common.hpp pochoir_stencil.hpp pochoir_wrapper.hpp pochoir_iter.hpp pochoir_walk_recursive.hpp pochoir_walk_loops.hpp
PP_FILE := PMain.hs PMainParser.hs PBasicParser.hs PData.hs PParser2.hs
BLITZ_HOME := /home/yuantang/tool_dest/blitz_install
BLITZ_HEADER := ${BLITZ_HOME}/include/
BLITZ_LIB := ${BLITZ_HOME}/lib/
#OPT_FLAGS=-funroll-loops -fstrict-aliasing -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload -funsafe-loop-optimizations -Wunsafe-loop-optimizations -fdump-tree-original -fdump-tree-optimized
#OPT_FLAGS=-funroll-loops -fstrict-aliasing -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload -funsafe-loop-optimizations -Wunsafe-loop-optimizations
OPT_FLAGS=-funroll-loops -fstrict-aliasing -fomit-frame-pointer -fgcse-sm -fgcse-las -fgcse-after-reload -funsafe-loop-optimizations 
DEBUG := 0
GCC := /home/yuantang/tool_dest/GCC/gcc_4.5/bin/g++
CC=icc
ifeq (${CC}, icc)
ifeq (${DEBUG}, 1)
CFLAGS := -DDEBUG -O0 -g3 -std=c++0x -include cilk_stub.h -I${INTEL_CILK_HEADER}
#CFLAGS := -DDEBUG -O0 -g3 -std=c++0x -I${INTEL_CILK_HEADER}
else
# CFLAGS := -O3 -DNDEBUG -std=c++0x -Wall -Werror -xSSE4.2 -ipo -I${INTEL_CILK_HEADER} 
CFLAGS := -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo -I${INTEL_CILK_HEADER} 
endif
else
ifeq (${DEBUG}, 1)
CFLAGS := -DDEBUG -O0 -g3 -std=c++0x
else
CFLAGS := -O3 -DNDEBUG -std=c++0x ${OPT_FLAGS}
endif
endif
pochoir : ${PP_FILE} 
	ghc -o pochoir -O --make PMain.hs
pp_iter : pp_iter.cpp ${POCHOIR_HEADER} 
	./pp -split-obase pp_iter.cpp
	${CC} -o pp_iter ${CFLAGS} pp_iter.cpp 
	${CC} -o pp_iter_pochoir ${CFLAGS} pp_iter_pochoir.cpp 
tb_spec : tb_spec.cpp ${POCHOIR_HEADER} 
	${CC} -o tb_spec ${CFLAGS} tb_spec.cpp 
pp_col : pp_col.cpp ${POCHOIR_HEADER} 
	./pp -split-obase pp_col.cpp
	${CC} -o pp_col ${CFLAGS} pp_col.cpp 
	${CC} -o pp_col_pochoir ${CFLAGS} pp_col_pochoir.cpp 
3dfd : tb_3dfd.cpp ${POCHOIR_HEADER} 
	./pp -split-obase tb_3dfd.cpp
	${CC} -o 3dfd ${CFLAGS} tb_3dfd.cpp 
	${CC} -o 3dfd_pochoir ${CFLAGS} tb_3dfd_pochoir.cpp 
life : tb_life.cpp ${POCHOIR_HEADER} 
	./pp -split-obase tb_life.cpp
	${CC} -o life ${CFLAGS} tb_life.cpp 
	${CC} -o life_pochoir ${CFLAGS} tb_life_pochoir.cpp 
heat_2D : tb_heat_2D_NP.cpp ${POCHOIR_HEADER} 
	./pp -split-macro-shadow tb_heat_2D_NP.cpp
	${CC} -o heat_2D ${CFLAGS} tb_heat_2D_NP.cpp 
	${CC} -o heat_2D_pochoir ${CFLAGS} tb_heat_2D_NP_pochoir.cpp 
pp_spec : pp_spec.cpp ${POCHOIR_HEADER} 
	./pp -split-obase pp_spec.cpp
	${CC} -o pp_spec ${CFLAGS} pp_spec.cpp 
	${CC} -o pp_spec_pochoir ${CFLAGS} pp_spec_pochoir.cpp 
clean: 
	rm *.o
