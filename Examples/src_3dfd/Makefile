CC = ./pochoir
ICC = icc
all : clean 3dfd life heat_P heat_NP
heat_NP : tb_heat_2D_NP.cpp
	${CC} -c -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_NP.cpp
	${CC} -o tb_heat_2D_NP_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_NP_pochoir.o
heat_P : tb_heat_2D_P.cpp
	${CC} -o tb_heat_2D_P_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_heat_2D_P.cpp
3dfd : tb_3dfd_pochoir.cpp
	${ICC} -o tb_3dfd_pochoir -O3 -DNDEBUG -std=c++0x -Wall -Werror -ipo tb_3dfd_pochoir.cpp
clean: 
	rm -f *.o *.i
	rm -f ./tb_3dfd_pochoir ./tb_life_pochoir ./tb_heat_2D_NP_pochoir ./tb_heat_2D_P_pochoir
