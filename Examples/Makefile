CC = ./pochoir
ICC = icc
all : clean klein 3dfd life heat_P heat_NP psa rna rna_struct
heat_NP : tb_heat_2D_NP.cpp
#	${CC} -o tb_heat_2D_NP_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -debug -g3 -ipo -split-macro-shadow tb_heat_2D_NP.cpp
	${CC} -o tb_heat_2D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_NP.cpp
heat_P : tb_heat_2D_P.cpp
#	${CC} -o tb_heat_2D_P_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_P.cpp
	${CC} -o tb_heat_2D_P_pochoir -O0 -g3 -DDEBUG -std=c++0x -Wall -Werror -ipo -debug -split-macro-shadow tb_heat_2D_P.cpp
life : tb_life.cpp
	${CC} -o tb_life_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_life.cpp
rna : rna.cpp
	${CC} -o rna_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo rna.cpp
psa : psa.cpp
	${CC} -o psa_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo psa.cpp
psa_struct : psa_struct.cpp
	${CC} -o psa_struct_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo psa_struct.cpp
rna_struct : rna_struct.cpp
	${CC} -o rna_struct_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo rna_struct.cpp
3dfd : tb_3dfd.cpp
	${CC} -o tb_3dfd_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_3dfd.cpp
klein : tb_life_klein.cpp
	${CC} -o klein_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_life_klein.cpp
#	${CC} -o klein_pochoir -O0 -DDEBUG -std=c++0x -Wall -Werror -ipo -g3 -debug -split-macro-shadow tb_life_klein.cpp
clean: 
	rm -f *.o *.i
	rm -f ./tb_3dfd_pochoir ./tb_life_pochoir ./tb_heat_2D_NP_pochoir ./tb_heat_2D_P_pochoir
