/*############################################################################*/

#define MY_EXTERN extern
#include "main.h"
#include "lbm_co.h"
#include "lbm_xo.h"
//#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
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

//#define DEBUG_MSG(msg) printf("%s\n", msg); fflush(stdout)
#define DEBUG_MSG(msg)

/*############################################################################*/

#define DFL1 (1.0/ 3.0)
#define DFL2 (1.0/18.0)
#define DFL3 (1.0/36.0)

/*############################################################################*/

double sqrt(const double x)
{
  return x;
}

void LBM_allocateGrid( MY_TYPE** ptr ) {
#ifdef SOA
    const size_t margin_tol = 2*SIZE_X*SIZE_Y*N_CELL_ENTRIES,
                     size   = sizeof( LBM_Grid ) + 2*margin_tol*sizeof( MY_TYPE );
    const size_t margin = 2*SIZE_X*SIZE_Y;
#else
    const size_t margin = 2*SIZE_X*SIZE_Y*N_CELL_ENTRIES,
                 size   = sizeof( LBM_Grid ) + 2*margin*sizeof( MY_TYPE );
#endif

    *ptr = (MY_TYPE*)MY_MALLOC( size );
    if( ! *ptr ) {
        printf( "LBM_allocateGrid: could not allocate %.1f MByte\n",
                size / (1024.0*1024.0) );
        exit( 1 );
    }
#if !defined(SPEC_CPU)
    printf( "LBM_allocateGrid: allocated %.1f MByte\n",
            size / (1024.0*1024.0) );
#endif
    *ptr += margin;
}

/*############################################################################*/

void LBM_freeGrid( MY_TYPE** ptr ) {
#ifdef SOA
  const size_t margin = 2*SIZE_X*SIZE_Y;
#else
  const size_t margin = 2*SIZE_X*SIZE_Y*N_CELL_ENTRIES;
#endif

    MY_FREE( *ptr-margin );
	*ptr = NULL;
}

/*############################################################################*/

void LBM_initializeGrid( LBM_Grid grid ) {
	SWEEP_VAR

	/*voption indep*/
#if !defined(SPEC_CPU)
#endif
	SWEEP_START( 0, 0, -2, 0, 0, SIZE_Z+2 )
		LOCAL( grid, C  ) = DFL1;
		LOCAL( grid, N  ) = DFL2;
		LOCAL( grid, S  ) = DFL2;
		LOCAL( grid, E  ) = DFL2;
		LOCAL( grid, W  ) = DFL2;
		LOCAL( grid, T  ) = DFL2;
		LOCAL( grid, B  ) = DFL2;
		LOCAL( grid, NE ) = DFL3;
		LOCAL( grid, NW ) = DFL3;
		LOCAL( grid, SE ) = DFL3;
		LOCAL( grid, SW ) = DFL3;
		LOCAL( grid, NT ) = DFL3;
		LOCAL( grid, NB ) = DFL3;
		LOCAL( grid, ST ) = DFL3;
		LOCAL( grid, SB ) = DFL3;
		LOCAL( grid, ET ) = DFL3;
		LOCAL( grid, EB ) = DFL3;
		LOCAL( grid, WT ) = DFL3;
		LOCAL( grid, WB ) = DFL3;

		CLEAR_ALL_FLAGS_SWEEP( grid );
	SWEEP_END
}

/*############################################################################*/

void LBM_swapGrids( LBM_GridPtr* grid1, LBM_GridPtr* grid2 ) {
	LBM_GridPtr aux = *grid1;
	*grid1 = *grid2;
	*grid2 = aux;
}

/*############################################################################*/

void LBM_loadObstacleFile( LBM_Grid grid, const char* filename ) {
	int x,  y,  z;

	FILE* file = fopen( filename, "rb" );

	for( z = 0; z < SIZE_Z; z++ ) {
		for( y = 0; y < SIZE_Y; y++ ) {
			for( x = 0; x < SIZE_X; x++ ) {
				if( fgetc( file ) != '.' ) SET_FLAG( grid, x, y, z, OBSTACLE );
			}
			fgetc( file );
		}
		fgetc( file );
	}

	fclose( file );
}

/*############################################################################*/

void LBM_initializeSpecialCellsForLDC( LBM_Grid grid ) {
	int x,  y,  z;

	/*voption indep*/
#if !defined(SPEC_CPU)
#endif
	for( z = -2; z < SIZE_Z+2; z++ ) {
		for( y = 0; y < SIZE_Y; y++ ) {
			for( x = 0; x < SIZE_X; x++ ) {
				if( x == 0 || x == SIZE_X-1 ||
				    y == 0 || y == SIZE_Y-1 ||
				    z == 0 || z == SIZE_Z-1 ) {
					SET_FLAG( grid, x, y, z, OBSTACLE );
				}
				else {
					if( (z == 1 || z == SIZE_Z-2) &&
					     x > 1 && x < SIZE_X-2 &&
					     y > 1 && y < SIZE_Y-2 ) {
						SET_FLAG( grid, x, y, z, ACCEL );
					}
				}
			}
		}
	}
}

/*############################################################################*/

void LBM_initializeSpecialCellsForChannel( LBM_Grid grid ) {
	int x,  y,  z;

	/*voption indep*/
#if !defined(SPEC_CPU)
#endif
	for( z = -2; z < SIZE_Z+2; z++ ) {
		for( y = 0; y < SIZE_Y; y++ ) {
			for( x = 0; x < SIZE_X; x++ ) {
				if( x == 0 || x == SIZE_X-1 ||
				    y == 0 || y == SIZE_Y-1 ) {
					SET_FLAG( grid, x, y, z, OBSTACLE );

					if( (z == 0 || z == SIZE_Z-1) &&
					    ! TEST_FLAG( grid, x, y, z, OBSTACLE ))
						SET_FLAG( grid, x, y, z, IN_OUT_FLOW );
				}
			}
		}
	}
}

