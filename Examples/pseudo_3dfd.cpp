/*
 * ============================================================================
 *
 *       Filename:  pseudo_3dfd.cpp
 *
 *    Description:  pseudo code for Matteo's 3dfd
 *
 *        Version:  1.0
 *        Created:  12/16/2010 11:50:16 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yuan Tang (), yuantang@csail.mit.edu
 *        Company:  CSAIL, M.I.T.
 *
 * ============================================================================
 */

void walk3(int t0, int t1, 
	   int x0, int dx0, int x1, int dx1,
	   int y0, int dy0, int y1, int dy1, 
	   int z0, int dz0, int z1, int dz1 )
{
  int dt = t1 - t0, dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
  int i;

  if (dx >= dx_threshold && dx >= dy && dx >= dz &&
      dt >= 1 && dx >= 2 * ds * dt * NPIECES) {
    int chunk = dx / NPIECES;

    for (i = 0; i < NPIECES - 1; ++i)
      cilk_spawn walk3(t0, t1,
		       x0 + i * chunk, ds, x0 + (i+1) * chunk, -ds,
		       y0, dy0, y1, dy1,
		       z0, dz0, z1, dz1);
    cilk_spawn walk3(t0, t1,
		     x0 + i * chunk, ds, x1, -ds,
		     y0, dy0, y1, dy1, 
		     z0, dz0, z1, dz1);
    cilk_sync;
    cilk_spawn walk3(t0, t1, 
		     x0, dx0, x0, ds,
		     y0, dy0, y1, dy1, 
		     z0, dz0, z1, dz1);
    for (i = 1; i < NPIECES; ++i)
      cilk_spawn walk3(t0, t1,
		       x0 + i * chunk, -ds, x0 + i * chunk, ds,
		       y0, dy0, y1, dy1, 
		       z0, dz0, z1, dz1);
    cilk_spawn walk3(t0, t1, 
		     x1, -ds, x1, dx1,
		     y0, dy0, y1, dy1, 
		     z0, dz0, z1, dz1);
  } else if (dy >= dyz_threshold && dy >= dz && dt >= 1 && dy >= 2 * ds * dt * NPIECES) {
    int chunk = dy / NPIECES;

    for (i = 0; i < NPIECES - 1; ++i)
      cilk_spawn walk3(t0, t1,
		       x0, dx0, x1, dx1,
		       y0 + i * chunk, ds, y0 + (i+1) * chunk, -ds, 
		       z0, dz0, z1, dz1);
    cilk_spawn walk3(t0, t1,
		     x0, dx0, x1, dx1,
		     y0 + i * chunk, ds, y1, -ds, 
		     z0, dz0, z1, dz1);
    cilk_sync;
    cilk_spawn walk3(t0, t1, 
		     x0, dx0, x1, dx1,
		     y0, dy0, y0, ds, 
		     z0, dz0, z1, dz1);
    for (i = 1; i < NPIECES; ++i)
      cilk_spawn walk3(t0, t1,
		       x0, dx0, x1, dx1,
		       y0 + i * chunk, -ds, y0 + i * chunk, ds, 
		       z0, dz0, z1, dz1);
    cilk_spawn walk3(t0, t1, 
		     x0, dx0, x1, dx1,
		     y1, -ds, y1, dy1, 
		     z0, dz0, z1, dz1);
  } else if (dz >= dyz_threshold && dt >= 1 && dz >= 2 * ds * dt * NPIECES) {
    int chunk = dz / NPIECES;

    for (i = 0; i < NPIECES - 1; ++i)
      cilk_spawn walk3(t0, t1,
		       x0, dx0, x1, dx1,
		       y0, dy0, y1, dy1,
		       z0 + i * chunk, ds, z0 + (i+1) * chunk, -ds);
    cilk_spawn walk3(t0, t1,
		     x0, dx0, x1, dx1,
		     y0, dy0, y1, dy1, 
		     z0 + i * chunk, ds, z1, -ds);
    cilk_sync;
    cilk_spawn walk3(t0, t1, 
		     x0, dx0, x1, dx1,
		     y0, dy0, y1, dy1,
		     z0, dz0, z0, ds);
    for (i = 1; i < NPIECES; ++i)
      cilk_spawn walk3(t0, t1,
		       x0, dx0, x1, dx1,
		       y0, dy0, y1, dy1,
		       z0 + i * chunk, -ds, z0 + i * chunk, ds);
    cilk_spawn walk3(t0, t1, 
		     x0, dx0, x1, dx1,
		     y0, dy0, y1, dy1,
		     z1, -ds, z1, dz1);
  }  else if (dt > dt_threshold) {
    int halfdt = dt / 2;
    walk3(t0, t0 + halfdt,
	  x0, dx0, x1, dx1,
	  y0, dy0, y1, dy1, 
	  z0, dz0, z1, dz1);
    walk3(t0 + halfdt, t1, 
	  x0 + dx0 * halfdt, dx0, x1 + dx1 * halfdt, dx1,
	  y0 + dy0 * halfdt, dy0, y1 + dy1 * halfdt, dy1, 
	  z0 + dz0 * halfdt, dz0, z1 + dz1 * halfdt, dz1);
  } else {
    basecase(t0, t1, 
	     x0, dx0, x1, dx1,
	     y0, dy0, y1, dy1,
	     z0, dz0, z1, dz1);
  } 
}

