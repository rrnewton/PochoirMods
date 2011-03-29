#!/bin/bash
CC = ./pochoir
ICC = icpc
#ICC_FLAGS = -ipo -unroll-aggressive -xHOST -simd
<<<<<<< .mine
ICC_FLAGS = -unroll-aggressive -ipo -xHOST -fno-alias -fno-fnalias 
=======
#ICC_FLAGS = -unroll-aggressive -xHOST 
ICC_FLAGS = -unroll-aggressive -fno-fnalias -fno-alias -xHost 
>>>>>>> .r20875
all : clean klein 3dfd life heat_P heat_NP psa rna rna_struct
loop_iter : tb_loop_iter.cpp
	${ICC} -o loop_iter -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_loop_iter.cpp
loop : tb_loop.cpp
	${ICC} -o loop -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_loop.cpp
#	${ICC} -o loop -O0 -DDEBUG -std=c++0x -Wall -Werror -g3 tb_loop.cpp
#	${CC} -o tb_loop_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_loop.cpp
heat_NP_zero : tb_heat_2D_NP_zero.cpp tb_heat_2D_NP_zero_pochoir.cpp tb_heat_2D_NP_zero_ploop.cpp
	${CC} -o heat_sim -O3 -DDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_zero.cpp
#	${CC} -o tb_heat_2D_NP_zero_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_zero_pochoir.cpp
#	${CC} -o tb_heat_2D_NP_zero_ploop -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_zero_ploop.cpp
heat_NP_loop : tb_heat_2D_NP_loop.cpp
	${CC} -o tb_heat_2D_NP_loop_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_loop.cpp
##	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_loop_pochoir.cpp
##	perl repair.pl tb_heat_2D_NP_loop_pochoir.s > heat_2D_NP_loop.s
##	${ICC} -o heat_2D_NP_loop heat_2D_NP_loop.s
berkeley3d27pt : tb_3d27pt.cpp
	${CC} -o tb_3d27pt -O4 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3d27pt.cpp
heat_NP : tb_heat_2D_NP.cpp
	${CC} -o tb_heat_2D_NP_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP.cpp
##	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_NP_pochoir.cpp
##	perl repair.pl tb_heat_2D_NP_pochoir.s > heat_2D_NP_seq.s
##	${ICC} -o heat_2D_NP_seq heat_2D_NP_seq.s
heat_P_loop : tb_heat_2D_P_loop.cpp
	${CC} -o tb_heat_2D_P_loop_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_P_loop.cpp
##	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_P_loop_pochoir.cpp
##	perl repair.pl tb_heat_2D_P_loop_pochoir.s > heat_2D_P_loop.s
##	${ICC} -o heat_2D_P_loop heat_2D_P_loop.s
heat_P : tb_heat_2D_P.cpp
	${ICC} -o tb_heat_2D_P_raw_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_P_pochoir.cpp
##	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_P_pochoir.cpp
##	perl repair.pl tb_heat_2D_P_pochoir.s > heat_2D_P_sim.s
##	${ICC} -o heat_2D_P_sim heat_2D_P_sim.s
#	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_2D_P_pochoir.cpp
#	perl repair.pl tb_heat_2D_P_pochoir.s >& heat_2D_P_seq.s
life_loop : tb_life_loop.cpp
	${CC} -o tb_life_loop_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life_loop.cpp
life_loop_bt : tb_life_loop_bt.cpp
	${CC} -o tb_life_loop_bt_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life_loop_bt.cpp
life_bt : tb_life_bt.cpp
	${CC} -o tb_life_bt_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life_bt.cpp
life : tb_life.cpp
	${CC} -o tb_life_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life.cpp
lcs : lcs.cpp
	${CC} -o lcs_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} lcs.cpp
rna : rna.cpp
	${CC} -o rna_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} rna.cpp
psa : psa.cpp
	${CC} -o psa_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} psa.cpp
psa_struct : psa_struct.cpp
	${CC} -o psa_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} psa_struct.cpp
rna_struct : rna_struct.cpp
	${CC} -o rna_struct_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} rna_struct.cpp
3dfd_loop : tb_3dfd_loop.cpp
	${CC} -o tb_3dfd_loop_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3dfd_loop.cpp
3dfd_seq : tb_3dfd_seq.cpp
	${CC} -o tb_3dfd_seq_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3dfd_seq.cpp
3dfd_sim : tb_3dfd_sim.cpp
		${CC} -o tb_3dfd_sim_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3dfd_sim.cpp
3dfd : tb_3dfd.cpp
#	${CC} -o tb_3dfd_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_3dfd.cpp
#
	${CC} -o tb_3dfd_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3dfd.cpp
#	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_3dfd_pochoir.cpp
#	perl repair.pl tb_3dfd_pochoir.s > 3dfd.s
#	${ICC} -o 3dfd_seq 3dfd.s
klein : tb_life_klein.cpp
	${CC} -o klein_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_life_klein.cpp
#	${CC} -o klein_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -g3 -debug -split-macro-shadow tb_life_klein.cpp
heat_3D_NP : tb_heat_3D_NP.cpp
	${CC} -o tb_heat_3D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_3D_NP.cpp
#	${CC} -o tb_heat_3D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo tb_heat_3D_NP.cpp
heat_4D_NP_loop : tb_heat_4D_NP_loop.cpp
	${CC} -o tb_heat_4D_NP_loop_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_4D_NP_loop.cpp
	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_4D_NP_loop_pochoir.cpp
	perl repair.pl tb_heat_4D_NP_loop_pochoir.s >& heat_4D_NP_loop.s
	${ICC} -o heat_4D_NP_loop heat_4D_NP_loop.s
heat_4D_NP : tb_heat_4D_NP.cpp
	${CC} -o tb_heat_4D_NP_spaa -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_4D_NP.cpp
	${ICC} -S -O3 -DNDEBUG -std=c++0x -Wall -Werror ${ICC_FLAGS} tb_heat_4D_NP_pochoir.cpp
	perl repair.pl tb_heat_4D_NP_pochoir.s >& heat_4D_NP_sim.s
	${ICC} -o heat_4D_NP_sim heat_4D_NP_sim.s
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
