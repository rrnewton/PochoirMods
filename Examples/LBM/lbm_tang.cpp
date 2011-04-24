/* $Id: lbm.c,v 1.6 2004/05/03 08:23:51 pohlt Exp $ */

/*############################################################################*/

#include "lbm_tang.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <pochoir.hpp>

#if !defined(SPEC_CPU)
#ifdef _OPENMP
#include <omp.h>
#endif
#endif

#ifdef ALIGNED_MALLOC
#define MY_ALIGNMENT 16
#define MY_MALLOC(x) _mm_malloc((x), MY_ALIGNMENT)
#define MY_FREE(x)   _mm_free(x)
#else
#define MY_MALLOC(x) malloc(x)
#define MY_FREE(x)   free(x)
#endif

/*############################################################################*/

#define DFL1 (1.0/ 3.0)
#define DFL2 (1.0/18.0)
#define DFL3 (1.0/36.0)

/*############################################################################*/

void LBM_initializeGrid( Pochoir_Array_3D(PoCellEntry) & pa, const int t ) {
    for (int z = 0; z < SIZE_Z + 2 + MARGIN_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
    for (int x = 0; x < SIZE_X; ++x) {
        pa.interior(t, z, y, x)._C  = DFL1;
        pa.interior(t, z, y, x)._N  = DFL2;
        pa.interior(t, z, y, x)._S  = DFL2;
        pa.interior(t, z, y, x)._E  = DFL2;
        pa.interior(t, z, y, x)._W  = DFL2;
        pa.interior(t, z, y, x)._T  = DFL2;
        pa.interior(t, z, y, x)._B  = DFL2;
        pa.interior(t, z, y, x)._NE = DFL3;
        pa.interior(t, z, y, x)._NW = DFL3;
        pa.interior(t, z, y, x)._SE = DFL3;
        pa.interior(t, z, y, x)._SW = DFL3;
        pa.interior(t, z, y, x)._NT = DFL3;
        pa.interior(t, z, y, x)._NB = DFL3;
        pa.interior(t, z, y, x)._ST = DFL3;
        pa.interior(t, z, y, x)._SB = DFL3;
        pa.interior(t, z, y, x)._ET = DFL3;
        pa.interior(t, z, y, x)._EB = DFL3;
        pa.interior(t, z, y, x)._WT = DFL3;
        pa.interior(t, z, y, x)._WB = DFL3;
        CLEAR_ALL_FLAGS_SWEEP(pa.interior(t, z, y, x)._FLAGS);
    }
        }
    }
}

/*############################################################################*/

void LBM_loadObstacleFile( Pochoir_Array_3D(PoCellEntry) & pa, const int t, const char* filename ) {
	FILE* file = fopen( filename, "rb" );

	for( int z = 0 + MARGIN_Z; z < SIZE_Z + MARGIN_Z; z++ ) {
		for( int y = 0; y < SIZE_Y; y++ ) {
			for( int x = 0; x < SIZE_X; x++ ) {
				if( fgetc( file ) != '.' ) 
                    SET_FLAG(pa.interior(t, z, y, x)._FLAGS, OBSTACLE);
			}
			fgetc( file );
		}
		fgetc( file );
	}

	fclose( file );
}

/*############################################################################*/