/*############################################################################*/
void LBM_performStreamCollide( LBM_Grid srcGrid, LBM_Grid dstGrid,
                               const int x0, const int x1, const int y0, const int y1, const int z0, const int z1)
{
#ifdef ALIGNED_MALLOC    
    __assume_aligned(srcGrid, MY_ALIGNMENT);
    __assume_aligned(dstGrid, MY_ALIGNMENT);
#endif
    
    MY_TYPE ux, uy, uz, u2, rho;

    // CKLUK: Over 95% of total Runtime spent in this loop
    int i, x, y, z;

    for (z = z0; z < z1; z++)
    {
        for (y = y0; y < y1; y++)
        {
#ifdef VECTORIZE
#pragma simd
#endif            
            for (x = x0; x < x1; x++)
            {
#ifdef MYOPT                
                i = CALC_INDEX(x, y, z, 0);

                const MY_TYPE src_c =  SRC_C  ( srcGrid );
                const MY_TYPE src_n =  SRC_N  ( srcGrid );
                const MY_TYPE src_s =  SRC_S  ( srcGrid );
                const MY_TYPE src_e =  SRC_E  ( srcGrid );
                const MY_TYPE src_w =  SRC_W  ( srcGrid );
                const MY_TYPE src_t =  SRC_T  ( srcGrid );
                const MY_TYPE src_b =  SRC_B  ( srcGrid );
                const MY_TYPE src_ne = SRC_NE ( srcGrid );
                const MY_TYPE src_nw = SRC_NW ( srcGrid );
                const MY_TYPE src_se = SRC_SE ( srcGrid );
                const MY_TYPE src_sw = SRC_SW ( srcGrid );
                const MY_TYPE src_nt = SRC_NT ( srcGrid );
                const MY_TYPE src_nb = SRC_NB ( srcGrid );
                const MY_TYPE src_st = SRC_ST ( srcGrid );
                const MY_TYPE src_sb = SRC_SB ( srcGrid );
                const MY_TYPE src_et = SRC_ET ( srcGrid );
                const MY_TYPE src_eb = SRC_EB ( srcGrid );
                const MY_TYPE src_wt = SRC_WT ( srcGrid );
                const MY_TYPE src_wb = SRC_WB ( srcGrid );

                MY_TYPE* dst_c  = &DST_C  (dstGrid);
                MY_TYPE* dst_n  = &DST_N  (dstGrid);
                MY_TYPE* dst_s  = &DST_S  (dstGrid);
                MY_TYPE* dst_e  = &DST_E  (dstGrid);
                MY_TYPE* dst_w  = &DST_W  (dstGrid);
                MY_TYPE* dst_t  = &DST_T  (dstGrid);
                MY_TYPE* dst_b  = &DST_B  (dstGrid);
                MY_TYPE* dst_ne = &DST_NE  (dstGrid);
                MY_TYPE* dst_nw = &DST_NW  (dstGrid);
                MY_TYPE* dst_se = &DST_SE  (dstGrid);
                MY_TYPE* dst_sw = &DST_SW  (dstGrid);
                MY_TYPE* dst_nt = &DST_NT  (dstGrid);
                MY_TYPE* dst_nb = &DST_NB  (dstGrid);
                MY_TYPE* dst_st = &DST_ST  (dstGrid);
                MY_TYPE* dst_sb = &DST_SB  (dstGrid);
                MY_TYPE* dst_et = &DST_ET  (dstGrid);
                MY_TYPE* dst_eb = &DST_EB  (dstGrid);
                MY_TYPE* dst_wt = &DST_WT  (dstGrid);
                MY_TYPE* dst_wb = &DST_WB  (dstGrid);

                if( TEST_FLAG_SWEEP( srcGrid, OBSTACLE )) {
                    *dst_c  = src_c;
                    *dst_s  = src_n;
                    *dst_n  = src_s;
                    *dst_w  = src_e;
                    *dst_e  = src_w;
                    *dst_b  = src_t;
                    *dst_t  = src_b;
                    *dst_sw = src_ne;
                    *dst_se = src_nw;
                    *dst_nw = src_se;
                    *dst_ne = src_sw;
                    *dst_sb = src_nt;
                    *dst_st = src_nb;
                    *dst_nb = src_st;
                    *dst_nt = src_sb;
                    *dst_wb = src_et;
                    *dst_wt = src_eb;
                    *dst_eb = src_wt;
                    *dst_et = src_wb;
                    continue;
                }

                rho = + src_c  + src_n 
                      + src_s  + src_e 
                      + src_w  + src_t 
                      + src_b  + src_ne
                      + src_nw + src_se
                      + src_sw + src_nt
                      + src_nb + src_st
                      + src_sb + src_et
                      + src_eb + src_wt
                      + src_wb;

                ux = + src_e  - src_w 
                     + src_ne - src_nw
                     + src_se - src_sw
                     + src_et + src_eb
                     - src_wt - src_wb;
                uy = + src_n  - src_s 
                     + src_ne + src_nw
                     - src_se - src_sw
                     + src_nt + src_nb
                     - src_st - src_sb;
                uz = + src_t  - src_b 
                     + src_nt - src_nb
                     + src_st - src_sb
                     + src_et - src_eb
                     + src_wt - src_wb;

                ux /= rho;
                uy /= rho;
                uz /= rho;

                if( TEST_FLAG_SWEEP( srcGrid, ACCEL )) {
                    ux = 0.005;
                    uy = 0.002;
                    uz = 0.000;
                }
        
                u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

	        const MY_TYPE k0 = (1.0-OMEGA);
		const MY_TYPE k1 = (DFL1*OMEGA*rho);
		const MY_TYPE k2 = (DFL2*OMEGA*rho);
		const MY_TYPE k3 = (DFL3*OMEGA*rho);
		
                *dst_c  = k0*src_c  + k1*(1.0                                 - u2);
                
                *dst_n  = k0*src_n  + k2*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
                *dst_s  = k0*src_s  + k2*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
                *dst_e  = k0*src_e  + k2*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
                *dst_w  = k0*src_w  + k2*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
                *dst_t  = k0*src_t  + k2*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
                *dst_b  = k0*src_b  + k2*(1.0 +       uz*(4.5*uz       - 3.0) - u2);
        
                *dst_ne = k0*src_ne + k3*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
                *dst_nw = k0*src_nw + k3*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
                *dst_se = k0*src_se + k3*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
                *dst_sw = k0*src_sw + k3*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
                *dst_nt = k0*src_nt + k3*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
                *dst_nb = k0*src_nb + k3*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
                *dst_st = k0*src_st + k3*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
                *dst_sb = k0*src_sb + k3*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
                *dst_et = k0*src_et + k3*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
                *dst_eb = k0*src_eb + k3*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
                *dst_wt = k0*src_wt + k3*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
                *dst_wb = k0*src_wb + k3*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);

#else                
                i = CALC_INDEX(x, y, z, 0);
                
                if( TEST_FLAG_SWEEP( srcGrid, OBSTACLE )) {
                    DST_C ( dstGrid ) = SRC_C ( srcGrid );
                    DST_S ( dstGrid ) = SRC_N ( srcGrid );
                    DST_N ( dstGrid ) = SRC_S ( srcGrid );
                    DST_W ( dstGrid ) = SRC_E ( srcGrid );
                    DST_E ( dstGrid ) = SRC_W ( srcGrid );
                    DST_B ( dstGrid ) = SRC_T ( srcGrid );
                    DST_T ( dstGrid ) = SRC_B ( srcGrid );
                    DST_SW( dstGrid ) = SRC_NE( srcGrid );
                    DST_SE( dstGrid ) = SRC_NW( srcGrid );
                    DST_NW( dstGrid ) = SRC_SE( srcGrid );
                    DST_NE( dstGrid ) = SRC_SW( srcGrid );
                    DST_SB( dstGrid ) = SRC_NT( srcGrid );
                    DST_ST( dstGrid ) = SRC_NB( srcGrid );
                    DST_NB( dstGrid ) = SRC_ST( srcGrid );
                    DST_NT( dstGrid ) = SRC_SB( srcGrid );
                    DST_WB( dstGrid ) = SRC_ET( srcGrid );
                    DST_WT( dstGrid ) = SRC_EB( srcGrid );
                    DST_EB( dstGrid ) = SRC_WT( srcGrid );
                    DST_ET( dstGrid ) = SRC_WB( srcGrid );
                    continue;
                }
                            
                rho = + SRC_C ( srcGrid ) + SRC_N ( srcGrid )
                      + SRC_S ( srcGrid ) + SRC_E ( srcGrid )
                      + SRC_W ( srcGrid ) + SRC_T ( srcGrid )
                      + SRC_B ( srcGrid ) + SRC_NE( srcGrid )
                      + SRC_NW( srcGrid ) + SRC_SE( srcGrid )
                      + SRC_SW( srcGrid ) + SRC_NT( srcGrid )
                      + SRC_NB( srcGrid ) + SRC_ST( srcGrid )
                      + SRC_SB( srcGrid ) + SRC_ET( srcGrid )
                      + SRC_EB( srcGrid ) + SRC_WT( srcGrid )
                      + SRC_WB( srcGrid );

                ux = + SRC_E ( srcGrid ) - SRC_W ( srcGrid )
                     + SRC_NE( srcGrid ) - SRC_NW( srcGrid )
                     + SRC_SE( srcGrid ) - SRC_SW( srcGrid )
                     + SRC_ET( srcGrid ) + SRC_EB( srcGrid )
                     - SRC_WT( srcGrid ) - SRC_WB( srcGrid );
                uy = + SRC_N ( srcGrid ) - SRC_S ( srcGrid )
                     + SRC_NE( srcGrid ) + SRC_NW( srcGrid )
                     - SRC_SE( srcGrid ) - SRC_SW( srcGrid )
                     + SRC_NT( srcGrid ) + SRC_NB( srcGrid )
                     - SRC_ST( srcGrid ) - SRC_SB( srcGrid );
                uz = + SRC_T ( srcGrid ) - SRC_B ( srcGrid )
                     + SRC_NT( srcGrid ) - SRC_NB( srcGrid )
                     + SRC_ST( srcGrid ) - SRC_SB( srcGrid )
                     + SRC_ET( srcGrid ) - SRC_EB( srcGrid )
                     + SRC_WT( srcGrid ) - SRC_WB( srcGrid );

                ux /= rho;
                uy /= rho;
                uz /= rho;

                if( TEST_FLAG_SWEEP( srcGrid, ACCEL )) {
                    ux = 0.005;
                    uy = 0.002;
                    uz = 0.000;
                }
        
                u2 = 1.5 * (ux*ux + uy*uy + uz*uz);
                DST_C ( dstGrid ) = (1.0-OMEGA)*SRC_C ( srcGrid ) + DFL1*OMEGA*rho*(1.0                                 - u2);
                
                DST_N ( dstGrid ) = (1.0-OMEGA)*SRC_N ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
                DST_S ( dstGrid ) = (1.0-OMEGA)*SRC_S ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
                DST_E ( dstGrid ) = (1.0-OMEGA)*SRC_E ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
                DST_W ( dstGrid ) = (1.0-OMEGA)*SRC_W ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
                DST_T ( dstGrid ) = (1.0-OMEGA)*SRC_T ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
                DST_B ( dstGrid ) = (1.0-OMEGA)*SRC_B ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);
        
                DST_NE( dstGrid ) = (1.0-OMEGA)*SRC_NE( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
                DST_NW( dstGrid ) = (1.0-OMEGA)*SRC_NW( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
                DST_SE( dstGrid ) = (1.0-OMEGA)*SRC_SE( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
                DST_SW( dstGrid ) = (1.0-OMEGA)*SRC_SW( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
                DST_NT( dstGrid ) = (1.0-OMEGA)*SRC_NT( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
                DST_NB( dstGrid ) = (1.0-OMEGA)*SRC_NB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
                DST_ST( dstGrid ) = (1.0-OMEGA)*SRC_ST( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
                DST_SB( dstGrid ) = (1.0-OMEGA)*SRC_SB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
                DST_ET( dstGrid ) = (1.0-OMEGA)*SRC_ET( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
                DST_EB( dstGrid ) = (1.0-OMEGA)*SRC_EB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
                DST_WT( dstGrid ) = (1.0-OMEGA)*SRC_WT( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
                DST_WB( dstGrid ) = (1.0-OMEGA)*SRC_WB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
#endif /*MYOPT*/
            }
        }
    }
}


/*############################################################################*/

void LBM_handleInOutFlow( LBM_Grid srcGrid,
                          const int x0, const int x1, const int y0, const int y1, const int z0, const int z1) {
    
    MY_TYPE ux , uy , uz , rho ,
        ux1, uy1, uz1, rho1,
        ux2, uy2, uz2, rho2,
        u2, px, py;
    
    int i, x, y, z;

    for (z= z0; z < z1; z++)
    {
        if (z == 0)
        { /* inflow */        
            for (y = y0; y < y1; y++)
            {
                for (x = x0; x < x1; x++)
                {
                    i = CALC_INDEX(x, y, z, 0);

                    rho1 = + srcGrid[CALC_INDEX(x, y, z+1, C )] + srcGrid[CALC_INDEX(x, y, z+1, N)]
                           + srcGrid[CALC_INDEX(x, y, z+1, S )] + srcGrid[CALC_INDEX(x, y, z+1, E)]
                           + srcGrid[CALC_INDEX(x, y, z+1, W )] + srcGrid[CALC_INDEX(x, y, z+1, T)]
                           + srcGrid[CALC_INDEX(x, y, z+1, B )] + srcGrid[CALC_INDEX(x, y, z+1, NE)]
                           + srcGrid[CALC_INDEX(x, y, z+1, NW)] + srcGrid[CALC_INDEX(x, y, z+1, SE)]
                           + srcGrid[CALC_INDEX(x, y, z+1, SW)] + srcGrid[CALC_INDEX(x, y, z+1, NT)]
                           + srcGrid[CALC_INDEX(x, y, z+1, NB)] + srcGrid[CALC_INDEX(x, y, z+1, ST)]
                           + srcGrid[CALC_INDEX(x, y, z+1, SB)] + srcGrid[CALC_INDEX(x, y, z+1, ET)]
                           + srcGrid[CALC_INDEX(x, y, z+1, EB)] + srcGrid[CALC_INDEX(x, y, z+1, WT)]
                           + srcGrid[CALC_INDEX(x, y, z+1, WB)];

                    rho2 = + srcGrid[CALC_INDEX(x, y, z+2, C )] + srcGrid[CALC_INDEX(x, y, z+2, N)]
                           + srcGrid[CALC_INDEX(x, y, z+2, S )] + srcGrid[CALC_INDEX(x, y, z+2, E)]
                           + srcGrid[CALC_INDEX(x, y, z+2, W )] + srcGrid[CALC_INDEX(x, y, z+2, T)]
                           + srcGrid[CALC_INDEX(x, y, z+2, B )] + srcGrid[CALC_INDEX(x, y, z+2, NE)]
                           + srcGrid[CALC_INDEX(x, y, z+2, NW)] + srcGrid[CALC_INDEX(x, y, z+2, SE)]
                           + srcGrid[CALC_INDEX(x, y, z+2, SW)] + srcGrid[CALC_INDEX(x, y, z+2, NT)]
                           + srcGrid[CALC_INDEX(x, y, z+2, NB)] + srcGrid[CALC_INDEX(x, y, z+2, ST)]
                           + srcGrid[CALC_INDEX(x, y, z+2, SB)] + srcGrid[CALC_INDEX(x, y, z+2, ET)]
                           + srcGrid[CALC_INDEX(x, y, z+2, EB)] + srcGrid[CALC_INDEX(x, y, z+2, WT)]
                           + srcGrid[CALC_INDEX(x, y, z+2, WB)];


                    rho = 2.0*rho1 - rho2;

                    px = (x / (0.5*(SIZE_X-1))) - 1.0;
                    py = (y / (0.5*(SIZE_Y-1))) - 1.0;
                    ux = 0.00;
                    uy = 0.00;
                    uz = 0.01 * (1.0-px*px) * (1.0-py*py);

                    u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

                    LOCAL( srcGrid, C ) = DFL1*rho*(1.0                                 - u2);

                    LOCAL( srcGrid, N ) = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
                    LOCAL( srcGrid, S ) = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
                    LOCAL( srcGrid, E ) = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
                    LOCAL( srcGrid, W ) = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
                    LOCAL( srcGrid, T ) = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
                    LOCAL( srcGrid, B ) = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

                    LOCAL( srcGrid, NE) = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
                    LOCAL( srcGrid, NW) = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
                    LOCAL( srcGrid, SE) = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
                    LOCAL( srcGrid, SW) = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
                    LOCAL( srcGrid, NT) = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
                    LOCAL( srcGrid, NB) = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
                    LOCAL( srcGrid, ST) = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
                    LOCAL( srcGrid, SB) = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
                    LOCAL( srcGrid, ET) = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
                    LOCAL( srcGrid, EB) = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
                    LOCAL( srcGrid, WT) = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
                    LOCAL( srcGrid, WB) = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
                } /* for x */                
            } /* for y */
        } /* if (z==0) */

        if (z == (SIZE_Z-1))
        {     /* outflow */
            for (y = y0; y < y1; y++)
            {
                for (x = x0; x < x1; x++)
                {
                    i = CALC_INDEX(x, y, z, 0);
                
                    rho1 = + srcGrid[CALC_INDEX(x, y, z-1, C )] + srcGrid[CALC_INDEX(x, y, z-1, N)]
                           + srcGrid[CALC_INDEX(x, y, z-1, S )] + srcGrid[CALC_INDEX(x, y, z-1, E)]
                           + srcGrid[CALC_INDEX(x, y, z-1, W )] + srcGrid[CALC_INDEX(x, y, z-1, T)]
                           + srcGrid[CALC_INDEX(x, y, z-1, B )] + srcGrid[CALC_INDEX(x, y, z-1, NE)]
                           + srcGrid[CALC_INDEX(x, y, z-1, NW)] + srcGrid[CALC_INDEX(x, y, z-1, SE)]
                           + srcGrid[CALC_INDEX(x, y, z-1, SW)] + srcGrid[CALC_INDEX(x, y, z-1, NT)]
                           + srcGrid[CALC_INDEX(x, y, z-1, NB)] + srcGrid[CALC_INDEX(x, y, z-1, ST)]
                           + srcGrid[CALC_INDEX(x, y, z-1, SB)] + srcGrid[CALC_INDEX(x, y, z-1, ET)]
                           + srcGrid[CALC_INDEX(x, y, z-1, EB)] + srcGrid[CALC_INDEX(x, y, z-1, WT)]
                           + srcGrid[CALC_INDEX(x, y, z-1, WB)];
                
                    ux1 = + srcGrid[CALC_INDEX(x, y, z-1, E )] - srcGrid[CALC_INDEX(x, y, z-1, W )]
                          + srcGrid[CALC_INDEX(x, y, z-1, NE)] - srcGrid[CALC_INDEX(x, y, z-1, NW)]
                          + srcGrid[CALC_INDEX(x, y, z-1, SE)] - srcGrid[CALC_INDEX(x, y, z-1, SW)]
                          + srcGrid[CALC_INDEX(x, y, z-1, ET)] + srcGrid[CALC_INDEX(x, y, z-1, EB)]
                          - srcGrid[CALC_INDEX(x, y, z-1, WT)] - srcGrid[CALC_INDEX(x, y, z-1, WB)];
                    uy1 = + srcGrid[CALC_INDEX(x, y, z-1, N )] - srcGrid[CALC_INDEX(x, y, z-1, S )]
                          + srcGrid[CALC_INDEX(x, y, z-1, NE)] + srcGrid[CALC_INDEX(x, y, z-1, NW)]
                          - srcGrid[CALC_INDEX(x, y, z-1, SE)] - srcGrid[CALC_INDEX(x, y, z-1, SW)]
                          + srcGrid[CALC_INDEX(x, y, z-1, NT)] + srcGrid[CALC_INDEX(x, y, z-1, NB)]
                          - srcGrid[CALC_INDEX(x, y, z-1, ST)] - srcGrid[CALC_INDEX(x, y, z-1, SB)];
                    uz1 = + srcGrid[CALC_INDEX(x, y, z-1, T )] - srcGrid[CALC_INDEX(x, y, z-1, B )]
                          + srcGrid[CALC_INDEX(x, y, z-1, NT)] - srcGrid[CALC_INDEX(x, y, z-1, NB)]
                          + srcGrid[CALC_INDEX(x, y, z-1, ST)] - srcGrid[CALC_INDEX(x, y, z-1, SB)]
                          + srcGrid[CALC_INDEX(x, y, z-1, ET)] - srcGrid[CALC_INDEX(x, y, z-1, EB)]
                          + srcGrid[CALC_INDEX(x, y, z-1, WT)] - srcGrid[CALC_INDEX(x, y, z-1, WB)];

                    ux1 /= rho1;
                    uy1 /= rho1;
                    uz1 /= rho1;

                    rho2 = + srcGrid[CALC_INDEX(x, y, z-2, C )] + srcGrid[CALC_INDEX(x, y, z-2, N)]
                           + srcGrid[CALC_INDEX(x, y, z-2, S )] + srcGrid[CALC_INDEX(x, y, z-2, E)]
                           + srcGrid[CALC_INDEX(x, y, z-2, W )] + srcGrid[CALC_INDEX(x, y, z-2, T)]
                           + srcGrid[CALC_INDEX(x, y, z-2, B )] + srcGrid[CALC_INDEX(x, y, z-2, NE)]
                           + srcGrid[CALC_INDEX(x, y, z-2, NW)] + srcGrid[CALC_INDEX(x, y, z-2, SE)]
                           + srcGrid[CALC_INDEX(x, y, z-2, SW)] + srcGrid[CALC_INDEX(x, y, z-2, NT)]
                           + srcGrid[CALC_INDEX(x, y, z-2, NB)] + srcGrid[CALC_INDEX(x, y, z-2, ST)]
                           + srcGrid[CALC_INDEX(x, y, z-2, SB)] + srcGrid[CALC_INDEX(x, y, z-2, ET)]
                           + srcGrid[CALC_INDEX(x, y, z-2, EB)] + srcGrid[CALC_INDEX(x, y, z-2, WT)]
                           + srcGrid[CALC_INDEX(x, y, z-2, WB)];
                
                    ux2 = + srcGrid[CALC_INDEX(x, y, z-2, E )] - srcGrid[CALC_INDEX(x, y, z-2, W )]
                          + srcGrid[CALC_INDEX(x, y, z-2, NE)] - srcGrid[CALC_INDEX(x, y, z-2, NW)]
                          + srcGrid[CALC_INDEX(x, y, z-2, SE)] - srcGrid[CALC_INDEX(x, y, z-2, SW)]
                          + srcGrid[CALC_INDEX(x, y, z-2, ET)] + srcGrid[CALC_INDEX(x, y, z-2, EB)]
                          - srcGrid[CALC_INDEX(x, y, z-2, WT)] - srcGrid[CALC_INDEX(x, y, z-2, WB)];
                    uy2 = + srcGrid[CALC_INDEX(x, y, z-2, N )] - srcGrid[CALC_INDEX(x, y, z-2, S )]
                          + srcGrid[CALC_INDEX(x, y, z-2, NE)] + srcGrid[CALC_INDEX(x, y, z-2, NW)]
                          - srcGrid[CALC_INDEX(x, y, z-2, SE)] - srcGrid[CALC_INDEX(x, y, z-2, SW)]
                          + srcGrid[CALC_INDEX(x, y, z-2, NT)] + srcGrid[CALC_INDEX(x, y, z-2, NB)]
                          - srcGrid[CALC_INDEX(x, y, z-2, ST)] - srcGrid[CALC_INDEX(x, y, z-2, SB)];
                    uz2 = + srcGrid[CALC_INDEX(x, y, z-2, T )] - srcGrid[CALC_INDEX(x, y, z-2, B )]
                          + srcGrid[CALC_INDEX(x, y, z-2, NT)] - srcGrid[CALC_INDEX(x, y, z-2, NB)]
                          + srcGrid[CALC_INDEX(x, y, z-2, ST)] - srcGrid[CALC_INDEX(x, y, z-2, SB)]
                          + srcGrid[CALC_INDEX(x, y, z-2, ET)] - srcGrid[CALC_INDEX(x, y, z-2, EB)]
                          + srcGrid[CALC_INDEX(x, y, z-2, WT)] - srcGrid[CALC_INDEX(x, y, z-2, WB)];


                    ux2 /= rho2;
                    uy2 /= rho2;
                    uz2 /= rho2;

                    rho = 1.0;

                    ux = 2*ux1 - ux2;
                    uy = 2*uy1 - uy2;
                    uz = 2*uz1 - uz2;

                    u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

                    LOCAL( srcGrid, C ) = DFL1*rho*(1.0                                 - u2);

                    LOCAL( srcGrid, N ) = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
                    LOCAL( srcGrid, S ) = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
                    LOCAL( srcGrid, E ) = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
                    LOCAL( srcGrid, W ) = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
                    LOCAL( srcGrid, T ) = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
                    LOCAL( srcGrid, B ) = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

                    LOCAL( srcGrid, NE) = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
                    LOCAL( srcGrid, NW) = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
                    LOCAL( srcGrid, SE) = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
                    LOCAL( srcGrid, SW) = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
                    LOCAL( srcGrid, NT) = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
                    LOCAL( srcGrid, NB) = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
                    LOCAL( srcGrid, ST) = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
                    LOCAL( srcGrid, SB) = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
                    LOCAL( srcGrid, ET) = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
                    LOCAL( srcGrid, EB) = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
                    LOCAL( srcGrid, WT) = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
                    LOCAL( srcGrid, WB) = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
                }
            }            
        } /* if (z==SIZE_Z-1) */
    } /* for z */
}

/*############################################################################*/

void LBM_showGridStatistics( LBM_Grid grid ) {
	int nObstacleCells = 0,
	    nAccelCells    = 0,
	    nFluidCells    = 0;
	MY_TYPE ux, uy, uz;
	MY_TYPE minU2  = 1e+30, maxU2  = -1e+30, u2;
	MY_TYPE minRho = 1e+30, maxRho = -1e+30, rho;
	MY_TYPE mass = 0;

	SWEEP_VAR

	SWEEP_START( 0, 0, 0, 0, 0, SIZE_Z )
#ifdef SOA
    rho = LOCAL( grid, C  ) + LOCAL( grid, N  )
#else
    rho = + LOCAL( grid, C  ) + LOCAL( grid, N  )
#endif
		      + LOCAL( grid, S  ) + LOCAL( grid, E  )
		      + LOCAL( grid, W  ) + LOCAL( grid, T  )
		      + LOCAL( grid, B  ) + LOCAL( grid, NE )
		      + LOCAL( grid, NW ) + LOCAL( grid, SE )
		      + LOCAL( grid, SW ) + LOCAL( grid, NT )
		      + LOCAL( grid, NB ) + LOCAL( grid, ST )
		      + LOCAL( grid, SB ) + LOCAL( grid, ET )
		      + LOCAL( grid, EB ) + LOCAL( grid, WT )
		      + LOCAL( grid, WB );
		if( rho < minRho ) minRho = rho;
		if( rho > maxRho ) maxRho = rho;
		mass += rho;

		if( TEST_FLAG_SWEEP( grid, OBSTACLE )) {
			nObstacleCells++;
		}
		else {
			if( TEST_FLAG_SWEEP( grid, ACCEL ))
				nAccelCells++;
			else
				nFluidCells++;

			ux = + LOCAL( grid, E  ) - LOCAL( grid, W  )
			     + LOCAL( grid, NE ) - LOCAL( grid, NW )
			     + LOCAL( grid, SE ) - LOCAL( grid, SW )
			     + LOCAL( grid, ET ) + LOCAL( grid, EB )
			     - LOCAL( grid, WT ) - LOCAL( grid, WB );
			uy = + LOCAL( grid, N  ) - LOCAL( grid, S  )
			     + LOCAL( grid, NE ) + LOCAL( grid, NW )
			     - LOCAL( grid, SE ) - LOCAL( grid, SW )
			     + LOCAL( grid, NT ) + LOCAL( grid, NB )
			     - LOCAL( grid, ST ) - LOCAL( grid, SB );
			uz = + LOCAL( grid, T  ) - LOCAL( grid, B  )
			     + LOCAL( grid, NT ) - LOCAL( grid, NB )
			     + LOCAL( grid, ST ) - LOCAL( grid, SB )
			     + LOCAL( grid, ET ) - LOCAL( grid, EB )
			     + LOCAL( grid, WT ) - LOCAL( grid, WB );
			u2 = (ux*ux + uy*uy + uz*uz) / (rho*rho);
			if( u2 < minU2 ) minU2 = u2;
			if( u2 > maxU2 ) maxU2 = u2;
		}
	SWEEP_END

        printf( "LBM_showGridStatistics:\n"
        "\tnObstacleCells: %7i nAccelCells: %7i nFluidCells: %7i\n"
        "\tminRho: %8.4f maxRho: %8.4f mass: %e\n"
        "\tminU: %e maxU: %e\n\n",
        nObstacleCells, nAccelCells, nFluidCells,
        minRho, maxRho, mass,
        sqrt( minU2 ), sqrt( maxU2 ) );

}

/*############################################################################*/

static void storeValue( FILE* file, OUTPUT_PRECISION* v ) {
	const int litteBigEndianTest = 1;
	if( (*((unsigned char*) &litteBigEndianTest)) == 0 ) {         /* big endian */
		const char* vPtr = (char*) v;
		char buffer[sizeof( OUTPUT_PRECISION )];
		int i;

		for (i = 0; i < sizeof( OUTPUT_PRECISION ); i++)
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
		int i;

		fread( buffer, sizeof( OUTPUT_PRECISION ), 1, file );

		for (i = 0; i < sizeof( OUTPUT_PRECISION ); i++)
			vPtr[i] = buffer[sizeof( OUTPUT_PRECISION ) - i - 1];
	}
	else {                                                     /* little endian */
		fread( v, sizeof( OUTPUT_PRECISION ), 1, file );
	}
}

/*############################################################################*/

void LBM_storeVelocityField( LBM_Grid grid, const char* filename,
                             const int binary ) {
	int x, y, z;
	OUTPUT_PRECISION rho, ux, uy, uz;

	FILE* file = fopen( filename, (binary ? "wb" : "w") );

	for( z = 0; z < SIZE_Z; z++ ) {
		for( y = 0; y < SIZE_Y; y++ ) {
			for( x = 0; x < SIZE_X; x++ ) {
				rho = + GRID_ENTRY( grid, x, y, z, C  ) + GRID_ENTRY( grid, x, y, z, N  )
				      + GRID_ENTRY( grid, x, y, z, S  ) + GRID_ENTRY( grid, x, y, z, E  )
				      + GRID_ENTRY( grid, x, y, z, W  ) + GRID_ENTRY( grid, x, y, z, T  )
				      + GRID_ENTRY( grid, x, y, z, B  ) + GRID_ENTRY( grid, x, y, z, NE )
				      + GRID_ENTRY( grid, x, y, z, NW ) + GRID_ENTRY( grid, x, y, z, SE )
				      + GRID_ENTRY( grid, x, y, z, SW ) + GRID_ENTRY( grid, x, y, z, NT )
				      + GRID_ENTRY( grid, x, y, z, NB ) + GRID_ENTRY( grid, x, y, z, ST )
				      + GRID_ENTRY( grid, x, y, z, SB ) + GRID_ENTRY( grid, x, y, z, ET )
				      + GRID_ENTRY( grid, x, y, z, EB ) + GRID_ENTRY( grid, x, y, z, WT )
				      + GRID_ENTRY( grid, x, y, z, WB );
				ux = + GRID_ENTRY( grid, x, y, z, E  ) - GRID_ENTRY( grid, x, y, z, W  ) 
				     + GRID_ENTRY( grid, x, y, z, NE ) - GRID_ENTRY( grid, x, y, z, NW ) 
				     + GRID_ENTRY( grid, x, y, z, SE ) - GRID_ENTRY( grid, x, y, z, SW ) 
				     + GRID_ENTRY( grid, x, y, z, ET ) + GRID_ENTRY( grid, x, y, z, EB ) 
				     - GRID_ENTRY( grid, x, y, z, WT ) - GRID_ENTRY( grid, x, y, z, WB );
				uy = + GRID_ENTRY( grid, x, y, z, N  ) - GRID_ENTRY( grid, x, y, z, S  ) 
				     + GRID_ENTRY( grid, x, y, z, NE ) + GRID_ENTRY( grid, x, y, z, NW ) 
				     - GRID_ENTRY( grid, x, y, z, SE ) - GRID_ENTRY( grid, x, y, z, SW ) 
				     + GRID_ENTRY( grid, x, y, z, NT ) + GRID_ENTRY( grid, x, y, z, NB ) 
				     - GRID_ENTRY( grid, x, y, z, ST ) - GRID_ENTRY( grid, x, y, z, SB );
				uz = + GRID_ENTRY( grid, x, y, z, T  ) - GRID_ENTRY( grid, x, y, z, B  ) 
				     + GRID_ENTRY( grid, x, y, z, NT ) - GRID_ENTRY( grid, x, y, z, NB ) 
				     + GRID_ENTRY( grid, x, y, z, ST ) - GRID_ENTRY( grid, x, y, z, SB ) 
				     + GRID_ENTRY( grid, x, y, z, ET ) - GRID_ENTRY( grid, x, y, z, EB ) 
				     + GRID_ENTRY( grid, x, y, z, WT ) - GRID_ENTRY( grid, x, y, z, WB );
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

void LBM_compareVelocityField( LBM_Grid grid, const char* filename,
                             const int binary ) {
	int x, y, z;
	MY_TYPE rho, ux, uy, uz;
	OUTPUT_PRECISION fileUx, fileUy, fileUz,
	                 dUx, dUy, dUz,
	                 diff2, maxDiff2 = -1e+30;

	FILE* file = fopen( filename, (binary ? "rb" : "r") );

	for( z = 0; z < SIZE_Z; z++ ) {
		for( y = 0; y < SIZE_Y; y++ ) {
			for( x = 0; x < SIZE_X; x++ ) {
				rho = + GRID_ENTRY( grid, x, y, z, C  ) + GRID_ENTRY( grid, x, y, z, N  )
				      + GRID_ENTRY( grid, x, y, z, S  ) + GRID_ENTRY( grid, x, y, z, E  )
				      + GRID_ENTRY( grid, x, y, z, W  ) + GRID_ENTRY( grid, x, y, z, T  )
				      + GRID_ENTRY( grid, x, y, z, B  ) + GRID_ENTRY( grid, x, y, z, NE )
				      + GRID_ENTRY( grid, x, y, z, NW ) + GRID_ENTRY( grid, x, y, z, SE )
				      + GRID_ENTRY( grid, x, y, z, SW ) + GRID_ENTRY( grid, x, y, z, NT )
				      + GRID_ENTRY( grid, x, y, z, NB ) + GRID_ENTRY( grid, x, y, z, ST )
				      + GRID_ENTRY( grid, x, y, z, SB ) + GRID_ENTRY( grid, x, y, z, ET )
				      + GRID_ENTRY( grid, x, y, z, EB ) + GRID_ENTRY( grid, x, y, z, WT )
				      + GRID_ENTRY( grid, x, y, z, WB );
				ux = + GRID_ENTRY( grid, x, y, z, E  ) - GRID_ENTRY( grid, x, y, z, W  ) 
				     + GRID_ENTRY( grid, x, y, z, NE ) - GRID_ENTRY( grid, x, y, z, NW ) 
				     + GRID_ENTRY( grid, x, y, z, SE ) - GRID_ENTRY( grid, x, y, z, SW ) 
				     + GRID_ENTRY( grid, x, y, z, ET ) + GRID_ENTRY( grid, x, y, z, EB ) 
				     - GRID_ENTRY( grid, x, y, z, WT ) - GRID_ENTRY( grid, x, y, z, WB );
				uy = + GRID_ENTRY( grid, x, y, z, N  ) - GRID_ENTRY( grid, x, y, z, S  ) 
				     + GRID_ENTRY( grid, x, y, z, NE ) + GRID_ENTRY( grid, x, y, z, NW ) 
				     - GRID_ENTRY( grid, x, y, z, SE ) - GRID_ENTRY( grid, x, y, z, SW ) 
				     + GRID_ENTRY( grid, x, y, z, NT ) + GRID_ENTRY( grid, x, y, z, NB ) 
				     - GRID_ENTRY( grid, x, y, z, ST ) - GRID_ENTRY( grid, x, y, z, SB );
				uz = + GRID_ENTRY( grid, x, y, z, T  ) - GRID_ENTRY( grid, x, y, z, B  ) 
				     + GRID_ENTRY( grid, x, y, z, NT ) - GRID_ENTRY( grid, x, y, z, NB ) 
				     + GRID_ENTRY( grid, x, y, z, ST ) - GRID_ENTRY( grid, x, y, z, SB ) 
				     + GRID_ENTRY( grid, x, y, z, ET ) - GRID_ENTRY( grid, x, y, z, EB ) 
				     + GRID_ENTRY( grid, x, y, z, WT ) - GRID_ENTRY( grid, x, y, z, WB );
				ux /= rho;
				uy /= rho;
				uz /= rho;

				if( binary ) {
					loadValue( file, &fileUx );
					loadValue( file, &fileUy );
					loadValue( file, &fileUz );
				}
				else {
					if( sizeof( OUTPUT_PRECISION ) == sizeof( double )) {
						fscanf( file, "%lf %lf %lf\n", &fileUx, &fileUy, &fileUz );
					}
					else {
						fscanf( file, "%lf %lf %lbm_xo_pochoir.cpplf\n", &fileUx, &fileUy, &fileUz );
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

//#define TRACE_BASECASE 1
void co_basecase(LBM_GridPtr* toggle, MAIN_SimType simType,
		 int t0, int t1,
		 int x0, int dx0, int x1, int dx1,
		 int y0, int dy0, int y1, int dy1, 
		 int z0, int dz0, int z1, int dz1)
{
#ifdef TRACE_BASECASE    
    printf("co_basecase(): t0=%d, t1=%d\n", t0, t1);
    printf("\t\t: x0=%d, dx0=%d, x1=%d, dx1=%d\n", x0, dx0, x1, dx1);
    printf("\t\t: y0=%d, dy0=%d, y1=%d, dy1=%d\n", y0, dy0, y1, dy1);
    printf("\t\t: z0=%d, dz0=%d, z1=%d, dz1=%d\n", z0, dz0, z1, dz1);
#endif
    
    LBM_GridPtr src = toggle[(t0+1) & 1];
    LBM_GridPtr dst = toggle[t0 & 1];
    
    for (int t = t0; t < t1; t++)
    {
        //printf("\t\t : t=%d, src=%p, dst=%p, x0=%d, x1=%d, y0=%d, y1=%d, z0=%d, z1=%d\n",
        //       t, src, dst, x0, x1, y0, y1, z0, z1);
        
      if (simType == CHANNEL) {
        LBM_handleInOutFlow( *src, x0, x1, y0, y1, z0, z1);
      }

      LBM_performStreamCollide( *src, *dst, x0, x1, y0, y1, z0, z1);
        
      src = toggle[t & 1];
      dst = toggle[(t+1) & 1];

      x0 += dx0; x1 += dx1;
      y0 += dy0; y1 += dy1;
      z0 += dz0; z1 += dz1;
    }    
}

void co_basecase_1(LBM_GridPtr* toggle,
                   int t0, int t1,
                   int x0, int dx0, int x1, int dx1,
                   int y0, int dy0, int y1, int dy1, 
                   int z0, int dz0, int z1, int dz1)
{
#ifdef TRACE_BASECASE    
    printf("co_basecase_1(): t0=%d, t1=%d\n", t0, t1);
    printf("\t\t: x0=%d, dx0=%d, x1=%d, dx1=%d\n", x0, dx0, x1, dx1);
    printf("\t\t: y0=%d, dy0=%d, y1=%d, dy1=%d\n", y0, dy0, y1, dy1);
    printf("\t\t: z0=%d, dz0=%d, z1=%d, dz1=%d\n", z0, dz0, z1, dz1);
#endif
    
    LBM_GridPtr src = toggle[(t0+1) & 1];
    LBM_GridPtr dst = toggle[t0 & 1];
    
    for (int t = t0; t < t1; t++)
    {
        //printf("\t\t : t=%d, src=%p, dst=%p, x0=%d, x1=%d, y0=%d, y1=%d, z0=%d, z1=%d\n",
        //       t, src, dst, x0, x1, y0, y1, z0, z1);
        
        LBM_handleInOutFlow( *src, x0, x1, y0, y1, z0, z1);
        LBM_performStreamCollide( *src, *dst, x0, x1, y0, y1, z0, z1);
	//LBM_performStreamCollide( *src, *src, x0, x1, y0, y1, z0, z1);
        
        src = toggle[t & 1];
        dst = toggle[(t+1) & 1];

        x0 += dx0; x1 += dx1;
        y0 += dy0; y1 += dy1;
        z0 += dz0; z1 += dz1;
    }    
}


void co_basecase_2(LBM_GridPtr* toggle,
                   int t0, int t1,
                   int x0, int dx0, int x1, int dx1,
                   int y0, int dy0, int y1, int dy1, 
                   int z0, int dz0, int z1, int dz1)
{
#ifdef TRACE_BASECASE    
    printf("co_basecase_2(): t0=%d, t1=%d\n", t0, t1);
    printf("\t\t: x0=%d, dx0=%d, x1=%d, dx1=%d\n", x0, dx0, x1, dx1);
    printf("\t\t: y0=%d, dy0=%d, y1=%d, dy1=%d\n", y0, dy0, y1, dy1);
    printf("\t\t: z0=%d, dz0=%d, z1=%d, dz1=%d\n", z0, dz0, z1, dz1);
#endif
    
    LBM_GridPtr src = toggle[(t0+1) & 1];
    LBM_GridPtr dst = toggle[t0 & 1];
    
    for (int t = t0; t < t1; t++)
    {
        //printf("\t\t : t=%d, src=%p, dst=%p, x0=%d, x1=%d, y0=%d, y1=%d, z0=%d, z1=%d\n",
        //       t, src, dst, x0, x1, y0, y1, z0, z1);
      LBM_performStreamCollide( *src, *dst, x0, x1, y0, y1, z0, z1);
      //LBM_performStreamCollide( *src, *src, x0, x1, y0, y1, z0, z1);
        
      src = toggle[t & 1];
      dst = toggle[(t+1) & 1];

      x0 += dx0; x1 += dx1;
      y0 += dy0; y1 += dy1;
      z0 += dz0; z1 += dz1;
    }    
}

void LBM_handleInOutFlow_Orig( LBM_Grid srcGrid ) {
	MY_TYPE ux , uy , uz , rho ,
	       ux1, uy1, uz1, rho1,
	       ux2, uy2, uz2, rho2,
	       u2, px, py;
	SWEEP_VAR

	/* inflow */
	/*voption indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
//#pragma omp parallel for private( ux, uy, uz, rho, ux1, uy1, uz1, rho1, \
                                  ux2, uy2, uz2, rho2, u2, px, py )
#endif
#endif
	SWEEP_START( 0, 0, 0, 0, 0, 1 )
		rho1 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, C  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, N  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, S  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, E  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, W  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, T  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, B  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, NE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, NW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, SE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, SW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, NT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, NB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, ST )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, SB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, ET )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, EB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, WT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 1, WB );
		rho2 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, C  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, N  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, S  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, E  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, W  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, T  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, B  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, NE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, NW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, SE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, SW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, NT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, NB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, ST )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, SB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, ET )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, EB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, WT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, 2, WB );

		rho = 2.0*rho1 - rho2;

		px = (SWEEP_X / (0.5*(SIZE_X-1))) - 1.0;
		py = (SWEEP_Y / (0.5*(SIZE_Y-1))) - 1.0;
		ux = 0.00;
		uy = 0.00;
		uz = 0.01 * (1.0-px*px) * (1.0-py*py);

		u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

		LOCAL( srcGrid, C ) = DFL1*rho*(1.0                                 - u2);

		LOCAL( srcGrid, N ) = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
		LOCAL( srcGrid, S ) = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
		LOCAL( srcGrid, E ) = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
		LOCAL( srcGrid, W ) = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
		LOCAL( srcGrid, T ) = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
		LOCAL( srcGrid, B ) = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

		LOCAL( srcGrid, NE) = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
		LOCAL( srcGrid, NW) = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
		LOCAL( srcGrid, SE) = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
		LOCAL( srcGrid, SW) = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
		LOCAL( srcGrid, NT) = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
		LOCAL( srcGrid, NB) = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
		LOCAL( srcGrid, ST) = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
		LOCAL( srcGrid, SB) = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
		LOCAL( srcGrid, ET) = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
		LOCAL( srcGrid, EB) = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
		LOCAL( srcGrid, WT) = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
		LOCAL( srcGrid, WB) = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
	SWEEP_END

	/* outflow */
	/*voption indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
//#pragma omp parallel for private( ux, uy, uz, rho, ux1, uy1, uz1, rho1, \
                                  ux2, uy2, uz2, rho2, u2, px, py )            
#endif
#endif

	SWEEP_START( 0, 0, SIZE_Z-1, 0, 0, SIZE_Z )
		rho1 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, C  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, N  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, S  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, E  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, W  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, T  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, B  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, ST )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, ET )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, EB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, WT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, WB );
		ux1 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, E  ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, W  )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NE ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NW )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SE ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SW )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, ET ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, EB )
		      - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, WT ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, WB );
		uy1 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, N  ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, S  )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NE ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NW )
		      - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SE ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SW )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NT ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NB )
		      - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, ST ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SB );
		uz1 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, T  ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, B  )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NT ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, NB )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, ST ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, SB )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, ET ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, EB )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, WT ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -1, WB );

		ux1 /= rho1;
		uy1 /= rho1;
		uz1 /= rho1;

		rho2 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, C  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, N  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, S  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, E  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, W  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, T  )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, B  ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SE )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SW ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, ST )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, ET )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, EB ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, WT )
		       + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, WB );
		ux2 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, E  ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, W  )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NE ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NW )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SE ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SW )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, ET ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, EB )
		      - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, WT ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, WB );
		uy2 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, N  ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, S  )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NE ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NW )
		      - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SE ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SW )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NT ) + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NB )
		      - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, ST ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SB );
		uz2 = + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, T  ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, B  )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NT ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, NB )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, ST ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, SB )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, ET ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, EB )
		      + GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, WT ) - GRID_ENTRY_SWEEP( srcGrid, 0, 0, -2, WB );

		ux2 /= rho2;
		uy2 /= rho2;
		uz2 /= rho2;

		rho = 1.0;

		ux = 2*ux1 - ux2;
		uy = 2*uy1 - uy2;
		uz = 2*uz1 - uz2;

		u2 = 1.5 * (ux*ux + uy*uy + uz*uz);

		LOCAL( srcGrid, C ) = DFL1*rho*(1.0                                 - u2);

		LOCAL( srcGrid, N ) = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
		LOCAL( srcGrid, S ) = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
		LOCAL( srcGrid, E ) = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
		LOCAL( srcGrid, W ) = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
		LOCAL( srcGrid, T ) = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
		LOCAL( srcGrid, B ) = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

		LOCAL( srcGrid, NE) = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
		LOCAL( srcGrid, NW) = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
		LOCAL( srcGrid, SE) = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
		LOCAL( srcGrid, SW) = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
		LOCAL( srcGrid, NT) = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
		LOCAL( srcGrid, NB) = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
		LOCAL( srcGrid, ST) = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
		LOCAL( srcGrid, SB) = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
		LOCAL( srcGrid, ET) = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
		LOCAL( srcGrid, EB) = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
		LOCAL( srcGrid, WT) = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
		LOCAL( srcGrid, WB) = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
	SWEEP_END
}


void LBM_performStreamCollide_Orig ( LBM_Grid srcGrid, LBM_Grid dstGrid ) {
	MY_TYPE ux, uy, uz, u2, rho;

#ifdef ALIGNED_MALLOC        
        __assume_aligned(srcGrid, MY_ALIGNMENT);
        __assume_aligned(dstGrid, MY_ALIGNMENT);
#endif
        
        int i, x, y, z;

	/*voption indep*/
#if !defined(SPEC_CPU)
#ifdef _OPENMP
#pragma omp parallel for private( ux, uy, uz, u2, rho )
#endif
#endif
        for (z = 0; z < SIZE_Z; z++)
        {
            for (y = 0; y < SIZE_Y; y++)
            {
#ifdef VECTORIZE
#pragma simd
#endif                 
                for (x = 0; x < SIZE_X; x++)
                {
                    i = CALC_INDEX(x, y, z, 0);
                    if( TEST_FLAG_SWEEP( srcGrid, OBSTACLE )) {
			DST_C ( dstGrid ) = SRC_C ( srcGrid );
			DST_S ( dstGrid ) = SRC_N ( srcGrid );
			DST_N ( dstGrid ) = SRC_S ( srcGrid );
			DST_W ( dstGrid ) = SRC_E ( srcGrid );
			DST_E ( dstGrid ) = SRC_W ( srcGrid );
			DST_B ( dstGrid ) = SRC_T ( srcGrid );
			DST_T ( dstGrid ) = SRC_B ( srcGrid );
			DST_SW( dstGrid ) = SRC_NE( srcGrid );
			DST_SE( dstGrid ) = SRC_NW( srcGrid );
			DST_NW( dstGrid ) = SRC_SE( srcGrid );
			DST_NE( dstGrid ) = SRC_SW( srcGrid );
			DST_SB( dstGrid ) = SRC_NT( srcGrid );
			DST_ST( dstGrid ) = SRC_NB( srcGrid );
			DST_NB( dstGrid ) = SRC_ST( srcGrid );
			DST_NT( dstGrid ) = SRC_SB( srcGrid );
			DST_WB( dstGrid ) = SRC_ET( srcGrid );
			DST_WT( dstGrid ) = SRC_EB( srcGrid );
			DST_EB( dstGrid ) = SRC_WT( srcGrid );
			DST_ET( dstGrid ) = SRC_WB( srcGrid );
			continue;
                    }

                    rho = + SRC_C ( srcGrid ) + SRC_N ( srcGrid )
                          + SRC_S ( srcGrid ) + SRC_E ( srcGrid )
                          + SRC_W ( srcGrid ) + SRC_T ( srcGrid )
                          + SRC_B ( srcGrid ) + SRC_NE( srcGrid )
                          + SRC_NW( srcGrid ) + SRC_SE( srcGrid )
                          + SRC_SW( srcGrid ) + SRC_NT( srcGrid )
                          + SRC_NB( srcGrid ) + SRC_ST( srcGrid )
                          + SRC_SB( srcGrid ) + SRC_ET( srcGrid )
                          + SRC_EB( srcGrid ) + SRC_WT( srcGrid )
                          + SRC_WB( srcGrid );

                    ux = + SRC_E ( srcGrid ) - SRC_W ( srcGrid )
                         + SRC_NE( srcGrid ) - SRC_NW( srcGrid )
                         + SRC_SE( srcGrid ) - SRC_SW( srcGrid )
                         + SRC_ET( srcGrid ) + SRC_EB( srcGrid )
                         - SRC_WT( srcGrid ) - SRC_WB( srcGrid );
                    uy = + SRC_N ( srcGrid ) - SRC_S ( srcGrid )
                         + SRC_NE( srcGrid ) + SRC_NW( srcGrid )
                         - SRC_SE( srcGrid ) - SRC_SW( srcGrid )
                         + SRC_NT( srcGrid ) + SRC_NB( srcGrid )
                         - SRC_ST( srcGrid ) - SRC_SB( srcGrid );
                    uz = + SRC_T ( srcGrid ) - SRC_B ( srcGrid )
                         + SRC_NT( srcGrid ) - SRC_NB( srcGrid )
                         + SRC_ST( srcGrid ) - SRC_SB( srcGrid )
                         + SRC_ET( srcGrid ) - SRC_EB( srcGrid )
                         + SRC_WT( srcGrid ) - SRC_WB( srcGrid );

                    ux /= rho;
                    uy /= rho;
                    uz /= rho;

                    if( TEST_FLAG_SWEEP( srcGrid, ACCEL )) {
			ux = 0.005;
			uy = 0.002;
			uz = 0.000;
                    }

                    u2 = 1.5 * (ux*ux + uy*uy + uz*uz);
                    DST_C ( dstGrid ) = (1.0-OMEGA)*SRC_C ( srcGrid ) + DFL1*OMEGA*rho*(1.0                                 - u2);

                    DST_N ( dstGrid ) = (1.0-OMEGA)*SRC_N ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2);
                    DST_S ( dstGrid ) = (1.0-OMEGA)*SRC_S ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2);
                    DST_E ( dstGrid ) = (1.0-OMEGA)*SRC_E ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2);
                    DST_W ( dstGrid ) = (1.0-OMEGA)*SRC_W ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2);
                    DST_T ( dstGrid ) = (1.0-OMEGA)*SRC_T ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2);
                    DST_B ( dstGrid ) = (1.0-OMEGA)*SRC_B ( srcGrid ) + DFL2*OMEGA*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2);

                    DST_NE( dstGrid ) = (1.0-OMEGA)*SRC_NE( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2);
                    DST_NW( dstGrid ) = (1.0-OMEGA)*SRC_NW( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2);
                    DST_SE( dstGrid ) = (1.0-OMEGA)*SRC_SE( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2);
                    DST_SW( dstGrid ) = (1.0-OMEGA)*SRC_SW( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2);
                    DST_NT( dstGrid ) = (1.0-OMEGA)*SRC_NT( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2);
                    DST_NB( dstGrid ) = (1.0-OMEGA)*SRC_NB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2);
                    DST_ST( dstGrid ) = (1.0-OMEGA)*SRC_ST( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2);
                    DST_SB( dstGrid ) = (1.0-OMEGA)*SRC_SB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2);
                    DST_ET( dstGrid ) = (1.0-OMEGA)*SRC_ET( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2);
                    DST_EB( dstGrid ) = (1.0-OMEGA)*SRC_EB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2);
                    DST_WT( dstGrid ) = (1.0-OMEGA)*SRC_WT( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2);
                    DST_WB( dstGrid ) = (1.0-OMEGA)*SRC_WB( srcGrid ) + DFL3*OMEGA*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2);
                }
            }
        }
}


Pochoir_Array<PoCellEntry, 3> pa((2*MARGIN_Z+SIZE_Z), SIZE_Y, SIZE_X); 
Pochoir_Domain X(0, SIZE_X), Y(0, SIZE_Y), Z(0+MARGIN_Z, SIZE_Z+MARGIN_Z);
Pochoir_Shape_3D  lbm_shape[8] = {{1, 0, 0, 0},
                                  {0, 0, 0, 0},
                                  {0, 0, 0, +1},  {0, 0, 0, -1},
                                  {0, 0, +1, 0},  {0, 0, -1, 0},
                                  {0, +1, 0, 0},  {0, -1, 0, 0}};

#define Po_handleInOutFlow(t, z, y, x)  if (z == (0 + MARGIN_Z)) { \
                                           /* inflow */						   \
                                          int z1 = z + 1;							\
                                          MY_TYPE rho1 = + PoCellEntry( pa( t, z1, y, x) )._C + PoCellEntry( pa( t, z1, y, x) )._N \
					                 + PoCellEntry( pa( t, z1, y, x) )._S + PoCellEntry( pa( t, z1, y, x) )._E \
							 + PoCellEntry( pa( t, z1, y, x) )._W + PoCellEntry( pa( t, z1, y, x) )._T \
							 + PoCellEntry( pa( t, z1, y, x) )._B + PoCellEntry( pa( t, z1, y, x) )._NE \
							 + PoCellEntry( pa( t, z1, y, x) )._NW + PoCellEntry( pa( t, z1, y, x) )._SE \
							 + PoCellEntry( pa( t, z1, y, x) )._SW + PoCellEntry( pa( t, z1, y, x) )._NT \
							 + PoCellEntry( pa( t, z1, y, x) )._NB + PoCellEntry( pa( t, z1, y, x) )._ST \
							 + PoCellEntry( pa( t, z1, y, x) )._SB + PoCellEntry( pa( t, z1, y, x) )._ET \
							 + PoCellEntry( pa( t, z1, y, x) )._EB + PoCellEntry( pa( t, z1, y, x) )._WT \
							 + PoCellEntry( pa( t, z1, y, x) )._WB; \
					  int z2 = z + 2; \
                                          MY_TYPE rho2 = + PoCellEntry( pa( t, z2, y, x) )._C + PoCellEntry( pa( t, z2, y, x) )._N \
					                 + PoCellEntry( pa( t, z2, y, x) )._S + PoCellEntry( pa( t, z2, y, x) )._E \
							 + PoCellEntry( pa( t, z2, y, x) )._W + PoCellEntry( pa( t, z2, y, x) )._T \
							 + PoCellEntry( pa( t, z2, y, x) )._B + PoCellEntry( pa( t, z2, y, x) )._NE \
							 + PoCellEntry( pa( t, z2, y, x) )._NW + PoCellEntry( pa( t, z2, y, x) )._SE \
							 + PoCellEntry( pa( t, z2, y, x) )._SW + PoCellEntry( pa( t, z2, y, x) )._NT \
							 + PoCellEntry( pa( t, z2, y, x) )._NB + PoCellEntry( pa( t, z2, y, x) )._ST \
							 + PoCellEntry( pa( t, z2, y, x) )._SB + PoCellEntry( pa( t, z2, y, x) )._ET \
							 + PoCellEntry( pa( t, z2, y, x) )._EB + PoCellEntry( pa( t, z2, y, x) )._WT \
							 + PoCellEntry( pa( t, z2, y, x) )._WB; \
					  MY_TYPE rho = 2.0*rho1 - rho2; \
                                          MY_TYPE px = (x / (0.5*(SIZE_X-1))) - 1.0; \
					  MY_TYPE py = (y / (0.5*(SIZE_Y-1))) - 1.0; \
					  MY_TYPE ux = 0.00; \
					  MY_TYPE uy = 0.00; \
					  MY_TYPE uz = 0.01 * (1.0-px*px) * (1.0-py*py); \
					  MY_TYPE u2 = 1.5 * (ux*ux + uy*uy + uz*uz); \
					  PoCellEntry( pa(t, z, y, x) ). _C = DFL1 * rho * (1.0 - u2); \
					  PoCellEntry( pa(t, z, y, x) )._N = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._S = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._E = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._W = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._T = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._B = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._NE = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._NW = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._SE = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._SW = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._NT = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._NB = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._ST = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._SB = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._ET = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._EB = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._WT = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2); \
					  PoCellEntry( pa(t, z, y, x) )._WB = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2); \
                                        } \
                                        if (z == (SIZE_Z - 1 + MARGIN_Z)) { \
					   /* outflow */ \
					  int z1 = z - 1; \
					  MY_TYPE rho1 = + PoCellEntry( pa( t, z1, y, x) )._C + PoCellEntry( pa( t, z1, y, x) )._N \
					                 + PoCellEntry( pa( t, z1, y, x) )._S + PoCellEntry( pa( t, z1, y, x) )._E \
							 + PoCellEntry( pa( t, z1, y, x) )._W + PoCellEntry( pa( t, z1, y, x) )._T \
							 + PoCellEntry( pa( t, z1, y, x) )._B + PoCellEntry( pa( t, z1, y, x) )._NE \
							 + PoCellEntry( pa( t, z1, y, x) )._NW + PoCellEntry( pa( t, z1, y, x) )._SE \
							 + PoCellEntry( pa( t, z1, y, x) )._SW + PoCellEntry( pa( t, z1, y, x) )._NT \
							 + PoCellEntry( pa( t, z1, y, x) )._NB + PoCellEntry( pa( t, z1, y, x) )._ST \
							 + PoCellEntry( pa( t, z1, y, x) )._SB + PoCellEntry( pa( t, z1, y, x) )._ET \
							 + PoCellEntry( pa( t, z1, y, x) )._EB + PoCellEntry( pa( t, z1, y, x) )._WT \
							 + PoCellEntry( pa( t, z1, y, x) )._WB; \
					  MY_TYPE ux1 = + PoCellEntry( pa(t, z1, y, x) )._E  - PoCellEntry( pa(t, z1, y, x) )._W \
					    + PoCellEntry( pa(t, z1, y, x) )._NE - PoCellEntry( pa(t, z1, y, x) )._NW \
					    + PoCellEntry( pa(t, z1, y, x) )._SE - PoCellEntry( pa(t, z1, y, x) )._SW \
					    + PoCellEntry( pa(t, z1, y, x) )._ET + PoCellEntry( pa(t, z1, y, x) )._EB \
					    - PoCellEntry( pa(t, z1, y, x) )._WT - PoCellEntry( pa(t, z1, y, x) )._WB; \
					  MY_TYPE uy1 = + PoCellEntry( pa(t, z1, y, x) )._N  - PoCellEntry( pa(t, z1, y, x) )._S \
					    + PoCellEntry( pa(t, z1, y, x) )._NE + PoCellEntry( pa(t, z1, y, x) )._NW \
					    - PoCellEntry( pa(t, z1, y, x) )._SE - PoCellEntry( pa(t, z1, y, x) )._SW \
					    + PoCellEntry( pa(t, z1, y, x) )._NT + PoCellEntry( pa(t, z1, y, x) )._NB \
					    - PoCellEntry( pa(t, z1, y, x) )._ST - PoCellEntry( pa(t, z1, y, x) )._SB; \
					  MY_TYPE uz1 = + PoCellEntry( pa(t, z1, y, x) )._T  - PoCellEntry( pa(t, z1, y, x) )._B  \
					    + PoCellEntry( pa(t, z1, y, x) )._NT - PoCellEntry( pa(t, z1, y, x) )._NB \
					    + PoCellEntry( pa(t, z1, y, x) )._ST - PoCellEntry( pa(t, z1, y, x) )._SB \
					    + PoCellEntry( pa(t, z1, y, x) )._ET - PoCellEntry( pa(t, z1, y, x) )._EB \
					    + PoCellEntry( pa(t, z1, y, x) )._WT - PoCellEntry( pa(t, z1, y, x) )._WB; \
					  ux1 /= rho1; \
                                          uy1 /= rho1; \
					  uz1 /= rho1; \
					  int z2 = z - 2;		\
					  MY_TYPE rho2 = + PoCellEntry( pa(t, z2, y, x) )._C  + PoCellEntry( pa(t, z2, y, x) )._N \
					    + PoCellEntry( pa(t, z2, y, x) )._S  + PoCellEntry( pa(t, z2, y, x) )._E \
					    + PoCellEntry( pa(t, z2, y, x) )._W  + PoCellEntry( pa(t, z2, y, x) )._T \
					    + PoCellEntry( pa(t, z2, y, x) )._B  + PoCellEntry( pa(t, z2, y, x) )._NE \
					    + PoCellEntry( pa(t, z2, y, x) )._NW + PoCellEntry( pa(t, z2, y, x) )._SE \
					    + PoCellEntry( pa(t, z2, y, x) )._SW + PoCellEntry( pa(t, z2, y, x) )._NT \
					    + PoCellEntry( pa(t, z2, y, x) )._NB + PoCellEntry( pa(t, z2, y, x) )._ST \
					    + PoCellEntry( pa(t, z2, y, x) )._SB + PoCellEntry( pa(t, z2, y, x) )._ET \
					    + PoCellEntry( pa(t, z2, y, x) )._EB + PoCellEntry( pa(t, z2, y, x) )._WT \
					    + PoCellEntry( pa(t, z2, y, x) )._WB; \
                                            MY_TYPE ux2 = + PoCellEntry( pa(t, z2, y, x) )._E  - PoCellEntry( pa(t, z2, y, x) )._W \
					   + PoCellEntry( pa(t, z2, y, x) )._NE - PoCellEntry( pa(t, z2, y, x) )._NW \
					   + PoCellEntry( pa(t, z2, y, x) )._SE - PoCellEntry( pa(t, z2, y, x) )._SW \
					   + PoCellEntry( pa(t, z2, y, x) )._ET + PoCellEntry( pa(t, z2, y, x) )._EB \
					   - PoCellEntry( pa(t, z2, y, x) )._WT - PoCellEntry( pa(t, z2, y, x) )._WB; \
                                         MY_TYPE uy2 = + PoCellEntry( pa(t, z2, y, x) )._N  - PoCellEntry( pa(t, z2, y, x) )._S  \
					   + PoCellEntry( pa(t, z2, y, x) )._NE + PoCellEntry( pa(t, z2, y, x) )._NW \
					   - PoCellEntry( pa(t, z2, y, x) )._SE - PoCellEntry( pa(t, z2, y, x) )._SW \
					   + PoCellEntry( pa(t, z2, y, x) )._NT + PoCellEntry( pa(t, z2, y, x) )._NB \
					   - PoCellEntry( pa(t, z2, y, x) )._ST - PoCellEntry( pa(t, z2, y, x) )._SB; \
                                         MY_TYPE uz2 = + PoCellEntry( pa(t, z2, y, x) )._T  - PoCellEntry( pa(t, z2, y, x) )._B  \
					   + PoCellEntry( pa(t, z2, y, x) )._NT - PoCellEntry( pa(t, z2, y, x) )._NB \
					   + PoCellEntry( pa(t, z2, y, x) )._ST - PoCellEntry( pa(t, z2, y, x) )._SB \
					   + PoCellEntry( pa(t, z2, y, x) )._ET - PoCellEntry( pa(t, z2, y, x) )._EB \
					   + PoCellEntry( pa(t, z2, y, x) )._WT - PoCellEntry( pa(t, z2, y, x) )._WB; \
                                         ux2 /= rho2; \
                                         uy2 /= rho2; \
                                         uz2 /= rho2; \
                                         MY_TYPE rho = 1.0; \
                                         MY_TYPE ux = 2*ux1 - ux2; \
                                         MY_TYPE uy = 2*uy1 - uy2; \
                                         MY_TYPE uz = 2*uz1 - uz2; \
                                         MY_TYPE u2 = 1.5 * (ux*ux + uy*uy + uz*uz); \
                                         PoCellEntry( pa(t, z, y, x) )._C  = DFL1*rho*(1.0                                 - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._N  = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._S  = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._E  = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._W  = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._T  = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._B  = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._NE = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._NW = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._SE = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._SW = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._NT = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._NB = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._ST = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._SB = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._ET = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._EB = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._WT = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2); \
                                         PoCellEntry( pa(t, z, y, x) )._WB = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2); \
                                       }

#define Po_performStreamCollide(t, z, y, x)			\
  DEBUG_MSG("line 1");						\
  const MY_TYPE src_c = PoCellEntry( pa(t, z, y, x) )._C;	\
  const MY_TYPE src_n = PoCellEntry( pa(t, z, y, x) )._N;	\
  const MY_TYPE src_s = PoCellEntry( pa(t, z, y, x) )._S;	\
  const MY_TYPE src_e = PoCellEntry( pa(t, z, y, x) )._E;	\
  const MY_TYPE src_w = PoCellEntry( pa(t, z, y, x) )._W;	\
  const MY_TYPE src_t = PoCellEntry( pa(t, z, y, x) )._T;	\
  const MY_TYPE src_b = PoCellEntry( pa(t, z, y, x) )._B;	\
  const MY_TYPE src_ne = PoCellEntry( pa(t, z, y, x) )._NE;     \
  const MY_TYPE src_nw = PoCellEntry( pa(t, z, y, x) )._NW;     \
  const MY_TYPE src_se = PoCellEntry( pa(t, z, y, x) )._SE;     \
  const MY_TYPE src_sw = PoCellEntry( pa(t, z, y, x) )._SW;     \
  const MY_TYPE src_nt = PoCellEntry( pa(t, z, y, x) )._NT;     \
  const MY_TYPE src_nb = PoCellEntry( pa(t, z, y, x) )._NB;     \
  const MY_TYPE src_st = PoCellEntry( pa(t, z, y, x) )._ST;     \
  const MY_TYPE src_sb = PoCellEntry( pa(t, z, y, x) )._SB;     \
  const MY_TYPE src_et = PoCellEntry( pa(t, z, y, x) )._ET;     \
  const MY_TYPE src_eb = PoCellEntry( pa(t, z, y, x) )._EB;     \
  const MY_TYPE src_wt = PoCellEntry( pa(t, z, y, x) )._WT;     \
  const MY_TYPE src_wb = PoCellEntry( pa(t, z, y, x) )._WB;     \
  const MY_TYPE*src_flag_addr = &( PoCellEntry( pa(t, z, y, x) )._FLAGS); \
  DEBUG_MSG("line 2"); \
  MY_TYPE* dst_c = &( PoCellEntry( pa(t+1, z, y, x) )._C);    \
  MY_TYPE* dst_n = &( PoCellEntry( pa(t+1, z, y+1, x) )._N);  \
  MY_TYPE* dst_s = &( PoCellEntry( pa(t+1, z, y-1, x) )._S); \
  MY_TYPE* dst_e = &( PoCellEntry( pa(t+1, z, y, x+1) )._E); \
  MY_TYPE* dst_w = &( PoCellEntry( pa(t+1, z, y, x-1) )._W); \
  MY_TYPE* dst_t = &( PoCellEntry( pa(t+1, z+1, y, x) )._T);    \
  MY_TYPE* dst_b = &( PoCellEntry( pa(t+1, z-1, y, x) )._B);    \
  MY_TYPE* dst_ne = &( PoCellEntry( pa(t+1, z, y+1, x+1) )._NE);	\
  MY_TYPE* dst_nw = &( PoCellEntry( pa(t+1, z, y+1, x-1) )._NW);	\
  MY_TYPE* dst_se = &( PoCellEntry( pa(t+1, z, y-1, x+1) )._SE);	\
  MY_TYPE* dst_sw = &( PoCellEntry( pa(t+1, z, y-1, x-1) )._SW);	\
  MY_TYPE* dst_nt = &( PoCellEntry( pa(t+1, z+1, y+1, x) )._NT);	\
  MY_TYPE* dst_nb = &( PoCellEntry( pa(t+1, z-1, y+1, x) )._NB);	\
  MY_TYPE* dst_st = &( PoCellEntry( pa(t+1, z+1, y-1, x) )._ST);	\
  MY_TYPE* dst_sb = &( PoCellEntry( pa(t+1, z-1, y-1, x) )._SB);	\
  MY_TYPE* dst_et = &( PoCellEntry( pa(t+1, z+1, y, x+1) )._ET);    \
  MY_TYPE* dst_eb = &( PoCellEntry( pa(t+1, z-1, y, x+1) )._EB);    \
  MY_TYPE* dst_wt = &( PoCellEntry( pa(t+1, z+1, y, x-1) )._WT);    \
  MY_TYPE* dst_wb = &( PoCellEntry( pa(t+1, z-1, y, x-1) )._WB); \
  DEBUG_MSG("line 3"); \
  if ( *((unsigned int*)src_flag_addr) & OBSTACLE) {		 \
    DEBUG_MSG("line 4"); \
      *dst_c  = src_c; \
      *dst_s  = src_n; \
      *dst_n  = src_s; \
      *dst_w  = src_e; \
      *dst_e  = src_w; \
      *dst_b  = src_t; \
      *dst_t  = src_b; \
      *dst_sw = src_ne; \
      *dst_se = src_nw; \
      *dst_nw = src_se; \
      *dst_ne = src_sw; \
      *dst_sb = src_nt; \
      *dst_st = src_nb; \
      *dst_nb = src_st; \
      *dst_nt = src_sb; \
      *dst_wb = src_et; \
      *dst_wt = src_eb; \
      *dst_eb = src_wt; \
      *dst_et = src_wb; \
  DEBUG_MSG("line 5"); \
  } else { \
      DEBUG_MSG("line 6");		      \
      MY_TYPE rho = + src_c  + src_n  \
                      + src_s  + src_e \
                      + src_w  + src_t  \
                      + src_b  + src_ne \
                      + src_nw + src_se \
                      + src_sw + src_nt \
                      + src_nb + src_st \
                      + src_sb + src_et \
                      + src_eb + src_wt \
                      + src_wb; \
      MY_TYPE ux = + src_e  - src_w \
                     + src_ne - src_nw \
                     + src_se - src_sw \
                     + src_et + src_eb \
                     - src_wt - src_wb; \
      MY_TYPE uy = + src_n  - src_s \
                     + src_ne + src_nw \
                     - src_se - src_sw \
                     + src_nt + src_nb \
                     - src_st - src_sb; \
      MY_TYPE uz = + src_t  - src_b \
	             + src_nt - src_nb \
                     + src_st - src_sb \
                     + src_et - src_eb \
                     + src_wt - src_wb; \
      ux /= rho; \
      uy /= rho; \
      uz /= rho; \
      if( *((unsigned int*)src_flag_addr) & ACCEL) { \
         ux = 0.005; \
	 uy = 0.002; \
	 uz = 0.000; \
      } \
      const MY_TYPE u2 = 1.5 * (ux*ux + uy*uy + uz*uz); \
      const MY_TYPE k0 = (1.0-OMEGA); \
      const MY_TYPE k1 = (DFL1*OMEGA*rho); \
      const MY_TYPE k2 = (DFL2*OMEGA*rho); \
      const MY_TYPE k3 = (DFL3*OMEGA*rho); \		
      *dst_c  = k0*src_c  + k1*(1.0                                 - u2); \                
      *dst_n  = k0*src_n  + k2*(1.0 +       uy*(4.5*uy       + 3.0) - u2); \
      *dst_s  = k0*src_s  + k2*(1.0 +       uy*(4.5*uy       - 3.0) - u2); \
      *dst_e  = k0*src_e  + k2*(1.0 +       ux*(4.5*ux       + 3.0) - u2); \
      *dst_w  = k0*src_w  + k2*(1.0 +       ux*(4.5*ux       - 3.0) - u2); \
      *dst_t  = k0*src_t  + k2*(1.0 +       uz*(4.5*uz       + 3.0) - u2); \
      *dst_b  = k0*src_b  + k2*(1.0 +       uz*(4.5*uz       - 3.0) - u2); \        
      *dst_ne = k0*src_ne + k3*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2); \
      *dst_nw = k0*src_nw + k3*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2); \
      *dst_se = k0*src_se + k3*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2); \
      *dst_sw = k0*src_sw + k3*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2); \
      *dst_nt = k0*src_nt + k3*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2); \
      *dst_nb = k0*src_nb + k3*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2); \
      *dst_st = k0*src_st + k3*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2); \
      *dst_sb = k0*src_sb + k3*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2); \
      *dst_et = k0*src_et + k3*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2); \
      *dst_eb = k0*src_eb + k3*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2); \
      *dst_wt = k0*src_wt + k3*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2); \
      *dst_wb = k0*src_wb + k3*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2); \
    DEBUG_MSG("line 7");					 \
  }

