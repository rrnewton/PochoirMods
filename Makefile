POCHOIR_HEADER := pochoir.hpp pochoir_array.hpp pochoir_walk.hpp pochoir_range.hpp pochoir_common.hpp pochoir_wrapper.hpp pochoir_iter.hpp pochoir_walk_recursive.hpp pochoir_walk_loops.hpp
PP_FILE := PMain.hs PMainParser.hs PBasicParser.hs PData.hs PShow.hs PUtils.hs PParser2.hs
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
clean: 
	rm *.o
