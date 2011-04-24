/* $Id: lbm.h,v 1.1 2004/04/20 14:33:59 pohlt Exp $ */

/*############################################################################*/

#ifndef _LBM_H_
#define _LBM_H_

/*############################################################################*/

#include <pochoir.hpp>
#include "config.h"

/*############################################################################*/

typedef struct 
{
    MY_TYPE _C;
    MY_TYPE _N;
    MY_TYPE _S;
    MY_TYPE _E;
    MY_TYPE _W;
    MY_TYPE _T;
    MY_TYPE _B;
    MY_TYPE _NE;
    MY_TYPE _NW;
    MY_TYPE _SE;
    MY_TYPE _SW;
    MY_TYPE _NT;
    MY_TYPE _NB;
    MY_TYPE _ST;
    MY_TYPE _SB;
    MY_TYPE _ET;
    MY_TYPE _EB;
    MY_TYPE _WT;
    MY_TYPE _WB;
    MY_TYPE _FLAGS;
} PoCellEntry;

#define MARGIN_Z 2

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

#include "lbm_1d_array_tang.h"

/*############################################################################*/

void LBM_initializeGrid( Pochoir_Array_3D(PoCellEntry) & pa, const int t );
void LBM_initializeSpecialCellsForLDC( Pochoir_Array_3D(PoCellEntry) & pa, const int t );
void LBM_initializeSpecialCellsForChannel( Pochoir_Array_3D(PoCellEntry) & pa, const int t );
void LBM_loadObstacleFile( Pochoir_Array_3D(PoCellEntry) & pa, const int t, const char* filename );
void LBM_showGridStatistics( Pochoir_Array_3D(PoCellEntry) & pa, const int t );
void LBM_handleInOutFlow( Pochoir_Array_3D(PoCellEntry) & pa, const int t );
void LBM_performStreamCollide( Pochoir_Array_3D(PoCellEntry) & pa, const int t );
void LBM_storeVelocityField( Pochoir_Array_3D(PoCellEntry) & pa, const int t, 
                             const char* filename, const BOOL binary );
void LBM_compareVelocityField( Pochoir_Array_3D(PoCellEntry) & pa, const int t, 
                               const char* filename, const BOOL binary );

/*############################################################################*/

#endif /* _LBM_H_ */