static MAIN_SimType mySimType;

      /*

       */

static void CopyLbmGridToPochoirGrid(LBM_Grid lbmGrid, Pochoir_Array<PoCellEntry, 3>& parr, const int t)
{
    for (int z=0; z<SIZE_Z; z++)
    {
        const int new_z = z + MARGIN_Z;
        for (int y=0; y<SIZE_Y; y++)
        {
            for (int x=0; x<SIZE_X; x++)
            { 
                const int i = CALC_INDEX(z, y, x, 0);
                parr.interior(t, new_z, y, x)._C  =  LOCAL(lbmGrid, C);
                parr.interior(t, new_z, y, x)._N  =  LOCAL(lbmGrid, N);
                parr.interior(t, new_z, y, x)._S  =  LOCAL(lbmGrid, S);
                parr.interior(t, new_z, y, x)._E  =  LOCAL(lbmGrid, E);
                parr.interior(t, new_z, y, x)._W  =  LOCAL(lbmGrid, W);
                parr.interior(t, new_z, y, x)._T  =  LOCAL(lbmGrid, T);
                parr.interior(t, new_z, y, x)._B  =  LOCAL(lbmGrid, B);
                parr.interior(t, new_z, y, x)._NE =  LOCAL(lbmGrid, NE);
                parr.interior(t, new_z, y, x)._NW =  LOCAL(lbmGrid, NW);
                parr.interior(t, new_z, y, x)._SE =  LOCAL(lbmGrid, SE);
                parr.interior(t, new_z, y, x)._SW =  LOCAL(lbmGrid, SW);
                parr.interior(t, new_z, y, x)._NT =  LOCAL(lbmGrid, NT);
                parr.interior(t, new_z, y, x)._NB =  LOCAL(lbmGrid, NB);
                parr.interior(t, new_z, y, x)._ST =  LOCAL(lbmGrid, ST);
                parr.interior(t, new_z, y, x)._SB =  LOCAL(lbmGrid, SB);
                parr.interior(t, new_z, y, x)._ET =  LOCAL(lbmGrid, ET);
                parr.interior(t, new_z, y, x)._EB =  LOCAL(lbmGrid, EB);
                parr.interior(t, new_z, y, x)._WT =  LOCAL(lbmGrid, WT);
                parr.interior(t, new_z, y, x)._WB =  LOCAL(lbmGrid, WB);
                parr.interior(t, new_z, y, x)._FLAGS =  LOCAL(lbmGrid, FLAGS);
            }
        }
    }    
}