void LBM_initializeSpecialCellsForLDC( Pochoir_Array_3D(PoCellEntry) & pa, const int t ) {
	/*voption indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
//#pragma omp parallel for private( x, y )
#endif
#endif
	for( int z = 0; z < SIZE_Z + 2 + MARGIN_Z; z++ ) {
		for( int y = 0; y < SIZE_Y; y++ ) {
			for( int x = 0; x < SIZE_X; x++ ) {
				if( x == 0 || x == SIZE_X-1 ||
				    y == 0 || y == SIZE_Y-1 ||
				    z == MARGIN_Z || z == SIZE_Z - 1 + MARGIN_Z ) {
					SET_FLAG(pa.interior(t, z, y, x)._FLAGS, OBSTACLE);
				}
				else {
					if( (z == 1 + MARGIN_Z || z == SIZE_Z - 2 + MARGIN_Z) &&
					     x > 1 && x < SIZE_X-2 &&
					     y > 1 && y < SIZE_Y-2 ) {
                        SET_FLAG(pa.interior(t, z, y, x)._FLAGS, ACCEL);
					}
				}
			}
		}
	}
}

/*############################################################################*/

void LBM_initializeSpecialCellsForChannel( Pochoir_Array_3D(PoCellEntry) & pa, const int t ) {
	/*voption indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
//#pragma omp parallel for private( x, y )
#endif
#endif
	for( int z = -2 + MARGIN_Z; z < SIZE_Z + 2 + MARGIN_Z; z++ ) {
		for( int y = 0; y < SIZE_Y; y++ ) {
			for( int x = 0; x < SIZE_X; x++ ) {
				if( x == 0 || x == SIZE_X-1 ||
				    y == 0 || y == SIZE_Y-1 ) {
                    SET_FLAG(pa.interior(t, z, y, x)._FLAGS, OBSTACLE);

					if( (z == 0 + MARGIN_Z || z == SIZE_Z - 1 + MARGIN_Z ) &&
                        ! TEST_FLAG(pa(t, z, y, x)._FLAGS,  OBSTACLE))
                        SET_FLAG(pa.interior(t, z, y, x)._FLAGS, IN_OUT_FLOW);
				}
			}
		}
	}
}

/*############################################################################*/

void LBM_showGridStatistics( Pochoir_Array_3D(PoCellEntry) & pa, const int t ) {
	int nObstacleCells = 0,
	    nAccelCells    = 0,
	    nFluidCells    = 0;
	MY_TYPE ux, uy, uz;
	MY_TYPE minU2  = 1e+30, maxU2  = -1e+30, u2;
	MY_TYPE minRho = 1e+30, maxRho = -1e+30, rho;
	MY_TYPE mass = 0;

    for (int z = 0+MARGIN_Z; z < SIZE_Z + MARGIN_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
    for (int x = 0; x < SIZE_X; ++x) {
        rho = pa.interior(t, z, y, x)._C  + pa.interior(t, z, y, x)._N
            + pa.interior(t, z, y, x)._S  + pa.interior(t, z, y, x)._E
            + pa.interior(t, z, y, x)._W  + pa.interior(t, z, y, x)._T
            + pa.interior(t, z, y, x)._B  + pa.interior(t, z, y, x)._NE
            + pa.interior(t, z, y, x)._NW + pa.interior(t, z, y, x)._SE
            + pa.interior(t, z, y, x)._SW + pa.interior(t, z, y, x)._NT
            + pa.interior(t, z, y, x)._NB + pa.interior(t, z, y, x)._ST
            + pa.interior(t, z, y, x)._SB + pa.interior(t, z, y, x)._ET
            + pa.interior(t, z, y, x)._EB + pa.interior(t, z, y, x)._WT
            + pa.interior(t, z, y, x)._WB;
		if( rho < minRho ) minRho = rho;
		if( rho > maxRho ) maxRho = rho;
		mass += rho;

        if ( TEST_FLAG_SWEEP(pa.interior(t, z, y, x)._FLAGS, OBSTACLE) ) {
			nObstacleCells++;
		} else {
            if ( TEST_FLAG_SWEEP(pa.interior(t, z, y, x)._FLAGS, ACCEL) ) 
				nAccelCells++;
			else
				nFluidCells++;

            ux = + pa.interior(t, z, y, x)._E  - pa.interior(t, z, y, x)._W
                 + pa.interior(t, z, y, x)._NE - pa.interior(t, z, y, x)._NW
                 + pa.interior(t, z, y, x)._SE - pa.interior(t, z, y, x)._SW
                 + pa.interior(t, z, y, x)._ET + pa.interior(t, z, y, x)._EB
                 - pa.interior(t, z, y, x)._WT - pa.interior(t, z, y, x)._WB;
            uy = + pa.interior(t, z, y, x)._N  - pa.interior(t, z, y, x)._S
                 + pa.interior(t, z, y, x)._NE + pa.interior(t, z, y, x)._NW
                 - pa.interior(t, z, y, x)._SE - pa.interior(t, z, y, x)._SW
                 + pa.interior(t, z, y, x)._NT + pa.interior(t, z, y, x)._NB
                 - pa.interior(t, z, y, x)._ST - pa.interior(t, z, y, x)._SB;
			uz = + pa.interior(t, z, y, x)._T  - pa.interior(t, z, y, x)._B 
			     + pa.interior(t, z, y, x)._NT - pa.interior(t, z, y, x)._NB
			     + pa.interior(t, z, y, x)._ST - pa.interior(t, z, y, x)._SB
			     + pa.interior(t, z, y, x)._ET - pa.interior(t, z, y, x)._EB
			     + pa.interior(t, z, y, x)._WT - pa.interior(t, z, y, x)._WB;
			u2 = (ux*ux + uy*uy + uz*uz) / (rho*rho);
			if( u2 < minU2 ) minU2 = u2;
			if( u2 > maxU2 ) maxU2 = u2;
		}
    }
        }
    }

        printf( "LBM_showGridStatistics:\n"
        "\tnObstacleCells: %7i nAccelCells: %7i nFluidCells: %7i\n"
        "\tminRho: %8.4f maxRho: %8.4f mass: %e\n"
        "\tminU: %e maxU: %e\n\n",
        nObstacleCells, nAccelCells, nFluidCells,
        minRho, maxRho, mass,
        sqrt( minU2 ), sqrt( maxU2 ) );

}

/*############################################################################*/

void LBM_handleInOutFlow( Pochoir_Array_3D(PoCellEntry) & pa, const int t ) {
	MY_TYPE ux , uy , uz , rho ,
	       ux1, uy1, uz1, rho1,
	       ux2, uy2, uz2, rho2,
	       u2, px, py;

	/* inflow */
	/*voption indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
//#pragma omp parallel for private( ux, uy, uz, rho, ux1, uy1, uz1, rho1, \
                                  ux2, uy2, uz2, rho2, u2, px, py )
#endif
#endif
    for (int z = 0 + MARGIN_Z; z < 1 + MARGIN_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
    for (int x = 0; x < SIZE_X; ++x) {
		rho1 = + pa.interior(t-1, z + 1, y, x)._C  + pa.interior(t-1, z + 1, y, x)._N 
		       + pa.interior(t-1, z + 1, y, x)._S  + pa.interior(t-1, z + 1, y, x)._E 
		       + pa.interior(t-1, z + 1, y, x)._W  + pa.interior(t-1, z + 1, y, x)._T 
		       + pa.interior(t-1, z + 1, y, x)._B  + pa.interior(t-1, z + 1, y, x)._NE
		       + pa.interior(t-1, z + 1, y, x)._NW + pa.interior(t-1, z + 1, y, x)._SE
		       + pa.interior(t-1, z + 1, y, x)._SW + pa.interior(t-1, z + 1, y, x)._NT
		       + pa.interior(t-1, z + 1, y, x)._NB + pa.interior(t-1, z + 1, y, x)._ST
		       + pa.interior(t-1, z + 1, y, x)._SB + pa.interior(t-1, z + 1, y, x)._ET
		       + pa.interior(t-1, z + 1, y, x)._EB + pa.interior(t-1, z + 1, y, x)._WT
		       + pa.interior(t-1, z + 1, y, x)._WB;               
		rho2 = + pa.interior(t-1, z + 2, y, x)._C  + pa.interior(t-1, z + 2, y, x)._N 
		       + pa.interior(t-1, z + 2, y, x)._S  + pa.interior(t-1, z + 2, y, x)._E 
		       + pa.interior(t-1, z + 2, y, x)._W  + pa.interior(t-1, z + 2, y, x)._T 
		       + pa.interior(t-1, z + 2, y, x)._B  + pa.interior(t-1, z + 2, y, x)._NE
		       + pa.interior(t-1, z + 2, y, x)._NW + pa.interior(t-1, z + 2, y, x)._SE
		       + pa.interior(t-1, z + 2, y, x)._SW + pa.interior(t-1, z + 2, y, x)._NT
		       + pa.interior(t-1, z + 2, y, x)._NB + pa.interior(t-1, z + 2, y, x)._ST
		       + pa.interior(t-1, z + 2, y, x)._SB + pa.interior(t-1, z + 2, y, x)._ET
		       + pa.interior(t-1, z + 2, y, x)._EB + pa.interior(t-1, z + 2, y, x)._WT
		       + pa.interior(t-1, z + 2, y, x)._WB;

		rho = 2.0*rho1 - rho2;

		px = (x / (0.5*(SIZE_X-1))) - 1.0;
		py = (y / (0.5*(SIZE_Y-1))) - 1.0;
		ux = 0.00;
		uy = 0.00;
		uz = 0.01 * (1.0-px*px) * (1.0-py*py);

		u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

		pa.interior(t-1, z, y, x)._C  = DFL1*rho*(1.0                                 - u2);

		pa.interior(t-1, z, y, x)._N  = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
		pa.interior(t-1, z, y, x)._S  = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
		pa.interior(t-1, z, y, x)._E  = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
		pa.interior(t-1, z, y, x)._W  = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
		pa.interior(t-1, z, y, x)._T  = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
		pa.interior(t-1, z, y, x)._B  = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

		pa.interior(t-1, z, y, x)._NE = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._NW = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._SE = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._SW = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._NT = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._NB = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._ST = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._SB = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._ET = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._EB = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._WT = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._WB = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
    }
        }
    }

	/* out-1flow */
	/*vopt-1ion indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
//#pragma omp parallel for privat-1e( ux, uy, uz, rho, ux1, uy1, uz1, rho1, \
                                  ux2, uy2, uz2, rho2, u2, px, py )            
#endif
#endif

    for (int z = SIZE_Z - 1 + MARGIN_Z; z < SIZE_Z + MARGIN_Z; ++z) {
        for (int y = 0; y < SIZE_Y; ++y) {
    for (int x = 0; x < SIZE_X; ++x) {
		rho1 = + pa.interior(t-1, z-1, y, x)._C  + pa.interior(t-1, z-1, y, x)._N  
		       + pa.interior(t-1, z-1, y, x)._S  + pa.interior(t-1, z-1, y, x)._E  
		       + pa.interior(t-1, z-1, y, x)._W  + pa.interior(t-1, z-1, y, x)._T  
		       + pa.interior(t-1, z-1, y, x)._B  + pa.interior(t-1, z-1, y, x)._NE 
		       + pa.interior(t-1, z-1, y, x)._NW + pa.interior(t-1, z-1, y, x)._SE 
		       + pa.interior(t-1, z-1, y, x)._SW + pa.interior(t-1, z-1, y, x)._NT 
		       + pa.interior(t-1, z-1, y, x)._NB + pa.interior(t-1, z-1, y, x)._ST 
		       + pa.interior(t-1, z-1, y, x)._SB + pa.interior(t-1, z-1, y, x)._ET 
		       + pa.interior(t-1, z-1, y, x)._EB + pa.interior(t-1, z-1, y, x)._WT 
		       + pa.interior(t-1, z-1, y, x)._WB;
		ux1 = + pa.interior(t-1, z-1, y, x)._E  - pa.interior(t-1, z-1, y, x)._W 
		      + pa.interior(t-1, z-1, y, x)._NE - pa.interior(t-1, z-1, y, x)._NW
		      + pa.interior(t-1, z-1, y, x)._SE - pa.interior(t-1, z-1, y, x)._SW
		      + pa.interior(t-1, z-1, y, x)._ET + pa.interior(t-1, z-1, y, x)._EB
		      - pa.interior(t-1, z-1, y, x)._WT - pa.interior(t-1, z-1, y, x)._WB;
		uy1 = + pa.interior(t-1, z-1, y, x)._N  - pa.interior(t-1, z-1, y, x)._S 
		      + pa.interior(t-1, z-1, y, x)._NE + pa.interior(t-1, z-1, y, x)._NW
		      - pa.interior(t-1, z-1, y, x)._SE - pa.interior(t-1, z-1, y, x)._SW
		      + pa.interior(t-1, z-1, y, x)._NT + pa.interior(t-1, z-1, y, x)._NB
		      - pa.interior(t-1, z-1, y, x)._ST - pa.interior(t-1, z-1, y, x)._SB;
		uz1 = + pa.interior(t-1, z-1, y, x)._T  - pa.interior(t-1, z-1, y, x)._B 
		      + pa.interior(t-1, z-1, y, x)._NT - pa.interior(t-1, z-1, y, x)._NB
		      + pa.interior(t-1, z-1, y, x)._ST - pa.interior(t-1, z-1, y, x)._SB
		      + pa.interior(t-1, z-1, y, x)._ET - pa.interior(t-1, z-1, y, x)._EB
		      + pa.interior(t-1, z-1, y, x)._WT - pa.interior(t-1, z-1, y, x)._WB;

		ux1 /= rho1;
		uy1 /= rho1;
		uz1 /= rho1;

		rho2 = + pa.interior(t-1, z-2, y, x)._C  + pa.interior(t-1, z-2, y, x)._N 
		       + pa.interior(t-1, z-2, y, x)._S  + pa.interior(t-1, z-2, y, x)._E 
		       + pa.interior(t-1, z-2, y, x)._W  + pa.interior(t-1, z-2, y, x)._T 
		       + pa.interior(t-1, z-2, y, x)._B  + pa.interior(t-1, z-2, y, x)._NE
		       + pa.interior(t-1, z-2, y, x)._NW + pa.interior(t-1, z-2, y, x)._SE
		       + pa.interior(t-1, z-2, y, x)._SW + pa.interior(t-1, z-2, y, x)._NT
		       + pa.interior(t-1, z-2, y, x)._NB + pa.interior(t-1, z-2, y, x)._ST
		       + pa.interior(t-1, z-2, y, x)._SB + pa.interior(t-1, z-2, y, x)._ET
		       + pa.interior(t-1, z-2, y, x)._EB + pa.interior(t-1, z-2, y, x)._WT
		       + pa.interior(t-1, z-2, y, x)._WB;
		ux2 = + pa.interior(t-1, z-2, y, x)._E  - pa.interior(t-1, z-2, y, x)._W 
		      + pa.interior(t-1, z-2, y, x)._NE - pa.interior(t-1, z-2, y, x)._NW
		      + pa.interior(t-1, z-2, y, x)._SE - pa.interior(t-1, z-2, y, x)._SW
		      + pa.interior(t-1, z-2, y, x)._ET + pa.interior(t-1, z-2, y, x)._EB
		      - pa.interior(t-1, z-2, y, x)._WT - pa.interior(t-1, z-2, y, x)._WB;
		uy2 = + pa.interior(t-1, z-2, y, x)._N  - pa.interior(t-1, z-2, y, x)._S 
		      + pa.interior(t-1, z-2, y, x)._NE + pa.interior(t-1, z-2, y, x)._NW
		      - pa.interior(t-1, z-2, y, x)._SE - pa.interior(t-1, z-2, y, x)._SW
		      + pa.interior(t-1, z-2, y, x)._NT + pa.interior(t-1, z-2, y, x)._NB
		      - pa.interior(t-1, z-2, y, x)._ST - pa.interior(t-1, z-2, y, x)._SB;
		uz2 = + pa.interior(t-1, z-2, y, x)._T  - pa.interior(t-1, z-2, y, x)._B 
		      + pa.interior(t-1, z-2, y, x)._NT - pa.interior(t-1, z-2, y, x)._NB
		      + pa.interior(t-1, z-2, y, x)._ST - pa.interior(t-1, z-2, y, x)._SB
		      + pa.interior(t-1, z-2, y, x)._ET - pa.interior(t-1, z-2, y, x)._EB
		      + pa.interior(t-1, z-2, y, x)._WT - pa.interior(t-1, z-2, y, x)._WB;

		ux2 /= rho2;
		uy2 /= rho2;
		uz2 /= rho2;

		rho = 1.0;

		ux = 2*ux1 - ux2;
		uy = 2*uy1 - uy2;
		uz = 2*uz1 - uz2;

		u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

		pa.interior(t-1, z, y, x)._C  = DFL1*rho*(1.0                                 - u2);

		pa.interior(t-1, z, y, x)._N  = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
		pa.interior(t-1, z, y, x)._S  = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
		pa.interior(t-1, z, y, x)._E  = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
		pa.interior(t-1, z, y, x)._W  = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
		pa.interior(t-1, z, y, x)._T  = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
		pa.interior(t-1, z, y, x)._B  = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

		pa.interior(t-1, z, y, x)._NE = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._NW = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._SE = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._SW = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._NT = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._NB = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._ST = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._SB = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._ET = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._EB = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._WT = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
		pa.interior(t-1, z, y, x)._WB = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
    }
        }
    }
}

/*############################################################################*/

void LBM_performStreamCollide( Pochoir_Array_3D(PoCellEntry) & pa, const int t ) {
	MY_TYPE ux, uy, uz, u2, rho;

	/*voption indep*/
        for (int z = 0 + MARGIN_Z; z < SIZE_Z + MARGIN_Z; z++)
        {
            for (int y = 0; y < SIZE_Y; y++)
            {
                for (int x = 0; x < SIZE_X; x++)
                {
                    if ( TEST_FLAG_SWEEP(pa.interior(t-1, z, y, x)._FLAGS, OBSTACLE) ) {
                        pa.interior(t, z, y, x)._C      = pa.interior(t-1, z, y, x)._C ;
                        pa.interior(t, z, y-1, x)._S    = pa.interior(t-1, z, y, x)._N ;
                        pa.interior(t, z, y+1, x)._N    = pa.interior(t-1, z, y, x)._S ;
                        pa.interior(t, z, y, x-1)._W    = pa.interior(t-1, z, y, x)._E ;
                        pa.interior(t, z, y, x+1)._E    = pa.interior(t-1, z, y, x)._W ;
                        pa.interior(t, z-1, y, x)._B    = pa.interior(t-1, z, y, x)._T ;
                        pa.interior(t, z+1, y, x)._T    = pa.interior(t-1, z, y, x)._B ;
                        pa.interior(t, z, y-1, x-1)._SW = pa.interior(t-1, z, y, x)._NE;
                        pa.interior(t, z, y-1, x+1)._SE = pa.interior(t-1, z, y, x)._NW;
                        pa.interior(t, z, y+1, x-1)._NW = pa.interior(t-1, z, y, x)._SE;
                        pa.interior(t, z, y+1, x+1)._NE = pa.interior(t-1, z, y, x)._SW;
                        pa.interior(t, z-1, y-1, x)._SB = pa.interior(t-1, z, y, x)._NT;
                        pa.interior(t, z+1, y-1, x)._ST = pa.interior(t-1, z, y, x)._NB;
                        pa.interior(t, z-1, y+1, x)._NB = pa.interior(t-1, z, y, x)._ST;
                        pa.interior(t, z+1, y+1, x)._NT = pa.interior(t-1, z, y, x)._SB;
                        pa.interior(t, z-1, y, x-1)._WB = pa.interior(t-1, z, y, x)._ET;
                        pa.interior(t, z+1, y, x-1)._WT = pa.interior(t-1, z, y, x)._EB;
                        pa.interior(t, z-1, y, x+1)._EB = pa.interior(t-1, z, y, x)._WT;
                        pa.interior(t, z+1, y, x+1)._ET = pa.interior(t-1, z, y, x)._WB;
                        continue;
                    }

                    rho = + pa.interior(t-1, z, y, x)._C  + pa.interior(t-1, z, y, x)._N 
                          + pa.interior(t-1, z, y, x)._S  + pa.interior(t-1, z, y, x)._E 
                          + pa.interior(t-1, z, y, x)._W  + pa.interior(t-1, z, y, x)._T 
                          + pa.interior(t-1, z, y, x)._B  + pa.interior(t-1, z, y, x)._NE
                          + pa.interior(t-1, z, y, x)._NW + pa.interior(t-1, z, y, x)._SE
                          + pa.interior(t-1, z, y, x)._SW + pa.interior(t-1, z, y, x)._NT
                          + pa.interior(t-1, z, y, x)._NB + pa.interior(t-1, z, y, x)._ST
                          + pa.interior(t-1, z, y, x)._SB + pa.interior(t-1, z, y, x)._ET
                          + pa.interior(t-1, z, y, x)._EB + pa.interior(t-1, z, y, x)._WT
                          + pa.interior(t-1, z, y, x)._WB;

                    ux = + pa.interior(t-1, z, y, x)._E  - pa.interior(t-1, z, y, x)._W 
                         + pa.interior(t-1, z, y, x)._NE - pa.interior(t-1, z, y, x)._NW
                         + pa.interior(t-1, z, y, x)._SE - pa.interior(t-1, z, y, x)._SW
                         + pa.interior(t-1, z, y, x)._ET + pa.interior(t-1, z, y, x)._EB
                         - pa.interior(t-1, z, y, x)._WT - pa.interior(t-1, z, y, x)._WB;
                    uy = + pa.interior(t-1, z, y, x)._N  - pa.interior(t-1, z, y, x)._S 
                         + pa.interior(t-1, z, y, x)._NE + pa.interior(t-1, z, y, x)._NW
                         - pa.interior(t-1, z, y, x)._SE - pa.interior(t-1, z, y, x)._SW
                         + pa.interior(t-1, z, y, x)._NT + pa.interior(t-1, z, y, x)._NB
                         - pa.interior(t-1, z, y, x)._ST - pa.interior(t-1, z, y, x)._SB;
                    uz = + pa.interior(t-1, z, y, x)._T  - pa.interior(t-1, z, y, x)._B 
                         + pa.interior(t-1, z, y, x)._NT - pa.interior(t-1, z, y, x)._NB
                         + pa.interior(t-1, z, y, x)._ST - pa.interior(t-1, z, y, x)._SB
                         + pa.interior(t-1, z, y, x)._ET - pa.interior(t-1, z, y, x)._EB
                         + pa.interior(t-1, z, y, x)._WT - pa.interior(t-1, z, y, x)._WB;

                    ux /= rho;
                    uy /= rho;
                    uz /= rho;

                    if ( TEST_FLAG_SWEEP(pa.interior(t-1, z, y, x)._FLAGS, ACCEL) ) {
			            ux = 0.005;
			            uy = 0.002;
			            uz = 0.000;
                    }

                    u2 = 1.5 * (ux*ux + uy*uy + uz*uz);
                    pa.interior(t, z, y, x)._C  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._C  + DFL1*OMEGA*rho*(1.0                                 - u2);

                    pa.interior(t, z, y+1, x)._N  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._N  + DFL2*OMEGA*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
                    pa.interior(t, z, y-1, x)._S  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._S  + DFL2*OMEGA*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
                    pa.interior(t, z, y, x+1)._E  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._E  + DFL2*OMEGA*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
                    pa.interior(t, z, y, x-1)._W  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._W  + DFL2*OMEGA*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
                    pa.interior(t, z+1, y, x)._T  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._T  + DFL2*OMEGA*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
                    pa.interior(t, z-1, y, x)._B  = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._B  + DFL2*OMEGA*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

                    pa.interior(t, z, y+1, x+1)._NE = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._NE + DFL3*OMEGA*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
                    pa.interior(t, z, y+1, x-1)._NW = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._NW + DFL3*OMEGA*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
                    pa.interior(t, z, y-1, x+1)._SE = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._SE + DFL3*OMEGA*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
                    pa.interior(t, z, y-1, x-1)._SW = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._SW + DFL3*OMEGA*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
                    pa.interior(t, z+1, y+1, x)._NT = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._NT + DFL3*OMEGA*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
                    pa.interior(t, z-1, y+1, x)._NB = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._NB + DFL3*OMEGA*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
                    pa.interior(t, z+1, y-1, x)._ST = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._ST + DFL3*OMEGA*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
                    pa.interior(t, z-1, y-1, x)._SB = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._SB + DFL3*OMEGA*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
                    pa.interior(t, z+1, y, x+1)._ET = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._ET + DFL3*OMEGA*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
                    pa.interior(t, z-1, y, x+1)._EB = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._EB + DFL3*OMEGA*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
                    pa.interior(t, z+1, y, x-1)._WT = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._WT + DFL3*OMEGA*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
                    pa.interior(t, z-1, y, x-1)._WB = (1.0-OMEGA)*pa.interior(t-1, z, y, x)._WB + DFL3*OMEGA*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
                }
            }
        }
}


/*############################################################################*/

static void storeValue( FILE* file, OUTPUT_PRECISION* v ) {
	const int litteBigEndianTest = 1;
	if( (*((unsigned char*) &litteBigEndianTest)) == 0 ) {         /* big endian */
		const char* vPtr = (char*) v;
		char buffer[sizeof( OUTPUT_PRECISION )];
		for (int i = 0; i < sizeof( OUTPUT_PRECISION ); i++)
			buffer[i] = vPtr[sizeof( OUTPUT_PRECISION ) - i - 1];

		fwrite( buffer, sizeof( OUTPUT_PRECISION ), 1, file );
	}
	else {                                                     /* little endian */
		fwrite( v, sizeof( OUTPUT_PRECISION ), 1, file );
	}
}

/*############################################################################*/

static void loadValue( FILE* file, OUTPUT_PRECISION* v ) {
	const int litteBigEndianTest = 1;
	if( (*((unsigned char*) &litteBigEndianTest)) == 0 ) {         /* big endian */
		char* vPtr = (char*) v;
		char buffer[sizeof( OUTPUT_PRECISION )];

		fread( buffer, sizeof( OUTPUT_PRECISION ), 1, file );

		for (int i = 0; i < sizeof( OUTPUT_PRECISION ); i++)
			vPtr[i] = buffer[sizeof( OUTPUT_PRECISION ) - i - 1];
	}
	else {                                                     /* little endian */
		fread( v, sizeof( OUTPUT_PRECISION ), 1, file );
	}
}

/*############################################################################*/

void LBM_storeVelocityField( Pochoir_Array_3D(PoCellEntry) & pa, const int t, 
                             const char* filename, const int binary ) {
	OUTPUT_PRECISION rho, ux, uy, uz;

	FILE* file = fopen( filename, (binary ? "wb" : "w") );

	for( int z = 0 + MARGIN_Z; z < SIZE_Z + MARGIN_Z; ++z ) {
		for( int y = 0; y < SIZE_Y; ++y ) {
			for( int x = 0; x < SIZE_X; ++x ) {
				rho = + pa.interior(t, z, y, x)._C  + pa.interior(t, z, y, x)._N 
				      + pa.interior(t, z, y, x)._S  + pa.interior(t, z, y, x)._E 
				      + pa.interior(t, z, y, x)._W  + pa.interior(t, z, y, x)._T 
				      + pa.interior(t, z, y, x)._B  + pa.interior(t, z, y, x)._NE
				      + pa.interior(t, z, y, x)._NW + pa.interior(t, z, y, x)._SE
				      + pa.interior(t, z, y, x)._SW + pa.interior(t, z, y, x)._NT
				      + pa.interior(t, z, y, x)._NB + pa.interior(t, z, y, x)._ST
				      + pa.interior(t, z, y, x)._SB + pa.interior(t, z, y, x)._ET
				      + pa.interior(t, z, y, x)._EB + pa.interior(t, z, y, x)._WT
				      + pa.interior(t, z, y, x)._WB;
				ux = + pa.interior(t, z, y, x)._E  - pa.interior(t, z, y, x)._W  
				     + pa.interior(t, z, y, x)._NE - pa.interior(t, z, y, x)._NW 
				     + pa.interior(t, z, y, x)._SE - pa.interior(t, z, y, x)._SW 
				     + pa.interior(t, z, y, x)._ET + pa.interior(t, z, y, x)._EB 
				     - pa.interior(t, z, y, x)._WT - pa.interior(t, z, y, x)._WB;
				uy = + pa.interior(t, z, y, x)._N  - pa.interior(t, z, y, x)._S  
				     + pa.interior(t, z, y, x)._NE + pa.interior(t, z, y, x)._NW 
				     - pa.interior(t, z, y, x)._SE - pa.interior(t, z, y, x)._SW 
				     + pa.interior(t, z, y, x)._NT + pa.interior(t, z, y, x)._NB 
				     - pa.interior(t, z, y, x)._ST - pa.interior(t, z, y, x)._SB;
				uz = + pa.interior(t, z, y, x)._T  - pa.interior(t, z, y, x)._B  
				     + pa.interior(t, z, y, x)._NT - pa.interior(t, z, y, x)._NB 
				     + pa.interior(t, z, y, x)._ST - pa.interior(t, z, y, x)._SB 
				     + pa.interior(t, z, y, x)._ET - pa.interior(t, z, y, x)._EB 
				     + pa.interior(t, z, y, x)._WT - pa.interior(t, z, y, x)._WB;
				ux /= rho;
				uy /= rho;
				uz /= rho;

				if( binary ) {
					/*
					fwrite( &ux, sizeof( ux ), 1, file );
					fwrite( &uy, sizeof( uy ), 1, file );
					fwrite( &uz, sizeof( uz ), 1, file );
					*/
					storeValue( file, &ux );
					storeValue( file, &uy );
					storeValue( file, &uz );
				} else
					fprintf( file, "%e %e %e\n", ux, uy, uz );

			}
		}
	}

	fclose( file );
}

/*############################################################################*/

void LBM_compareVelocityField( Pochoir_Array_3D(PoCellEntry) & pa, const int t,
                               const char* filename, const int binary ) {
	MY_TYPE rho, ux, uy, uz;
	OUTPUT_PRECISION fileUx, fileUy, fileUz,
	                 dUx, dUy, dUz,
	                 diff2, maxDiff2 = -1e+30;

	FILE* file = fopen( filename, (binary ? "rb" : "r") );

	for( int z = 0 + MARGIN_Z; z < SIZE_Z + MARGIN_Z; ++z ) {
		for( int y = 0; y < SIZE_Y; ++y ) {
			for( int x = 0; x < SIZE_X; ++x ) {
				rho = + pa.interior(t, z, y, x)._C  + pa.interior(t, z, y, x)._N 
				      + pa.interior(t, z, y, x)._S  + pa.interior(t, z, y, x)._E 
				      + pa.interior(t, z, y, x)._W  + pa.interior(t, z, y, x)._T 
				      + pa.interior(t, z, y, x)._B  + pa.interior(t, z, y, x)._NE
				      + pa.interior(t, z, y, x)._NW + pa.interior(t, z, y, x)._SE
				      + pa.interior(t, z, y, x)._SW + pa.interior(t, z, y, x)._NT
				      + pa.interior(t, z, y, x)._NB + pa.interior(t, z, y, x)._ST
				      + pa.interior(t, z, y, x)._SB + pa.interior(t, z, y, x)._ET
				      + pa.interior(t, z, y, x)._EB + pa.interior(t, z, y, x)._WT
				      + pa.interior(t, z, y, x)._WB;
				ux = + pa.interior(t, z, y, x)._E  - pa.interior(t, z, y, x)._W  
				     + pa.interior(t, z, y, x)._NE - pa.interior(t, z, y, x)._NW 
				     + pa.interior(t, z, y, x)._SE - pa.interior(t, z, y, x)._SW 
				     + pa.interior(t, z, y, x)._ET + pa.interior(t, z, y, x)._EB 
				     - pa.interior(t, z, y, x)._WT - pa.interior(t, z, y, x)._WB;
				uy = + pa.interior(t, z, y, x)._N  - pa.interior(t, z, y, x)._S  
				     + pa.interior(t, z, y, x)._NE + pa.interior(t, z, y, x)._NW 
				     - pa.interior(t, z, y, x)._SE - pa.interior(t, z, y, x)._SW 
				     + pa.interior(t, z, y, x)._NT + pa.interior(t, z, y, x)._NB 
				     - pa.interior(t, z, y, x)._ST - pa.interior(t, z, y, x)._SB;
				uz = + pa.interior(t, z, y, x)._T  - pa.interior(t, z, y, x)._B  
				     + pa.interior(t, z, y, x)._NT - pa.interior(t, z, y, x)._NB 
				     + pa.interior(t, z, y, x)._ST - pa.interior(t, z, y, x)._SB 
				     + pa.interior(t, z, y, x)._ET - pa.interior(t, z, y, x)._EB 
				     + pa.interior(t, z, y, x)._WT - pa.interior(t, z, y, x)._WB;
				ux /= rho;
				uy /= rho;
				uz /= rho;

				if( binary ) {
					loadValue( file, &fileUx );
					loadValue( file, &fileUy );
					loadValue( file, &fileUz );
				}
				else {
					if( sizeof( OUTPUT_PRECISION ) == sizeof( MY_TYPE )) {
						fscanf( file, "%lf %lf %lf\n", &fileUx, &fileUy, &fileUz );
					}
					else {
						fscanf( file, "%lf %lf %lf\n", &fileUx, &fileUy, &fileUz );
					}
				}

				dUx = ux - fileUx;
				dUy = uy - fileUy;
				dUz = uz - fileUz;
				diff2 = dUx*dUx + dUy*dUy + dUz*dUz;
				if( diff2 > maxDiff2 ) maxDiff2 = diff2;
			}
		}
	}

#if defined(SPEC_CPU)
	printf( "LBM_compareVelocityField: maxDiff = %e  \n\n",
	        sqrt( maxDiff2 )  );
#else
	printf( "LBM_compareVelocityField: maxDiff = %e  ==>  %s\n\n",
	        sqrt( maxDiff2 ),
	        sqrt( maxDiff2 ) > 1e-5 ? "##### ERROR #####" : "OK" );
#endif
	fclose( file );
}

