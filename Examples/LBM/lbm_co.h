/* $Id: lbm.h,v 1.1 2004/04/20 14:33:59 pohlt Exp $ */

/*############################################################################*/

#ifndef _LBM_H_
#define _LBM_H_

/*############################################################################*/

#include "config.h"

/*############################################################################*/

typedef enum {C = 0,
              N, S, E, W, T, B,
              NE, NW, SE, SW,
              NT, NB, ST, SB,
              ET, EB, WT, WB,
              FLAGS, N_CELL_ENTRIES} CELL_ENTRIES;
#define N_DISTR_FUNCS FLAGS

typedef enum {OBSTACLE    = 1 << 0,
              ACCEL       = 1 << 1,
              IN_OUT_FLOW = 1 << 2} CELL_FLAGS;

#include "lbm_1d_array.h"

/*############################################################################*/
MY_EXTERN void LBM_allocateGrid( MY_TYPE** ptr );
MY_EXTERN void LBM_freeGrid( MY_TYPE** ptr );
MY_EXTERN void LBM_initializeGrid( LBM_Grid grid );
MY_EXTERN void LBM_initializeSpecialCellsForLDC( LBM_Grid grid );
MY_EXTERN void LBM_loadObstacleFile( LBM_Grid grid, const char* filename );
MY_EXTERN void LBM_initializeSpecialCellsForChannel( LBM_Grid grid );
MY_EXTERN void LBM_swapGrids( LBM_GridPtr* grid1, LBM_GridPtr* grid2 );
MY_EXTERN void LBM_performStreamCollide( LBM_Grid srcGrid, LBM_Grid dstGrid, const int x0, const int x1, const int y0, const int y1, const int z0, const int z1);
MY_EXTERN void LBM_handleInOutFlow( LBM_Grid srcGrid, const int x0, const int x1, const int y0, const int y1, const int z0, const int z1);
MY_EXTERN void LBM_showGridStatistics( LBM_Grid Grid );
MY_EXTERN void LBM_storeVelocityField( LBM_Grid grid, const char* filename,
                           const BOOL binary );
MY_EXTERN void LBM_compareVelocityField( LBM_Grid grid, const char* filename,
                               const BOOL binary );

MY_EXTERN void co_basecase_1(LBM_GridPtr* toggle,
                             int t0, int t1,
                             int x0, int dx0, int x1, int dx1,
                             int y0, int dy0, int y1, int dy1, 
                             int z0, int dz0, int z1, int dz1);

MY_EXTERN void co_basecase_2(LBM_GridPtr* toggle,
                             int t0, int t1,
                             int x0, int dx0, int x1, int dx1,
                             int y0, int dy0, int y1, int dy1, 
                             int z0, int dz0, int z1, int dz1);

/*############################################################################*/

#endif /* _LBM_H_ */
