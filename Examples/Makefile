all : clean 3dfd life heat_P heat_NP
3dfd : tb_3dfd.cpp
	./pochoir tb_3dfd.cpp
	./tb_3dfd_pochoir
klein : tb_life_klein.cpochoir tb_life_klein_pochoir.cpp
#	./pochoir tb_life.cpp
	${CC} -o klein ${CFLAGS} tb_life_klein.cpochoir
	${CC} -o klein_pochoir ${CFLAGS} tb_life_klein_pochoir.cpochoir
life : tb_life.cpp
	./pochoir tb_life.cpp
	./tb_life_pochoir
heat_P : tb_heat_2D_P.cpp
	./pochoir tb_heat_2D_P.cpp
	./tb_heat_2D_P_pochoir
heat_NP : tb_heat_2D_NP.cpp
	./pochoir tb_heat_2D_NP.cpp
	./tb_heat_2D_NP_pochoir
clean: 
	rm -f *.o *.i
	rm -f ./tb_3dfd_pochoir ./tb_life_pochoir ./tb_heat_2D_NP_pochoir ./tb_heat_2D_P_pochoir
