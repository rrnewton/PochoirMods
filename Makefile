TB_PP_FILE := tb_expr_pochoir.cpp
POCHOIR_HEADER := expr_array.hpp expr_walk.hpp expr_range.hpp expr_common.hpp expr_stencil.hpp expr_wrapper.hpp expr_iter.hpp expr_walk_recursive.hpp expr_walk_loops.hpp
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
CFLAGS := -O3 -DNDEBUG -std=c++0x -Wall -Werror -xSSE4.2 -ipo -I${INTEL_CILK_HEADER} 
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
tb_spec : tb_spec.cpp ${POCHOIR_HEADER} 
	${CC} -o tb_spec ${CFLAGS} tb_spec.cpp 
pp_spec : pp_spec.cpp ${POCHOIR_HEADER} 
	./pp -split-shadow pp_spec.cpp
	${CC} -o pp_spec ${CFLAGS} pp_spec.cpp 
	${CC} -o pp_spec_pochoir ${CFLAGS} pp_spec_pochoir.cpp 
clean: 
	rm *.o
