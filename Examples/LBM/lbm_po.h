#ifndef LBM_PO_H
#define LBM_PO_H

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

#define MARGIN_Z 2

typedef PoCellEntry POGRID[(2*MARGIN_Z+SIZE_Z) * SIZE_Y * SIZE_X];

#define PO_INDEX(X, Y, Z)  (((Z)+MARGIN_Z)*SIZE_Y*SIZE_X + (Y)*SIZE_X + (X))

#define PO_SRC_C(g)      ((g)[PO_INDEX(x, y, z)]._C)
#define PO_SRC_N(g)      ((g)[PO_INDEX(x, y, z)]._N)
#define PO_SRC_S(g)      ((g)[PO_INDEX(x, y, z)]._S)
#define PO_SRC_E(g)      ((g)[PO_INDEX(x, y, z)]._E)
#define PO_SRC_W(g)      ((g)[PO_INDEX(x, y, z)]._W)
#define PO_SRC_T(g)      ((g)[PO_INDEX(x, y, z)]._T)
#define PO_SRC_B(g)      ((g)[PO_INDEX(x, y, z)]._B)
#define PO_SRC_NE(g)     ((g)[PO_INDEX(x, y, z)]._NE)
#define PO_SRC_NW(g)     ((g)[PO_INDEX(x, y, z)]._NW)
#define PO_SRC_SE(g)     ((g)[PO_INDEX(x, y, z)]._SE)
#define PO_SRC_SW(g)     ((g)[PO_INDEX(x, y, z)]._SW)
#define PO_SRC_NT(g)     ((g)[PO_INDEX(x, y, z)]._NT)
#define PO_SRC_NB(g)     ((g)[PO_INDEX(x, y, z)]._NB)
#define PO_SRC_ST(g)     ((g)[PO_INDEX(x, y, z)]._ST)
#define PO_SRC_SB(g)     ((g)[PO_INDEX(x, y, z)]._SB)
#define PO_SRC_ET(g)     ((g)[PO_INDEX(x, y, z)]._ET)
#define PO_SRC_EB(g)     ((g)[PO_INDEX(x, y, z)]._EB)
#define PO_SRC_WT(g)     ((g)[PO_INDEX(x, y, z)]._WT)
#define PO_SRC_WB(g)     ((g)[PO_INDEX(x, y, z)]._WB)
#define PO_SRC_FLAGS(g)  ((g)[PO_INDEX(x, y, z)]._FLAGS)


#define PO_DST_C(g)      ((g)[PO_INDEX(x, y, z)]._C)
#define PO_DST_N(g)      ((g)[PO_INDEX(x, y+1, z)]._N)
#define PO_DST_S(g)      ((g)[PO_INDEX(x, y-1, z)]._S)
#define PO_DST_E(g)      ((g)[PO_INDEX(x+1, y, z)]._E)
#define PO_DST_W(g)      ((g)[PO_INDEX(x-1, y, z)]._W)
#define PO_DST_T(g)      ((g)[PO_INDEX(x, y, z+1)]._T)
#define PO_DST_B(g)      ((g)[PO_INDEX(x, y, z-1)]._B)
#define PO_DST_NE(g)     ((g)[PO_INDEX(x+1, y+1, z)]._NE)
#define PO_DST_NW(g)     ((g)[PO_INDEX(x-1, y+1, z)]._NW)
#define PO_DST_SE(g)     ((g)[PO_INDEX(x+1, y-1, z)]._SE)
#define PO_DST_SW(g)     ((g)[PO_INDEX(x-1, y-1, z)]._SW)
#define PO_DST_NT(g)     ((g)[PO_INDEX(x, y+1, z+1)]._NT)
#define PO_DST_NB(g)     ((g)[PO_INDEX(x, y+1, z-1)]._NB)
#define PO_DST_ST(g)     ((g)[PO_INDEX(x, y-1, z+1)]._ST)
#define PO_DST_SB(g)     ((g)[PO_INDEX(x, y-1, z-1)]._SB)
#define PO_DST_ET(g)     ((g)[PO_INDEX(x+1, y, z+1)]._ET)
#define PO_DST_EB(g)     ((g)[PO_INDEX(x+1, y, z-1)]._EB)
#define PO_DST_WT(g)     ((g)[PO_INDEX(x-1, y, z+1)]._WT)
#define PO_DST_WB(g)     ((g)[PO_INDEX(x-1, y, z-1)]._WB)
#define PO_DST_FLAGS(g)  ((g)[PO_INDEX(x, y, z)]._FLAGS)

#define PO_TEST_FLAG_SWEEP(g, f) ((*MAGIC_CAST(PO_SRC_FLAGS(g))) & (f))

MY_EXTERN void co_basecase(LBM_GridPtr* toggle, MAIN_SimType simType,
			   int t0, int t1,
			   int x0, int dx0, int x1, int dx1,
			   int y0, int dy0, int y1, int dy1, 
			   int z0, int dz0, int z1, int dz1);


MY_EXTERN void PO_handleInOutFlow(PoCellEntry* poSrcGrid);
MY_EXTERN void PO_performStreamCollide(PoCellEntry* poSrcGrid, PoCellEntry* poDstGrid);

#endif 