static void CopyPochoirGridToLbmGrid(LBM_Grid lbmGrid, Pochoir_Array<PoCellEntry, 3>& parr, const int t)
{
  for (int z=0; z<SIZE_Z; z++)
    {
        const int new_z = z + MARGIN_Z;
        for (int y=0; y<SIZE_Y; y++)
        {
            for (int x=0; x<SIZE_X; x++)
            {
                const int i = CALC_INDEX(z, y, x, 0);
                LOCAL(lbmGrid, C)  = PoCellEntry( parr(t, new_z, y, x) )._C;
                LOCAL(lbmGrid, N)  = PoCellEntry( parr(t, new_z, y, x) )._N;
                LOCAL(lbmGrid, S)  = PoCellEntry( parr(t, new_z, y, x) )._S;
                LOCAL(lbmGrid, E)  = PoCellEntry( parr(t, new_z, y, x) )._E;
                LOCAL(lbmGrid, W)  = PoCellEntry( parr(t, new_z, y, x) )._W;
                LOCAL(lbmGrid, T)  = PoCellEntry( parr(t, new_z, y, x) )._T;
                LOCAL(lbmGrid, B)  = PoCellEntry( parr(t, new_z, y, x) )._B;                
                LOCAL(lbmGrid, NE) = PoCellEntry( parr(t, new_z, y, x) )._NE;
                LOCAL(lbmGrid, NW) = PoCellEntry( parr(t, new_z, y, x) )._NW;
                LOCAL(lbmGrid, SE) = PoCellEntry( parr(t, new_z, y, x) )._SE;
                LOCAL(lbmGrid, SW) = PoCellEntry( parr(t, new_z, y, x) )._SW;
                LOCAL(lbmGrid, NT) = PoCellEntry( parr(t, new_z, y, x) )._NT;
		LOCAL(lbmGrid, NB) = PoCellEntry( parr(t, new_z, y, x) )._NB;
                LOCAL(lbmGrid, ST) = PoCellEntry( parr(t, new_z, y, x) )._ST;
                LOCAL(lbmGrid, SB) = PoCellEntry( parr(t, new_z, y, x) )._SB;
                LOCAL(lbmGrid, ET) = PoCellEntry( parr(t, new_z, y, x) )._ET;
                LOCAL(lbmGrid, EB) = PoCellEntry( parr(t, new_z, y, x) )._EB;
                LOCAL(lbmGrid, WT) = PoCellEntry( parr(t, new_z, y, x) )._WT;
                LOCAL(lbmGrid, WB) = PoCellEntry( parr(t, new_z, y, x) )._WB;
                LOCAL(lbmGrid, FLAGS) = PoCellEntry( parr(t, new_z, y, x) )._FLAGS;
            }
        }
    }    
}

