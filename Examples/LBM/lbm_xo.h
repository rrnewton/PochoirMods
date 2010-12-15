#ifndef LBM_XO_H
#define LBM_XO_H

#include "config.h"

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

#define MARGIN_Z 1

MY_EXTERN void co_basecase(LBM_GridPtr* toggle, MAIN_SimType simType,
			   int t0, int t1,
			   int x0, int dx0, int x1, int dx1,
			   int y0, int dy0, int y1, int dy1, 
			   int z0, int dz0, int z1, int dz1);

MY_EXTERN void RunPochoir(MAIN_SimType simtype, LBM_Grid srcGrid, LBM_Grid dstGrid, int numTimeSteps);
#endif 
