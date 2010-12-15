CC = ./pochoir
LBM_INCLUDE = lbm_1d_array.h config.h lbm.h lbm_co.h main.h
LBM_XO_SRC = main_xo.cpp lbm_xo.cpp
ICC = icc
all : clean lbm 3dfd life heat_P heat_NP psa rna rna_struct
heat_NP : tb_heat_2D_NP.cpp
	${CC} -c -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_NP.cpp
	${CC} -o tb_heat_2D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_NP_pochoir.o
heat_P : tb_heat_2D_P.cpp
	${CC} -o tb_heat_2D_P_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_P.cpp
life : tb_life.cpp
	${CC} -o tb_life_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_life.cpp
rna : rna.cpp
	${CC} -o rna_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo rna.cpp
psa : psa.cpp
	${CC} -o psa_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo psa.cpp
rna_struct : rna_struct.cpp
	${CC} -o rna_struct_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo rna_struct.cpp
3dfd : tb_3dfd.cpp
	${CC} -o tb_3dfd_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_3dfd.cpp
lbm : main_xo.cpp lbm_xo.cpp 
	$(CC) -DNDEBUG -std=c++0x -Wall -Werror -o lbm main_xo.cpp lbm_xo.cpp 
klein : tb_life_klein.cpp tb_life_klein_pochoir.cpp
#	./pochoir tb_life.cpp
	${CC} -o klein ${CFLAGS} tb_life_klein.cpochoir
	${CC} -o klein_pochoir ${CFLAGS} tb_life_klein_pochoir.cpochoir
clean: 
	rm -f *.o *.i
	rm -f ./tb_3dfd_pochoir ./tb_life_pochoir ./tb_heat_2D_NP_pochoir ./tb_heat_2D_P_pochoir
