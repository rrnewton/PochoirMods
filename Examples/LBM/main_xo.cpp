/* $Id: main.c,v 1.4 2004/04/21 04:23:43 pohlt Exp $ */

/*############################################################################*/

#include "main.h"
//#define MY_EXTERN extern "C"
#define MY_EXTERN extern
#include "lbm_co.h"
#include "lbm_xo.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include <math.h>
#include <cilk.h>

#if defined(SPEC_CPU)
#   include <time.h>
#else
#   include <sys/times.h>
#   include <unistd.h>
#endif

#include <sys/stat.h>

//#define TRACE_BASECASE 1
//#define VALIDATE 1

MY_EXTERN void co_basecase(LBM_GridPtr* toggle, MAIN_SimType simType,
			   int t0, int t1,
			   int x0, int dx0, int x1, int dx1,
			   int y0, int dy0, int y1, int dy1, 
			   int z0, int dz0, int z1, int dz1);

MY_EXTERN void LBM_handleInOutFlow_Orig( LBM_Grid srcGrid );
MY_EXTERN void LBM_performStreamCollide_Orig ( LBM_Grid srcGrid, LBM_Grid dstGrid );

/*############################################################################*/

static LBM_GridPtr srcGrid, dstGrid;
static LBM_GridPtr toggleR[2];

#ifdef VALIDATE
static LBM_GridPtr srcGridV, dstGridV;
static LBM_GridPtr toggleV[2];
#endif

/*############################################################################*/

#define MIN(X, Y) (((X)<(Y))? (X) : (Y))
#define SHOW_STATS_INTERVAL 64