#if 0
static void ValidateCopyingBetweenPochoirGridAndLbmGrid(LBM_Grid lbmGrid, Pochoir_Array<PoCellEntry, 3>& parr, const int t)
{
  for (int z=0; z<SIZE_Z; z++)
    {
      const int new_z = z + MARGIN_Z;
        for (int y=0; y<SIZE_Y; y++)
        {
            for (int x=0; x<SIZE_X; x++)
            {
                const int i = CALC_INDEX(z, y, x, 0);
		const MY_TYPE orig = LOCAL(lbmGrid, FLAGS);
		PoCellEntry( parr(t, new_z, y, x) )._FLAGS = orig;
		const MY_TYPE tmp = PoCellEntry( parr(t, new_z, y, x) )._FLAGS;
		if (orig != tmp) {
		  printf("ValidateCopying failed at x=%d, y=%d, z=%d\n", z, y, x);
		  exit(-1);
		}
	    }
	}
    }
  printf("ValidateCopying succeeded\n");
}
#endif
    
void RunPochoir(MAIN_SimType simtype, LBM_Grid srcGrid, LBM_Grid dstGrid, int numTimeSteps)
{

  mySimType = simtype;      
  //printf("RunPochoir, numTimeSteps=%d\n", numTimeSteps);  
  Pochoir<PoCellEntry, 3> lbm;
  lbm.Register_Array(pa);

  CopyLbmGridToPochoirGrid(srcGrid, pa, 0);
  CopyLbmGridToPochoirGrid(dstGrid, pa, 1);

  lbm.Register_Shape(lbm_shape);
  lbm.Register_Domain(Z, Y, X);

#define z1 (z + 1)
#define z2 (z + 2)
  Pochoir_Kernel_3D(lbm_kernel, t, z, y, x)
//    Po_handleInOutFlow(t, z, y, x);
    if (z == (0 + MARGIN_Z)) { 
        /* inflow */ 
        MY_TYPE rho1 = + PoCellEntry( pa( t, z1, y, x) )._C + PoCellEntry( pa( t, z1, y, x) )._N + PoCellEntry( pa( t, z1, y, x) )._S + PoCellEntry( pa( t, z1, y, x) )._E + PoCellEntry( pa( t, z1, y, x) )._W + PoCellEntry( pa( t, z1, y, x) )._T + PoCellEntry( pa( t, z1, y, x) )._B + PoCellEntry( pa( t, z1, y, x) )._NE + PoCellEntry( pa( t, z1, y, x) )._NW + PoCellEntry( pa( t, z1, y, x) )._SE + PoCellEntry( pa( t, z1, y, x) )._SW + PoCellEntry( pa( t, z1, y, x) )._NT + PoCellEntry( pa( t, z1, y, x) )._NB + PoCellEntry( pa( t, z1, y, x) )._ST + PoCellEntry( pa( t, z1, y, x) )._SB + PoCellEntry( pa( t, z1, y, x) )._ET + PoCellEntry( pa( t, z1, y, x) )._EB + PoCellEntry( pa( t, z1, y, x) )._WT + PoCellEntry( pa( t, z1, y, x) )._WB; 
        MY_TYPE rho2 = + PoCellEntry( pa( t, z2, y, x) )._C + PoCellEntry( pa( t, z2, y, x) )._N + PoCellEntry( pa( t, z2, y, x) )._S + PoCellEntry( pa( t, z2, y, x) )._E + PoCellEntry( pa( t, z2, y, x) )._W + PoCellEntry( pa( t, z2, y, x) )._T + PoCellEntry( pa( t, z2, y, x) )._B + PoCellEntry( pa( t, z2, y, x) )._NE + PoCellEntry( pa( t, z2, y, x) )._NW + PoCellEntry( pa( t, z2, y, x) )._SE + PoCellEntry( pa( t, z2, y, x) )._SW + PoCellEntry( pa( t, z2, y, x) )._NT + PoCellEntry( pa( t, z2, y, x) )._NB + PoCellEntry( pa( t, z2, y, x) )._ST + PoCellEntry( pa( t, z2, y, x) )._SB + PoCellEntry( pa( t, z2, y, x) )._ET + PoCellEntry( pa( t, z2, y, x) )._EB + PoCellEntry( pa( t, z2, y, x) )._WT + PoCellEntry( pa( t, z2, y, x) )._WB; 
        MY_TYPE rho = 2.0*rho1 - rho2; 
        MY_TYPE px = (x / (0.5*(SIZE_X-1))) - 1.0; 
        MY_TYPE py = (y / (0.5*(SIZE_Y-1))) - 1.0; 
        MY_TYPE ux = 0.00; 
        MY_TYPE uy = 0.00; 
        MY_TYPE uz = 0.01 * (1.0-px*px) * (1.0-py*py); 
        MY_TYPE u2 = 1.5 * (ux*ux + uy*uy + uz*uz); 
        PoCellEntry( pa(t, z, y, x) ). _C = DFL1 * rho * (1.0 - u2); 
        PoCellEntry( pa(t, z, y, x) )._N = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._S = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._E = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._W = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._T = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._B = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._NE = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._NW = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._SE = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._SW = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._NT = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._NB = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._ST = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._SB = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._ET = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._EB = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._WT = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2); 
        PoCellEntry( pa(t, z, y, x) )._WB = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2); 
    } 
        if (z == (SIZE_Z - 1 + MARGIN_Z)) { 
            /* outflow */ 
//            int z1 = z - 1; 
            MY_TYPE rho1 = + PoCellEntry( pa( t, z1, y, x) )._C + PoCellEntry( pa( t, z1, y, x) )._N + PoCellEntry( pa( t, z1, y, x) )._S + PoCellEntry( pa( t, z1, y, x) )._E + PoCellEntry( pa( t, z1, y, x) )._W + PoCellEntry( pa( t, z1, y, x) )._T + PoCellEntry( pa( t, z1, y, x) )._B + PoCellEntry( pa( t, z1, y, x) )._NE + PoCellEntry( pa( t, z1, y, x) )._NW + PoCellEntry( pa( t, z1, y, x) )._SE + PoCellEntry( pa( t, z1, y, x) )._SW + PoCellEntry( pa( t, z1, y, x) )._NT + PoCellEntry( pa( t, z1, y, x) )._NB + PoCellEntry( pa( t, z1, y, x) )._ST + PoCellEntry( pa( t, z1, y, x) )._SB + PoCellEntry( pa( t, z1, y, x) )._ET + PoCellEntry( pa( t, z1, y, x) )._EB + PoCellEntry( pa( t, z1, y, x) )._WT + PoCellEntry( pa( t, z1, y, x) )._WB; 
            MY_TYPE ux1 = + PoCellEntry( pa(t, z1, y, x) )._E  - PoCellEntry( pa(t, z1, y, x) )._W + PoCellEntry( pa(t, z1, y, x) )._NE - PoCellEntry( pa(t, z1, y, x) )._NW + PoCellEntry( pa(t, z1, y, x) )._SE - PoCellEntry( pa(t, z1, y, x) )._SW + PoCellEntry( pa(t, z1, y, x) )._ET + PoCellEntry( pa(t, z1, y, x) )._EB - PoCellEntry( pa(t, z1, y, x) )._WT - PoCellEntry( pa(t, z1, y, x) )._WB; 
            MY_TYPE uy1 = + PoCellEntry( pa(t, z1, y, x) )._N  - PoCellEntry( pa(t, z1, y, x) )._S + PoCellEntry( pa(t, z1, y, x) )._NE + PoCellEntry( pa(t, z1, y, x) )._NW - PoCellEntry( pa(t, z1, y, x) )._SE - PoCellEntry( pa(t, z1, y, x) )._SW + PoCellEntry( pa(t, z1, y, x) )._NT + PoCellEntry( pa(t, z1, y, x) )._NB - PoCellEntry( pa(t, z1, y, x) )._ST - PoCellEntry( pa(t, z1, y, x) )._SB; 
            MY_TYPE uz1 = + PoCellEntry( pa(t, z1, y, x) )._T  - PoCellEntry( pa(t, z1, y, x) )._B  + PoCellEntry( pa(t, z1, y, x) )._NT - PoCellEntry( pa(t, z1, y, x) )._NB + PoCellEntry( pa(t, z1, y, x) )._ST - PoCellEntry( pa(t, z1, y, x) )._SB + PoCellEntry( pa(t, z1, y, x) )._ET - PoCellEntry( pa(t, z1, y, x) )._EB + PoCellEntry( pa(t, z1, y, x) )._WT - PoCellEntry( pa(t, z1, y, x) )._WB; 
            ux1 /= rho1; 
            uy1 /= rho1; 
            uz1 /= rho1; 
//            int z2 = z - 2;		
            MY_TYPE rho2 = + PoCellEntry( pa(t, z2, y, x) )._C  + PoCellEntry( pa(t, z2, y, x) )._N + PoCellEntry( pa(t, z2, y, x) )._S  + PoCellEntry( pa(t, z2, y, x) )._E + PoCellEntry( pa(t, z2, y, x) )._W  + PoCellEntry( pa(t, z2, y, x) )._T + PoCellEntry( pa(t, z2, y, x) )._B  + PoCellEntry( pa(t, z2, y, x) )._NE + PoCellEntry( pa(t, z2, y, x) )._NW + PoCellEntry( pa(t, z2, y, x) )._SE + PoCellEntry( pa(t, z2, y, x) )._SW + PoCellEntry( pa(t, z2, y, x) )._NT + PoCellEntry( pa(t, z2, y, x) )._NB + PoCellEntry( pa(t, z2, y, x) )._ST + PoCellEntry( pa(t, z2, y, x) )._SB + PoCellEntry( pa(t, z2, y, x) )._ET + PoCellEntry( pa(t, z2, y, x) )._EB + PoCellEntry( pa(t, z2, y, x) )._WT + PoCellEntry( pa(t, z2, y, x) )._WB; 
            MY_TYPE ux2 = + PoCellEntry( pa(t, z2, y, x) )._E  - PoCellEntry( pa(t, z2, y, x) )._W + PoCellEntry( pa(t, z2, y, x) )._NE - PoCellEntry( pa(t, z2, y, x) )._NW + PoCellEntry( pa(t, z2, y, x) )._SE - PoCellEntry( pa(t, z2, y, x) )._SW + PoCellEntry( pa(t, z2, y, x) )._ET + PoCellEntry( pa(t, z2, y, x) )._EB - PoCellEntry( pa(t, z2, y, x) )._WT - PoCellEntry( pa(t, z2, y, x) )._WB; 
            MY_TYPE uy2 = + PoCellEntry( pa(t, z2, y, x) )._N  - PoCellEntry( pa(t, z2, y, x) )._S  + PoCellEntry( pa(t, z2, y, x) )._NE + PoCellEntry( pa(t, z2, y, x) )._NW - PoCellEntry( pa(t, z2, y, x) )._SE - PoCellEntry( pa(t, z2, y, x) )._SW + PoCellEntry( pa(t, z2, y, x) )._NT + PoCellEntry( pa(t, z2, y, x) )._NB - PoCellEntry( pa(t, z2, y, x) )._ST - PoCellEntry( pa(t, z2, y, x) )._SB; 
            MY_TYPE uz2 = + PoCellEntry( pa(t, z2, y, x) )._T  - PoCellEntry( pa(t, z2, y, x) )._B  + PoCellEntry( pa(t, z2, y, x) )._NT - PoCellEntry( pa(t, z2, y, x) )._NB + PoCellEntry( pa(t, z2, y, x) )._ST - PoCellEntry( pa(t, z2, y, x) )._SB + PoCellEntry( pa(t, z2, y, x) )._ET - PoCellEntry( pa(t, z2, y, x) )._EB + PoCellEntry( pa(t, z2, y, x) )._WT - PoCellEntry( pa(t, z2, y, x) )._WB; 
            ux2 /= rho2; 
            uy2 /= rho2; 
            uz2 /= rho2; 
            MY_TYPE rho = 1.0; 
            MY_TYPE ux = 2*ux1 - ux2; 
            MY_TYPE uy = 2*uy1 - uy2; 
            MY_TYPE uz = 2*uz1 - uz2; 
            MY_TYPE u2 = 1.5 * (ux*ux + uy*uy + uz*uz); 
            PoCellEntry( pa(t, z, y, x) )._C  = DFL1*rho*(1.0 - u2); 
            PoCellEntry( pa(t, z, y, x) )._N  = DFL2*rho*(1.0 +       uy*(4.5*uy       + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._S  = DFL2*rho*(1.0 +       uy*(4.5*uy       - 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._E  = DFL2*rho*(1.0 +       ux*(4.5*ux       + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._W  = DFL2*rho*(1.0 +       ux*(4.5*ux       - 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._T  = DFL2*rho*(1.0 +       uz*(4.5*uz       + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._B  = DFL2*rho*(1.0 +       uz*(4.5*uz       - 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._NE = DFL3*rho*(1.0 + (+ux+uy)*(4.5*(+ux+uy) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._NW = DFL3*rho*(1.0 + (-ux+uy)*(4.5*(-ux+uy) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._SE = DFL3*rho*(1.0 + (+ux-uy)*(4.5*(+ux-uy) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._SW = DFL3*rho*(1.0 + (-ux-uy)*(4.5*(-ux-uy) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._NT = DFL3*rho*(1.0 + (+uy+uz)*(4.5*(+uy+uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._NB = DFL3*rho*(1.0 + (+uy-uz)*(4.5*(+uy-uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._ST = DFL3*rho*(1.0 + (-uy+uz)*(4.5*(-uy+uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._SB = DFL3*rho*(1.0 + (-uy-uz)*(4.5*(-uy-uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._ET = DFL3*rho*(1.0 + (+ux+uz)*(4.5*(+ux+uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._EB = DFL3*rho*(1.0 + (+ux-uz)*(4.5*(+ux-uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._WT = DFL3*rho*(1.0 + (-ux+uz)*(4.5*(-ux+uz) + 3.0) - u2); 
            PoCellEntry( pa(t, z, y, x) )._WB = DFL3*rho*(1.0 + (-ux-uz)*(4.5*(-ux-uz) + 3.0) - u2); 
        }
    Po_performStreamCollide(t, z, y, x);
  Pochoir_Kernel_End

  lbm.Run(numTimeSteps, lbm_kernel);   
   
  CopyPochoirGridToLbmGrid(srcGrid, pa, 0);
  CopyPochoirGridToLbmGrid(dstGrid, pa, 1);
}


