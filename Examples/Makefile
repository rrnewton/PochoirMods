#!/bin/bash
CC = ./pochoir
ICC = icpc
# ICC_FLAGS = -unroll-aggressive -funroll-loops -scalar-rep -ipo -xHOST -fno-alias -fno-fnalias 
ICC_FLAGS = -unroll-aggressive -funroll-loops -ipo -xHOST -fno-alias -fno-fnalias 
all : clean klein 3dfd life heat_P heat_NP psa rna rna_struct
apop : apop.cpp
	${CC} -o apop -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} apop.cpp
apop_sloop : apop_sloop.cpp
	${CC} -o apop_sloop -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} apop_sloop.cpp
loop_iter : tb_loop_iter.cpp
	${ICC} -o loop_iter -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_loop_iter.cpp
loop : tb_loop.cpp
	${ICC} -o loop -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_loop.cpp
#	${ICC} -o loop -O0 -DDEBUG -std=c++0x -Wall -Werror -g3 tb_loop.cpp
#	${CC} -o tb_loop_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_loop.cpp
heat_NP_zero : tb_heat_2D_NP_zero.cpp 
	${CC} -o heat_zero -O3 -DDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_zero.cpp
#	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_zero_pochoir.cpp
#	perl repair.pl tb_heat_2D_NP_zero_pochoir.s > heat_2D_NP_zero.s
#	${ICC} -o heat_zero heat_2D_NP_zero.s
berkeley3d7pt : tb_3d7pt.cpp
	${CC} -o 3d7pt_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d7pt.cpp
#	${ICC} -o 3d7pt_pochoir -O3 -DNDEBUG -I${POCHOIR_LIB_PATH} -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d7pt.cpp
#	${ICC} -o 3d7pt_icc -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d7pt_pochoir.cpp
#	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d7pt_pochoir.cpp
#	perl repair.pl tb_3d7pt_pochoir.s > 3d7pt.s
#	${ICC} -o 3d7pt_s 3d7pt.s
berkeley3d27pt : tb_3d27pt.cpp
	${CC} -o 3d27pt_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d27pt.cpp
#	${ICC} -o 3d27pt_icc -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d27pt_pochoir.cpp
	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d27pt_pochoir.cpp
#	perl repair.pl tb_3d27pt_pochoir.s > 3d27pt.s
	${ICC} -o 3d27pt_s -O3 tb_3d27pt_pochoir.s
#	${ICC} -o 3d27pt_new_nopl 3d27pt.s
example : tb_example.cpp
#	${CC} -o tb_example_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_example.cpp
#	${ICC} -o tb_example_pochoir -O3 -I${POCHOIR_LIB_PATH} -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_example.cpp
	${ICC} -o tb_example_pochoir -O0 -g3 -I${POCHOIR_LIB_PATH} -DDEBUG -std=c++0x -Wall -Werror tb_example.cpp
heat_NP : tb_heat_2D_NP.cpp
	${CC} -o tb_heat_2D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP.cpp
#	${CC} -o tb_heat_2D_NP_pochoir -O3 -DDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} -split-c-pointer tb_heat_2D_NP.cpp
#	${ICC} -o tb_heat_2D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -opt-report 3 ${ICC_FLAGS} tb_heat_2D_NP_pochoir.cpp
#	${CC} -o tb_heat_2D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_2D_NP.cpp
heat_1D : tb_heat_1D_NP.cpp
	${CC} -o heat_1D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} -split-pointer tb_heat_1D_NP.cpp
heat_P : tb_heat_2D_P.cpp
	${CC} -o tb_heat_2D_P_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_P.cpp
#	${CC} -o tb_heat_2D_P_pochoir -O0 -g3 -DDEBUG -std=c++0x -Wall -Werror -ipo -debug tb_heat_2D_P.cpp
life : tb_life.cpp
	${CC} -o tb_life_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life.cpp
rna : rna.cpp
	${CC} -o rna_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} rna.cpp
	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} rna_pochoir.cpp
	perl repair.pl rna_pochoir.s > rna.s
	${ICC} -o rna rna.s
#	${CC} -o rna_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} -split-pointer rna.cpp
psa : psa.cpp
	${CC} -o psa_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} -split-pointer psa.cpp
psa_struct : psa_struct.cpp
	${CC} -o psa_struct_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} psa_struct.cpp
rna_struct : rna_struct.cpp
	${CC} -o rna_struct_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} rna_struct.cpp
3dfd : tb_3dfd.cpp
#	${CC} -o tb_3dfd_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_3dfd.cpp
	${CC} -o tb_3dfd_pochoir -O3 -DNDEBUG -std=c++0x -g3 -Wall -Werror ${ICC_FLAGS} tb_3dfd.cpp
	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3dfd_pochoir.cpp
	perl repair.pl tb_3dfd_pochoir.s > 3dfd.s
	${ICC} -o 3dfd 3dfd.s
klein : tb_life_klein.cpp
	${CC} -o klein_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life_klein.cpp
#	${CC} -o klein_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -g3 -debug -split-macro-shadow tb_life_klein.cpp
heat_3D_NP : tb_heat_3D_NP.cpp
	${CC} -o tb_heat_3D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_3D_NP.cpp
#	${CC} -o tb_heat_3D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_3D_NP.cpp
heat_4D_NP : tb_heat_4D_NP.cpp
	${CC} -o tb_heat_4D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_4D_NP.cpp
#	${CC} -o tb_heat_4D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_4D_NP.cpp
heat_5D_NP : tb_heat_5D_NP.cpp
	${CC} -o tb_heat_5D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_5D_NP.cpp
#	${CC} -o tb_heat_5D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_5D_NP.cpp
heat_6D_NP : tb_heat_6D_NP.cpp
	${CC} -o tb_heat_6D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_6D_NP.cpp
#	${CC} -o tb_heat_6D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_6D_NP.cpp
heat_7D_NP : tb_heat_7D_NP.cpp
	${CC} -o tb_heat_7D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_7D_NP.cpp
#	${CC} -o tb_heat_7D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_7D_NP.cpp
heat_8D_NP : tb_heat_8D_NP.cpp
	${CC} -o tb_heat_8D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_8D_NP.cpp
#	${CC} -o tb_heat_8D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_8D_NP.cpp
clean: 
	rm -f *.o *.i
	rm -f ./tb_3dfd_pochoir ./tb_life_pochoir ./tb_heat_2D_NP_pochoir ./tb_heat_2D_P_pochoir