#ifdef VALIDATE
#if 0
void CompareGridsExact(LBM_GridPtr g1, LBM_GridPtr g2, int findAllDiffs)
{
    const MY_TYPE tolerance = 1.0e-6;
    for (int z=0; z < SIZE_Z; z++)
    {
        for (int y=0; y < SIZE_Y; y++)
        {
            //if (y != 0) continue;
            for (int x=0; x < SIZE_X; x++)
            {
                //if (x!=0) continue;                
                const int i = CALC_INDEX(x, y, z, 0);
                
                if (fabs(SRC_C (g1) -SRC_C (g2)) > tolerance)
                {
                    printf("diff=%e at (%d,%d,%d, C, src_c(g1)=%e, src_c(g2)=%e)\n", fabs(SRC_C (g1) -SRC_C (g2)),
                           x, y, z, SRC_C(g1), SRC_C(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_N (g1) -SRC_N (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, N, src_n(g1)=%e, src_n(g2)=%e)\n", x, y, z, SRC_N(g1), SRC_N(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_S (g1) -SRC_S (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, S, src_s(g1)=%e, src_s(g2)=%e)\n", x, y, z, SRC_S(g1), SRC_S(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_E (g1) -SRC_E (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, E, src_e(g1)=%e, src_e(g2)=%e)\n", x, y, z, SRC_E(g1), SRC_E(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_W (g1) -SRC_W (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, W, src_w(g1)=%e, src_w(g2)=%e)\n", x, y, z, SRC_W(g1), SRC_W(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_T (g1) -SRC_T (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, T, src_t(g1)=%e, src_t(g2)=%e)\n", x, y, z, SRC_T(g1), SRC_T(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_B (g1) -SRC_B (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, B, src_t(g1)=%e, src_t(g2)=%e)\n", x, y, z, SRC_B(g1), SRC_B(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_NE (g1) -SRC_NE (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, NE, src_ne(g1)=%e, src_ne(g2)=%e)\n", x, y, z, SRC_NE(g1), SRC_NE(g2));
                    if (!findAllDiffs) return;
                }                
                if (fabs(SRC_NW (g1) -SRC_NW (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, NW, src_nw(g1)=%e, src_nw(g2)=%e)\n", x, y, z, SRC_NW(g1), SRC_NW(g2));
                    if (!findAllDiffs) return;
                }                
                if (fabs(SRC_SE (g1) -SRC_SE (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, SE, src_se(g1)=%e, src_se(g2)=%e)\n", x, y, z, SRC_SE(g1), SRC_SE(g2));
                    if (!findAllDiffs) return;
                }                
                if (fabs(SRC_SW (g1) -SRC_SW (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, SW, src_sw(g1)=%e, src_sw(g2)=%e)\n", x, y, z, SRC_SW(g1), SRC_SW(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_NT (g1) -SRC_NT (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, NT, src_nt(g1)=%e, src_nt(g2)=%e)\n", x, y, z, SRC_NT(g1), SRC_NT(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_NB (g1) -SRC_NB (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, NB, src_nb(g1)=%e, src_nb(g2)=%e)\n", x, y, z, SRC_NB(g1), SRC_NB(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_ST (g1) -SRC_ST (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, ST, src_st(g1)=%e, src_st(g2)=%e)\n", x, y, z, SRC_ST(g1), SRC_ST(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_SB (g1) -SRC_SB (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, SB, src_sb(g1)=%e, src_sb(g2)=%e)\n", x, y, z, SRC_SB(g1), SRC_SB(g2));
                    if (!findAllDiffs) return;
                }
                if (fabs(SRC_ET (g1) -SRC_ET (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, ET, src_et(g1)=%e, src_et(g2)=%e)\n", x, y, z, SRC_ET(g1), SRC_ET(g2));
                    if (!findAllDiffs) return;
                }                
                if (fabs(SRC_EB (g1) -SRC_EB (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, EB, src_eb(g1)=%e, src_eb(g2)=%e)\n", x, y, z, SRC_EB(g1), SRC_EB(g2));
                    if (!findAllDiffs) return;
                }                
                if (fabs(SRC_WT (g1) -SRC_WT (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, WT, src_wt(g1)=%e, src_wt(g2)=%e)\n", x, y, z, SRC_WT(g1), SRC_WT(g2));
                    if (!findAllDiffs) return;
                }                
                if (fabs(SRC_WB (g1) -SRC_WB (g2)) > tolerance)
                {
                    printf("diff at (%d,%d,%d, WB, src_wb(g1)=%e, src_wb(g2)=%e)\n", x, y, z, SRC_WB(g1), SRC_WB(g2));
                    if (!findAllDiffs) return;
                }                
            }
        }
    }    
}
#endif

void Validate( LBM_Grid grid_curr, LBM_Grid grid_ref)
{
	int x, y, z;
	MY_TYPE rho, ux, uy, uz;
	OUTPUT_PRECISION refRho, refUx, refUy, refUz,
	                 dUx, dUy, dUz,
	                 diff2, maxDiff2 = -1e+30;

	for( z = 0; z < SIZE_Z; z++ ) {
		for( y = 0; y < SIZE_Y; y++ ) {
			for( x = 0; x < SIZE_X; x++ ) {
				rho = + GRID_ENTRY( grid_curr, x, y, z, C  ) + GRID_ENTRY( grid_curr, x, y, z, N  )
				      + GRID_ENTRY( grid_curr, x, y, z, S  ) + GRID_ENTRY( grid_curr, x, y, z, E  )
				      + GRID_ENTRY( grid_curr, x, y, z, W  ) + GRID_ENTRY( grid_curr, x, y, z, T  )
				      + GRID_ENTRY( grid_curr, x, y, z, B  ) + GRID_ENTRY( grid_curr, x, y, z, NE )
				      + GRID_ENTRY( grid_curr, x, y, z, NW ) + GRID_ENTRY( grid_curr, x, y, z, SE )
				      + GRID_ENTRY( grid_curr, x, y, z, SW ) + GRID_ENTRY( grid_curr, x, y, z, NT )
				      + GRID_ENTRY( grid_curr, x, y, z, NB ) + GRID_ENTRY( grid_curr, x, y, z, ST )
				      + GRID_ENTRY( grid_curr, x, y, z, SB ) + GRID_ENTRY( grid_curr, x, y, z, ET )
				      + GRID_ENTRY( grid_curr, x, y, z, EB ) + GRID_ENTRY( grid_curr, x, y, z, WT )
				      + GRID_ENTRY( grid_curr, x, y, z, WB );
				ux = + GRID_ENTRY( grid_curr, x, y, z, E  ) - GRID_ENTRY( grid_curr, x, y, z, W  ) 
				     + GRID_ENTRY( grid_curr, x, y, z, NE ) - GRID_ENTRY( grid_curr, x, y, z, NW ) 
				     + GRID_ENTRY( grid_curr, x, y, z, SE ) - GRID_ENTRY( grid_curr, x, y, z, SW ) 
				     + GRID_ENTRY( grid_curr, x, y, z, ET ) + GRID_ENTRY( grid_curr, x, y, z, EB ) 
				     - GRID_ENTRY( grid_curr, x, y, z, WT ) - GRID_ENTRY( grid_curr, x, y, z, WB );
				uy = + GRID_ENTRY( grid_curr, x, y, z, N  ) - GRID_ENTRY( grid_curr, x, y, z, S  ) 
				     + GRID_ENTRY( grid_curr, x, y, z, NE ) + GRID_ENTRY( grid_curr, x, y, z, NW ) 
				     - GRID_ENTRY( grid_curr, x, y, z, SE ) - GRID_ENTRY( grid_curr, x, y, z, SW ) 
				     + GRID_ENTRY( grid_curr, x, y, z, NT ) + GRID_ENTRY( grid_curr, x, y, z, NB ) 
				     - GRID_ENTRY( grid_curr, x, y, z, ST ) - GRID_ENTRY( grid_curr, x, y, z, SB );
				uz = + GRID_ENTRY( grid_curr, x, y, z, T  ) - GRID_ENTRY( grid_curr, x, y, z, B  ) 
				     + GRID_ENTRY( grid_curr, x, y, z, NT ) - GRID_ENTRY( grid_curr, x, y, z, NB ) 
				     + GRID_ENTRY( grid_curr, x, y, z, ST ) - GRID_ENTRY( grid_curr, x, y, z, SB ) 
				     + GRID_ENTRY( grid_curr, x, y, z, ET ) - GRID_ENTRY( grid_curr, x, y, z, EB ) 
				     + GRID_ENTRY( grid_curr, x, y, z, WT ) - GRID_ENTRY( grid_curr, x, y, z, WB );
				ux /= rho;
				uy /= rho;
				uz /= rho;


				refRho = + GRID_ENTRY( grid_ref, x, y, z, C  ) + GRID_ENTRY( grid_ref, x, y, z, N  )
				      + GRID_ENTRY( grid_ref, x, y, z, S  ) + GRID_ENTRY( grid_ref, x, y, z, E  )
				      + GRID_ENTRY( grid_ref, x, y, z, W  ) + GRID_ENTRY( grid_ref, x, y, z, T  )
				      + GRID_ENTRY( grid_ref, x, y, z, B  ) + GRID_ENTRY( grid_ref, x, y, z, NE )
				      + GRID_ENTRY( grid_ref, x, y, z, NW ) + GRID_ENTRY( grid_ref, x, y, z, SE )
				      + GRID_ENTRY( grid_ref, x, y, z, SW ) + GRID_ENTRY( grid_ref, x, y, z, NT )
				      + GRID_ENTRY( grid_ref, x, y, z, NB ) + GRID_ENTRY( grid_ref, x, y, z, ST )
				      + GRID_ENTRY( grid_ref, x, y, z, SB ) + GRID_ENTRY( grid_ref, x, y, z, ET )
				      + GRID_ENTRY( grid_ref, x, y, z, EB ) + GRID_ENTRY( grid_ref, x, y, z, WT )
				      + GRID_ENTRY( grid_ref, x, y, z, WB );
				refUx = + GRID_ENTRY( grid_ref, x, y, z, E  ) - GRID_ENTRY( grid_ref, x, y, z, W  ) 
				     + GRID_ENTRY( grid_ref, x, y, z, NE ) - GRID_ENTRY( grid_ref, x, y, z, NW ) 
				     + GRID_ENTRY( grid_ref, x, y, z, SE ) - GRID_ENTRY( grid_ref, x, y, z, SW ) 
				     + GRID_ENTRY( grid_ref, x, y, z, ET ) + GRID_ENTRY( grid_ref, x, y, z, EB ) 
				     - GRID_ENTRY( grid_ref, x, y, z, WT ) - GRID_ENTRY( grid_ref, x, y, z, WB );
				refUy = + GRID_ENTRY( grid_ref, x, y, z, N  ) - GRID_ENTRY( grid_ref, x, y, z, S  ) 
				     + GRID_ENTRY( grid_ref, x, y, z, NE ) + GRID_ENTRY( grid_ref, x, y, z, NW ) 
				     - GRID_ENTRY( grid_ref, x, y, z, SE ) - GRID_ENTRY( grid_ref, x, y, z, SW ) 
				     + GRID_ENTRY( grid_ref, x, y, z, NT ) + GRID_ENTRY( grid_ref, x, y, z, NB ) 
				     - GRID_ENTRY( grid_ref, x, y, z, ST ) - GRID_ENTRY( grid_ref, x, y, z, SB );
				refUz = + GRID_ENTRY( grid_ref, x, y, z, T  ) - GRID_ENTRY( grid_ref, x, y, z, B  ) 
				     + GRID_ENTRY( grid_ref, x, y, z, NT ) - GRID_ENTRY( grid_ref, x, y, z, NB ) 
				     + GRID_ENTRY( grid_ref, x, y, z, ST ) - GRID_ENTRY( grid_ref, x, y, z, SB ) 
				     + GRID_ENTRY( grid_ref, x, y, z, ET ) - GRID_ENTRY( grid_ref, x, y, z, EB ) 
				     + GRID_ENTRY( grid_ref, x, y, z, WT ) - GRID_ENTRY( grid_ref, x, y, z, WB );

				refUx /= refRho;
				refUy /= refRho;
				refUz /= refRho;

				dUx = ux - refUx;
				dUy = uy - refUy;
				dUz = uz - refUz;
				diff2 = dUx*dUx + dUy*dUy + dUz*dUz;
				if( diff2 > maxDiff2 ) maxDiff2 = diff2;
			}
		}
	}
#if 0
	printf( "Validate: maxDiff = %e  ==>  %s\n\n",
	        sqrt( maxDiff2 ),
	        sqrt( maxDiff2 ) > 1e-4 ? "##### ERROR #####" : "OK" );
#endif
}

#endif

#define MY_SPAWN       cilk_spawn
#define MY_SYNC        cilk_sync

#define NPIECES        2
#define dx_threshold   32 /*128*/
#define dy_threshold   2  /*3*/
#define dz_threshold   2  /*3*/
#define dt_threshold   3


void co_execute_1(LBM_GridPtr* toggle,
                int t0, int t1, 
                int x0, int dx0, int x1, int dx1,
                int y0, int dy0, int y1, int dy1, 
                int z0, int dz0, int z1, int dz1 )
{

  const int slope = 2;

  const int ds_x = slope; //1;
  const int ds_y = slope; //1;
  const int ds_z = slope; /* take the z data dependence in LBM_handleInOutFlow() into account */
    
    const int dt = t1 - t0;
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int dz = z1 - z0;

    int i;

    if (dx >= dx_threshold && dx >= dy && dx >= dz &&
        dt >= 1 && dx >= 2 * ds_x * dt * NPIECES) {
        int chunk = dx / NPIECES;

        for (i = 0; i < NPIECES - 1; ++i)
            MY_SPAWN co_execute_1(toggle,
                                t0, t1,
                                x0 + i * chunk, ds_x, x0 + (i+1) * chunk, -ds_x,
                                y0, dy0, y1, dy1,
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_1(toggle,
                            t0, t1,
                            x0 + i * chunk, ds_x, x1, -ds_x,
                            y0, dy0, y1, dy1, 
                            z0, dz0, z1, dz1);
        MY_SYNC;
        MY_SPAWN co_execute_1(toggle,
                            t0, t1, 
                            x0, dx0, x0, ds_x,
                            y0, dy0, y1, dy1, 
                            z0, dz0, z1, dz1);
        for (i = 1; i < NPIECES; ++i)
            MY_SPAWN co_execute_1(toggle,
                                t0, t1,
                                x0 + i * chunk, -ds_x, x0 + i * chunk, ds_x,
                                y0, dy0, y1, dy1, 
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_1(toggle,
                            t0, t1, 
                            x1, -ds_x, x1, dx1,
                            y0, dy0, y1, dy1, 
                            z0, dz0, z1, dz1);
    }
    else if (dy >= dy_threshold && dy >= dz && dt >= 1 && dy >= 2 * ds_y * dt * NPIECES) {
        int chunk = dy / NPIECES;

        for (i = 0; i < NPIECES - 1; ++i)
            MY_SPAWN co_execute_1(toggle,
                                t0, t1,
                                x0, dx0, x1, dx1,
                                y0 + i * chunk, ds_y, y0 + (i+1) * chunk, -ds_y, 
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_1(toggle,
                            t0, t1,
                            x0, dx0, x1, dx1,
                            y0 + i * chunk, ds_y, y1, -ds_y, 
                            z0, dz0, z1, dz1);
        MY_SYNC;
        MY_SPAWN co_execute_1(toggle,
                            t0, t1, 
                            x0, dx0, x1, dx1,
                            y0, dy0, y0, ds_y, 
                            z0, dz0, z1, dz1);
        for (i = 1; i < NPIECES; ++i)
            MY_SPAWN co_execute_1(toggle,
                                t0, t1,
                                x0, dx0, x1, dx1,
                                y0 + i * chunk, -ds_y, y0 + i * chunk, ds_y, 
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_1(toggle,
                            t0, t1, 
                            x0, dx0, x1, dx1,
                            y1, -ds_y, y1, dy1, 
                            z0, dz0, z1, dz1);
    }        
    else
        if (dz >= dz_threshold && dt >= 1 && dz >= 2 * ds_z * dt * NPIECES) {
            int chunk = dz / NPIECES;

            for (i = 0; i < NPIECES - 1; ++i)
                MY_SPAWN co_execute_1(toggle,
                                    t0, t1,
                                    x0, dx0, x1, dx1,
                                    y0, dy0, y1, dy1,
                                    z0 + i * chunk, ds_z, z0 + (i+1) * chunk, -ds_z);
            MY_SPAWN co_execute_1(toggle,
                                t0, t1,
                                x0, dx0, x1, dx1,
                                y0, dy0, y1, dy1, 
                                z0 + i * chunk, ds_z, z1, -ds_z);
            MY_SYNC;
            MY_SPAWN co_execute_1(toggle,
                                t0, t1, 
                                x0, dx0, x1, dx1,
                                y0, dy0, y1, dy1,
                                z0, dz0, z0, ds_z);
            for (i = 1; i < NPIECES; ++i)
                MY_SPAWN co_execute_1(toggle,
                                    t0, t1,
                                    x0, dx0, x1, dx1,
                                    y0, dy0, y1, dy1,
                                    z0 + i * chunk, -ds_z, z0 + i * chunk, ds_z);
            MY_SPAWN co_execute_1(toggle,
                                t0, t1, 
                                x0, dx0, x1, dx1,
                                y0, dy0, y1, dy1,
                                z1, -ds_z, z1, dz1);
        }
        else if (dt > dt_threshold) {
            int halfdt = dt / 2;
            co_execute_1(toggle,
                       t0, t0 + halfdt,
                       x0, dx0, x1, dx1,
                       y0, dy0, y1, dy1, 
                       z0, dz0, z1, dz1);
            co_execute_1(toggle,
                       t0 + halfdt, t1, 
                       x0 + dx0 * halfdt, dx0, x1 + dx1 * halfdt, dx1,
                       y0 + dy0 * halfdt, dy0, y1 + dy1 * halfdt, dy1, 
                       z0 + dz0 * halfdt, dz0, z1 + dz1 * halfdt, dz1);
        }
        else {
            co_basecase_1(toggle,
                          t0, t1, 
                          x0, dx0, x1, dx1,
                          y0, dy0, y1, dy1,
                          z0, dz0, z1, dz1);
        } 
}

void co_execute_2(LBM_GridPtr* toggle,
                  int t0, int t1, 
                  int x0, int dx0, int x1, int dx1,
                  int y0, int dy0, int y1, int dy1, 
                  int z0, int dz0, int z1, int dz1 )
{

  const int ds_x = 2; //1;
  const int ds_y = 2; //1;
  const int ds_z = 2; //1;
    
    const int dt = t1 - t0;
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int dz = z1 - z0;

    int i;

    if (dx >= dx_threshold && dx >= dy && dx >= dz &&
        dt >= 1 && dx >= 2 * ds_x * dt * NPIECES) {
        int chunk = dx / NPIECES;

        for (i = 0; i < NPIECES - 1; ++i)
            MY_SPAWN co_execute_2(toggle,
                                t0, t1,
                                x0 + i * chunk, ds_x, x0 + (i+1) * chunk, -ds_x,
                                y0, dy0, y1, dy1,
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_2(toggle,
                            t0, t1,
                            x0 + i * chunk, ds_x, x1, -ds_x,
                            y0, dy0, y1, dy1, 
                            z0, dz0, z1, dz1);
        MY_SYNC;
        MY_SPAWN co_execute_2(toggle,
                            t0, t1, 
                            x0, dx0, x0, ds_x,
                            y0, dy0, y1, dy1, 
                            z0, dz0, z1, dz1);
        for (i = 1; i < NPIECES; ++i)
            MY_SPAWN co_execute_2(toggle,
                                t0, t1,
                                x0 + i * chunk, -ds_x, x0 + i * chunk, ds_x,
                                y0, dy0, y1, dy1, 
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_2(toggle,
                            t0, t1, 
                            x1, -ds_x, x1, dx1,
                            y0, dy0, y1, dy1, 
                            z0, dz0, z1, dz1);
    }
    else if (dy >= dy_threshold && dy >= dz && dt >= 1 && dy >= 2 * ds_y * dt * NPIECES) {
        int chunk = dy / NPIECES;

        for (i = 0; i < NPIECES - 1; ++i)
            MY_SPAWN co_execute_2(toggle,
                                t0, t1,
                                x0, dx0, x1, dx1,
                                y0 + i * chunk, ds_y, y0 + (i+1) * chunk, -ds_y, 
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_2(toggle,
                            t0, t1,
                            x0, dx0, x1, dx1,
                            y0 + i * chunk, ds_y, y1, -ds_y, 
                            z0, dz0, z1, dz1);
        MY_SYNC;
        MY_SPAWN co_execute_2(toggle,
                            t0, t1, 
                            x0, dx0, x1, dx1,
                            y0, dy0, y0, ds_y, 
                            z0, dz0, z1, dz1);
        for (i = 1; i < NPIECES; ++i)
            MY_SPAWN co_execute_2(toggle,
                                t0, t1,
                                x0, dx0, x1, dx1,
                                y0 + i * chunk, -ds_y, y0 + i * chunk, ds_y, 
                                z0, dz0, z1, dz1);
        MY_SPAWN co_execute_2(toggle,
                            t0, t1, 
                            x0, dx0, x1, dx1,
                            y1, -ds_y, y1, dy1, 
                            z0, dz0, z1, dz1);
    }        
    else
        if (dz >= dz_threshold && dt >= 1 && dz >= 2 * ds_z * dt * NPIECES) {
            int chunk = dz / NPIECES;

            for (i = 0; i < NPIECES - 1; ++i)
                MY_SPAWN co_execute_2(toggle,
                                    t0, t1,
                                    x0, dx0, x1, dx1,
                                    y0, dy0, y1, dy1,
                                    z0 + i * chunk, ds_z, z0 + (i+1) * chunk, -ds_z);
            MY_SPAWN co_execute_2(toggle,
                                t0, t1,
                                x0, dx0, x1, dx1,
                                y0, dy0, y1, dy1, 
                                z0 + i * chunk, ds_z, z1, -ds_z);
            MY_SYNC;
            MY_SPAWN co_execute_2(toggle,
                                t0, t1, 
                                x0, dx0, x1, dx1,
                                y0, dy0, y1, dy1,
                                z0, dz0, z0, ds_z);
            for (i = 1; i < NPIECES; ++i)
                MY_SPAWN co_execute_2(toggle,
                                    t0, t1,
                                    x0, dx0, x1, dx1,
                                    y0, dy0, y1, dy1,
                                    z0 + i * chunk, -ds_z, z0 + i * chunk, ds_z);
            MY_SPAWN co_execute_2(toggle,
                                t0, t1, 
                                x0, dx0, x1, dx1,
                                y0, dy0, y1, dy1,
                                z1, -ds_z, z1, dz1);
        }
        else if (dt > dt_threshold) {
            int halfdt = dt / 2;
            co_execute_2(toggle,
                       t0, t0 + halfdt,
                       x0, dx0, x1, dx1,
                       y0, dy0, y1, dy1, 
                       z0, dz0, z1, dz1);
            co_execute_2(toggle,
                         t0 + halfdt, t1, 
                         x0 + dx0 * halfdt, dx0, x1 + dx1 * halfdt, dx1,
                         y0 + dy0 * halfdt, dy0, y1 + dy1 * halfdt, dy1, 
                         z0 + dz0 * halfdt, dz0, z1 + dz1 * halfdt, dz1);
        }
        else {
            co_basecase_2(toggle,
                          t0, t1, 
                          x0, dx0, x1, dx1,
                          y0, dy0, y1, dy1,
                          z0, dz0, z1, dz1);
        } 
}

static int GetNumCpuThreads()
{
    FILE* fp;
    char res[128];
    fp = popen("/bin/cat /proc/cpuinfo | grep -c '^processor'", "r");
    fread(res, 1, sizeof(res)-1, fp);
    fclose(fp);
    const int numCores = (int)strtol(res, 0, 0);
    return numCores;
}

static void InitCilk()
{
    const int numThreads = GetNumCpuThreads();
    printf("Number of CPU threads available = %d\n", numThreads);

    // If CILK_NPROC environment variable is set, use its value, otherwise, use the number of CPU cores
    char* x = getenv("CILK_NPROC");
    const int nworkers_specified = x? (int)strtol(x,0,0): numThreads;

    // Tune the number of workers used
                                                      
    printf("Set the number of workers used by Cilk to %d\n", nworkers_specified);
    // set the number of workers used in cilk
    char buf[100];
    sprintf(buf, "%d", nworkers_specified);
    const int status = __cilkrts_set_param((void*)"nworkers", (void*)buf);
    assert(status == 0);
    
    const int nworkers_actual =  __cilkrts_get_nworkers();
    printf("Actual number of workers used by Cilk = %d\n", nworkers_actual);
    assert(nworkers_specified == nworkers_actual);    
}

int main( int nArgs, char* arg[] ) {
    MAIN_Param param;
#if !defined(SPEC_CPU)
    MAIN_Time time;
#endif
    int outer_t, t;
    
    InitCilk();
    
#ifdef SOA
    printf("Use SOA version\n");
#else
    printf("Use AOS version\n");
#endif
    
    MAIN_parseCommandLine( nArgs, arg, &param );
    MAIN_printInfo( &param );


    MAIN_initialize( &param );

    // we assume that param.simType==CHANNEL so that LBM_handleInOutFlow() is called in the kernel
    if (param.simType!=CHANNEL) {
      printf("assumption of param.simType==CHANNEL fails\n");
      exit(-1);
    }
    

    //printf("InitPochoir\n");

#if !defined(SPEC_CPU)
    MAIN_startClock( &time );
#endif
    //printf("RunPochoir\n");
    RunPochoir(param.simType, *srcGrid, *dstGrid, param.nTimeSteps);

#if !defined(SPEC_CPU)
    MAIN_stopClock( &time, &param );
#endif
    
#ifdef VALIDATE
    co_basecase(toggleV, param.simType, 1, param.nTimeSteps+1,
                0, 0, SIZE_X, 0,
                0, 0, SIZE_Y, 0,
                0, 0, SIZE_Z, 0);
		    
	
    Validate(*srcGrid, *srcGridV);
        
        //CompareGridsExact(srcGrid, srcGridV, 1);
#endif
        
    MAIN_finalize( &param );

    // Comment out the following return to let XTune able to dump timing stats at the end of program execution.
    //return 0;
}

/*############################################################################*/

void MAIN_parseCommandLine( int nArgs, char* arg[], MAIN_Param* param ) {
	struct stat fileStat;
	
	if( nArgs < 5 || nArgs > 6 ) {
		printf( "syntax: lbm <time steps> <result file> <0: nil, 1: cmp, 2: str> <0: ldc, 1: channel flow> [<obstacle file>]\n" );
		exit( 1 );
	}

	param->nTimeSteps     = atoi( arg[1] );
	param->resultFilename = arg[2];
	param->action         = (MAIN_Action) atoi( arg[3] );
	param->simType        = (MAIN_SimType) atoi( arg[4] );

	if( nArgs == 6 ) {
		param->obstacleFilename = arg[5];

		if( stat( param->obstacleFilename, &fileStat ) != 0 ) {
			printf( "MAIN_parseCommandLine: cannot stat obstacle file '%s'\n",
			         param->obstacleFilename );
			exit( 1 );
		}
		if( fileStat.st_size != SIZE_X*SIZE_Y*SIZE_Z+(SIZE_Y+1)*SIZE_Z ) {
			printf( "MAIN_parseCommandLine:\n"
			        "\tsize of file '%s' is %i bytes\n"
					    "\texpected size is %i bytes\n",
			        param->obstacleFilename, (int) fileStat.st_size,
			        SIZE_X*SIZE_Y*SIZE_Z+(SIZE_Y+1)*SIZE_Z );
			exit( 1 );
		}
	}
	else param->obstacleFilename = NULL;

	if( param->action == COMPARE &&
	    stat( param->resultFilename, &fileStat ) != 0 ) {
		printf( "MAIN_parseCommandLine: cannot stat result file '%s'\n",
		         param->resultFilename );
		exit( 1 );
	}
}

/*############################################################################*/

void MAIN_printInfo( const MAIN_Param* param ) {
	const char actionString[3][32] = {"nothing", "compare", "store"};
	const char simTypeString[3][32] = {"lid-driven cavity", "channel flow"};
	printf( "MAIN_printInfo:\n"
	        "\tgrid size      : %i x %i x %i = %.2f * 10^6 Cells\n"
	        "\tnTimeSteps     : %i\n"
	        "\tresult file    : %s\n"
	        "\taction         : %s\n"
	        "\tsimulation type: %s\n"
	        "\tobstacle file  : %s\n\n",
	        SIZE_X, SIZE_Y, SIZE_Z, 1e-6*SIZE_X*SIZE_Y*SIZE_Z,
	        param->nTimeSteps, param->resultFilename, 
	        actionString[param->action], simTypeString[param->simType],
	        (param->obstacleFilename == NULL) ? "<none>" :
	                                            param->obstacleFilename );
}

/*############################################################################*/

void MAIN_initialize( const MAIN_Param* param ) {
	LBM_allocateGrid( (MY_TYPE**) &srcGrid );
	LBM_allocateGrid( (MY_TYPE**) &dstGrid );

	LBM_initializeGrid( *srcGrid );
	LBM_initializeGrid( *dstGrid );

	if( param->obstacleFilename != NULL ) {
		LBM_loadObstacleFile( *srcGrid, param->obstacleFilename );
		LBM_loadObstacleFile( *dstGrid, param->obstacleFilename );
	}

	if( param->simType == CHANNEL ) {
		LBM_initializeSpecialCellsForChannel( *srcGrid );
		LBM_initializeSpecialCellsForChannel( *dstGrid );
	}
	else {
		LBM_initializeSpecialCellsForLDC( *srcGrid );
		LBM_initializeSpecialCellsForLDC( *dstGrid );
	}

	LBM_showGridStatistics( *srcGrid );

        toggleR[0] = srcGrid;
        toggleR[1] = dstGrid;
        
#ifdef VALIDATE
	LBM_allocateGrid( (MY_TYPE**) &srcGridV );
	LBM_allocateGrid( (MY_TYPE**) &dstGridV );

	LBM_initializeGrid( *srcGridV );
	LBM_initializeGrid( *dstGridV );

	if( param->obstacleFilename != NULL ) {
		LBM_loadObstacleFile( *srcGridV, param->obstacleFilename );
		LBM_loadObstacleFile( *dstGridV, param->obstacleFilename );
	}

	if( param->simType == CHANNEL ) {
		LBM_initializeSpecialCellsForChannel( *srcGridV );
		LBM_initializeSpecialCellsForChannel( *dstGridV );
	}
	else {
		LBM_initializeSpecialCellsForLDC( *srcGridV );
		LBM_initializeSpecialCellsForLDC( *dstGridV );
	}


        toggleV[0] = srcGridV;
        toggleV[1] = dstGridV;        
#endif        
}

/*############################################################################*/

void MAIN_finalize( const MAIN_Param* param ) {
    printf("MAIN_finalize: output from co_execute()\n");
    printf("MAIN_finalize: srcGrid:\n");
    LBM_showGridStatistics( *srcGrid );
    printf("MAIN_finalize: dstGrid:\n");
    LBM_showGridStatistics( *dstGrid );

#ifdef VALIDATE
    printf("MAIN_finalize: output from validation run()\n");
    printf("MAIN_finalize: srcGridV:\n");
    LBM_showGridStatistics( *srcGridV );
    printf("MAIN_finalize: dstGridV:\n");
    LBM_showGridStatistics( *dstGridV );
#endif
        
    if( param->action == COMPARE )
        LBM_compareVelocityField( *srcGrid, param->resultFilename, TRUE );
    if( param->action == STORE )
	LBM_storeVelocityField( *srcGrid, param->resultFilename, TRUE );
    
    LBM_freeGrid( (MY_TYPE**) &srcGrid );
    LBM_freeGrid( (MY_TYPE**) &dstGrid );
}

#if !defined(SPEC_CPU)
/*############################################################################*/

void MAIN_startClock( MAIN_Time* time ) {
	time->timeScale = 1.0 / sysconf( _SC_CLK_TCK );
	time->tickStart = times( &(time->timeStart) );
}


/*############################################################################*/

void MAIN_stopClock( MAIN_Time* time, const MAIN_Param* param ) {
	time->tickStop = times( &(time->timeStop) );

	printf( "MAIN_stopClock:\n"
	        "\tusr: %7.2f sys: %7.2f tot: %7.2f wct: %7.2f MLUPS: %5.2f\n\n",
	        (time->timeStop.tms_utime - time->timeStart.tms_utime) * time->timeScale,
	        (time->timeStop.tms_stime - time->timeStart.tms_stime) * time->timeScale,
	        (time->timeStop.tms_utime - time->timeStart.tms_utime +
	         time->timeStop.tms_stime - time->timeStart.tms_stime) * time->timeScale,
	        (time->tickStop           - time->tickStart          ) * time->timeScale,
	        1.0e-6 * SIZE_X * SIZE_Y * SIZE_Z * param->nTimeSteps /
	        (time->tickStop           - time->tickStart          ) / time->timeScale );
}
#endif
