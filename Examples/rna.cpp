/*
 **********************************************************************************
 *  Copyright (C) 2010  Massachusetts Institute of Technology
 *  Copyright (C) 2010  Yuan Tang <yuantang@csail.mit.edu>
 * 		                Charles E. Leiserson <cel@mit.edu>
 * 	 
 *  Written by: Rezaul Alam Chowdhury <rezaul@mit.edu>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Suggestsions:                  yuantang@csail.mit.edu
 *   Bugs:                          yuantang@csail.mit.edu
 *
 *********************************************************************************
 */

/* Test bench - RNA secondary structure prediction with simple pseudoknots */
/* ( quadratic space implementation of Akutsu's algorithm from Chowdhury et al., TCBB, 2010 ) */

#include <cstdio>
#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <cmath>

#include <pochoir.hpp>

using namespace std;
#define SIMPLE 0
#define N_RANK 2

#ifndef base_pair
  #undef base_pair
#endif  

#define base_pair( a1, a2 ) ( a1 + a2 == 0 )

#define not_base_pair( a1, a2 ) ( a1 + a2 )

#ifndef MAX_SEQ_LENGTH
  #define MAX_SEQ_LENGTH  1000000
#endif

#ifndef INF
  #define INF 10000000 /*( 10 * MAX_SEQ_LENGTH )*/
#endif

typedef struct
{
   int SL, SR, SM, SMAX, SP;
} NODE;


#define P_ARRAY_STRUCT_R2_T3 Pochoir_Array< NODE, N_RANK, 3 >

#define P_ARRAY_R2_T3 Pochoir_Array< int, N_RANK, 3 >

#define min2( x, y ) ( ( y ) ^ ( ( ( x ) ^ ( y ) ) & -( ( x ) < ( y ) ) ) ) 
#define max2( x, y ) ( ( x ) ^ ( ( ( x ) ^ ( y ) ) & -( ( x ) < ( y ) ) ) ) 

#define min3( x, y ) ( ( y ) + ( ( ( x ) - ( y ) ) & ( ( ( x ) - ( y ) ) >> 31 ) ) )
#define max3( x, y ) ( ( x ) - ( ( ( x ) - ( y ) ) & ( ( ( x ) - ( y ) ) >> 31 ) ) )

enum alphabet{ A = -1, U = 1, G = -2, C = 2 };

enum err_msgs{ SEQUENCE_READ, LENGTH_READ, NO_SEQUENCE, SEQUENCE_TOO_LONG, INVALID_SEQUENCE_LENGTH, FILE_OPEN_ERROR, MEM_ALLOC_ERROR };

//#define cilk_for for

int inline maxxx( int a, int b, int c ) 
{ 
  __asm__ ( 
           "cmp     %0, %1\n\t"  
           "cmoge  %1, %0\n\t" 
           "cmp     %0, %2\n\t" 
           "cmoge  %2, %0\n\t"  
          : "+r"(a) :  
            "%r"(b), "r"(c) 
          ); 
          
  return a; 
} 


int read_next_seq( char *fn, char **sq, int *len )
{
  FILE *fp;

  if ( fn != NULL )
    {
     if ( ( fp = fopen( fn, "rt" ) ) == NULL ) return FILE_OPEN_ERROR;
    }
  else fp = stdin;

  if ( fscanf( fp, "%d", len ) != 1 ) return NO_SEQUENCE;
  
  ( *sq ) = ( char * ) malloc( ( *len + 2 ) * sizeof( char ) );
  
  if ( *sq == NULL ) return MEM_ALLOC_ERROR;

  int i = 1, c;
  
  while ( ( i <= *len ) && ( ( c = fgetc( fp ) ) != NULL ) )
    {
      ( *sq )[ i++ ] = c;
    } 
  
  ( *sq )[ i ] = 0;

  if ( fn != NULL ) fclose( fp );

  return SEQUENCE_READ;
}


int read_sequence( char *fnX, int *nx, char **X )
{
  int l;

  l = read_next_seq( fnX, X, nx );

  if ( l == FILE_OPEN_ERROR )
    {
      printf( "Error: Unable to open input file ( %s )!\n\n", fnX );
      return 0;
    }
  
  if ( l == MEM_ALLOC_ERROR )
    {
      printf( "Error: Memory allocation error!\n\n" );
      return 0;
    }
  
  if ( l == NO_SEQUENCE )
    {
      printf( "Error: Failed to read sequence!\n\n" );
      return 0;
    }

  return 1;
}



int initRandSeq( int nX, char **X )
{
    ( *X ) = ( char * ) malloc( ( nX + 2 ) * sizeof( char ) );
  
    if ( ( *X == NULL ) ) 
      {
        printf( "Error: Memory allocation error!\n\n" );      
        return 0;
      }   

    char *SYM = "AUGC";    
    int nS = 4;//sizeof( SYM ) / sizeof( SYM[ 0 ] );

    srand( time( NULL ) );

    for ( int i = 1; i <= nX; ++i )
       ( *X )[ i ] = SYM[ rand( ) % nS ];

    ( *X )[ nX + 1 ] = 0;       
    
    return 1;
}


void convertToInts( int nX, char *X )
{
  for ( int i = 1; i <= nX; ++i )
    {
      switch ( X[ i ] )
        {
          case 'A' : X[ i ] = A; break;

          case 'U' : X[ i ] = U; break;

          case 'G' : X[ i ] = G; break;

          case 'C' : X[ i ] = C; break;
        }

    }
}



void print_usage( char *prog )
{
  printf( "Usage: %s [ options ]\n\n", prog );

  printf( "Options:\n" );

  printf( "\t-s fname : input file containing the seqeunece (file format: sequence length in line 1 followed by the sequence in line 2)\n" );
  printf( "\t-r len   : generate a random sequence of length len\n\n" );

  printf( "\t-d       : run standard DP algorithm\n" );
  printf( "\t-i       : run iterative stencil\n\n" );  
   
  printf( "\t-h       : print this help screen\n\n" );
}



int read_command_line( int argc, char *argv[ ], int &nx, char **fnX, int &run_standard_dp, int &run_iter_stencil )
{
  nx = 0;
  run_standard_dp = 0;
  run_iter_stencil = 0;

  for ( int i = 1; i < argc; )
    {
     int j = i;

     if ( !strcasecmp( argv[ i ], "-d" ) )
       {
        run_standard_dp = 1;
         
        i++;
        
        if ( i >= argc ) break;
       }
     

     if ( !strcasecmp( argv[ i ], "-i" ) )
       {
        run_iter_stencil = 1;
         
        i++;
        
        if ( i >= argc ) break;
       }

     
     if ( !strcasecmp( argv[ i ], "-r" ) )
       {
        if ( ( i + 1 >= argc ) || ( argv[ i + 1 ][ 0 ] == '-' ) )
          {
           printf( "Error: Missing sequence length ( specify -r sequence-length )!\n\n" );
           return 0;
          }

        nx = atoi( argv[ i + 1 ] );

        if ( nx < 1 )
          {
           printf( "Error: Specify positive sequence length!\n\n" );
           return 0;
          }
          
        if ( nx > MAX_SEQ_LENGTH )
          {
           printf( "Error: Cannot generate sequences longer than %d!\n\n", MAX_SEQ_LENGTH );
           return 0;
          }          

        i += 2;

        if ( i >= argc ) break;
       }
       
     if ( !strcasecmp( argv[ i ], "-s" ) )
       {
        if ( ( i + 1 >= argc ) || ( argv[ i + 1 ][ 0 ] == '-' ) )
          {
           printf( "Error: Missing input file name ( specify -s sequence-filename )!\n\n" );
           return 0;
          }

        ( *fnX ) = strdup( argv[ i + 1 ] );

        i += 2;
        
        if ( i >= argc ) break;
       }
                     
     if ( !strcasecmp( argv[ i ], "-h" ) || !strcasecmp( argv[ i ], "-help" ) || !strcasecmp( argv[ i ], "--help" ) )
       {
        print_usage( argv[ 0 ] );
        exit( 0 );
        i++;

        if ( i >= argc ) break;
       }


     if ( i == j )
       {
        printf( "Error: Unknown option ( %s )!\n\n", argv[ i ] );
        return 0;
       }
    }
   
   return 1;
}


void stencilRNAi0( int nX, char *X, int i_0, 
                   P_ARRAY_STRUCT_R2_T3 &pArray )
{
    Pochoir< NODE, N_RANK, 3 > pRNA;    
    Pochoir_Shape< N_RANK > pRNA_shape[ 6 ] = { { 2, 0, 0 }, 
                                                { 1, 0, 0 }, { 1, 0, -1 }, { 1, -1, 0 },
                                                { 0, -1, 0 }, { 0, 0, -1 } };    

    cilk_for ( int k_0 = 1; k_0 <= nX; ++k_0 )
      for ( int i = i_0; i < k_0 - 1; ++i )
         pArray( 0, i, k_0 ).SP = -INF;

    for ( int t = 0; t < 2; ++t )
      cilk_for ( int i = 0; i <= nX; ++i )
        for ( int k = 0; k <= nX; ++k )      
          {
            pArray( t, i, k ).SMAX = -INF;
            
            int j = t - i - k, jj = nX - j + 1;
            
            if ( ( j >= 0 ) && ( j <= nX ) && ( i_0 - 1 <= i ) && ( i < jj ) && ( jj <= k ) )
               {    
                 bool b0 = ( jj == k ) || ( i >= i_0 ), b1 = base_pair( X[ i ], X[ jj ] );
                 
                 pArray( t, i, k ).SL = b0 ? ( b1 ? 1 : -INF ) : 0;
                                
                 b0 = ( k == jj + 1 ) || ( i != i_0 - 1 );            
                 b1 = base_pair( X[ jj ], X[ k ] );   
                 
                 pArray( t, i, k ).SR = b0 ? ( b1 ? 1 : -INF ) : 0;               
        
                 b0 = ( i == i_0 - 1 ) || ( t == 0 );
                 
                 if ( b0 ) pArray( t, i, k ).SM = 0;  
                 else
                   {
                     int v1 = -INF, v2 = -INF;
                     
                     if ( jj < k ) 
                       {
                         int sr = pArray( 0, i, k - 1 ).SR, sm = pArray( 0, i, k - 1 ).SM, smax = pArray( 0, i, k ).SMAX;
                         
                         v1 = max( sr, sm );
                         v1 = max( v1, smax );
                       } 
                     
                     if ( i - 1 < jj ) 
                       {
                         int sl = pArray( 0, i - 1, k ).SL, sm = pArray( 0, i - 1, k ).SM;
                          
                         v2 = max( sl, sm );                           
                       }  
                                                
                     pArray( t, i, k ).SM = max( v1, v2 );  
                   }
                   
                 int sl = pArray( t, i, k ).SL, sr = pArray( t, i, k ).SR, sm = pArray( t, i, k ).SM;  
                                     
                 int smax = max( sl, sr );       
                        
                 smax = max( smax, sm );       
                        
                 pArray( t, i, k ).SMAX = smax;       
                 
                 b0 = ( i_0 <= i ) && ( jj < k );
                 
                 int sp = pArray( 0, i, k ).SP;
                 
                 pArray( 0, i, k ).SP = b0 ? max( smax, sp ) : -INF;                            
               }  
          }


    Pochoir_kernel_2D( pRNA_fn, t, i, k )
      
       int j = t + 2 - i - k, jj = nX - j + 1;

       if ( ( j >= 0 ) && ( j <= nX ) && ( i < jj ) )
         {                   
           if ( ( i_0 - 1 < i ) && ( jj + 1 < k ) )
             {
	       int sr = pArray( t + 1, i, k - 1 ).SR, smr = pArray( t + 1, i, k - 1 ).SM;
	       int sl = pArray( t + 1, i - 1, k ).SL, sml = pArray( t + 1, i - 1, k ).SM;
	       int smax = pArray( t + 1, i, k ).SMAX;
	       	       	      
               int v1 = max( sr, smr );
               int v2 = max( sl, sml );

               v1 = max( v1, smax );
                                                    
               pArray( t + 2, i, k ).SM = max( v1, v2 );  

               pArray( t + 2, i, k ).SL = sl = not_base_pair( X[ i ], X[ jj ] ) ? -INF : ( 1 + pArray( t, i - 1, k ).SMAX );                       
               pArray( t + 2, i, k ).SR = sr = not_base_pair( X[ jj ], X[ k ] ) ? -INF : ( 1 + pArray( t, i, k - 1 ).SMAX );        

               v1 = max( sl, sr );       
                      
               smr = pArray( t + 2, i, k ).SM;       
                      
               pArray( t + 2, i, k ).SMAX = smax = max( v1, smr );       
               
               int sp = pArray( 0, i, k ).SP;       
                          
               pArray( 0, i, k ).SP = max( smax, sp );
             }       
           else if ( ( i_0 - 1 <= i ) && ( jj <= k ) )
                 {                   
                   int sl, sr, sm = 0;
                   
                   bool b0 = ( i_0 - 1 == i ) && ( k <= jj + 1 );
                   
                   sl = sr = b0 ? 0 : -INF;

                   b0 = base_pair( X[ i ], X[ jj ] );
                   bool b1 = ( jj == k ), b2 = ~b1 && ( ( i_0 - 1 < i ) || ( jj + 1 != k ) ) && b0;
                   b1 = b1 && b0;
                   
                   pArray( t + 2, i, k ).SL = sl = b1 ? 1 : ( b2 ? ( 1 + pArray( t, i - 1, k ).SMAX ) : sl );
                                              
                   b0 = base_pair( X[ jj ], X[ k ] );
                   b1 = ( i_0 - 1 == i );
                   b2 = ( jj + 1 <= k - 1 );                           
                   bool b3 = ( b1 && ( jj + 1 == k ) ) || ( ~b1 && ~b2 );
                   b3 = b3 && b0; 
                   b2 = b2 && b1 && b0;        
                        
                   pArray( t + 2, i, k ).SR = sr = b3 ? 1 : ( b2 ? ( 1 + pArray( t, i, k - 1 ).SMAX ) : sr );     
                                     
                   if ( i > i_0 - 1 ) 
                     {
                       int v1 = -INF, v2 = -INF;
                       
                       if ( jj < k ) 
                         {
                           int sr = pArray( t + 1, i, k - 1 ).SR, sm = pArray( t + 1, i, k - 1 ).SM, smax = pArray( t + 1, i, k ).SMAX;
                           
                           v1 = max( sr, sm );
                           v1 = max( v1, smax );
                         } 
                       
                       if ( i - 1 < jj ) 
                         {
                           int sl = pArray( t + 1, i - 1, k ).SL, sm = pArray( t + 1, i - 1, k ).SM;
                           
                           v2 = max( sl, sm );
                         }  
                       
                       sm = max( v1, v2 );  
                     }
          
                   pArray( t + 2, i, k ).SM = sm;  
                                                  
                   int v = max( sl, sr );       
                          
                   int smax = max( v, sm );;
                          
                   pArray( t + 2, i, k ).SMAX = smax;       
                              
                   b0 = ( i_0 <= i ) && ( jj < k );           
                   
                   int sp = pArray( 0, i, k ).SP;
                   
                   pArray( 0, i, k ).SP = b0 ? max( smax, sp ) : -INF;
                 }
         }
                      	      
    Pochoir_kernel_end

    pRNA.registerShape( pRNA_shape );

    pRNA.registerArray( pArray );
                    
    int t = 3 * nX - 1;

    pRNA.run( t, pRNA_fn );
}




void stencilRNAi0( int nX, char *X, int i_0, 
                   P_ARRAY_R2_T3 &SL, 
                   P_ARRAY_R2_T3 &SR, 
                   P_ARRAY_R2_T3 &SM,
                   P_ARRAY_R2_T3 &SMAX,                    
                   P_ARRAY_R2_T3 &SP )
{
    Pochoir< int, N_RANK, 3 > pRNA;    
    Pochoir_Shape< N_RANK > pRNA_shape[ 6 ] = { { 2, 0, 0 }, 
                                                { 1, 0, 0 }, { 1, 0, -1 }, { 1, -1, 0 },
                                                { 0, -1, 0 }, { 0, 0, -1 } };    

    cilk_for ( int k_0 = 1; k_0 <= nX; ++k_0 )
      for ( int i = i_0; i < k_0 - 1; ++i )
         SP( 0, i, k_0 ) = -INF;

    for ( int t = 0; t < 2; ++t )
      cilk_for ( int i = 0; i <= nX; ++i )
        for ( int k = 0; k <= nX; ++k )      
          {
            SMAX( t, i, k ) = -INF;
            
            int j = t - i - k, jj = nX - j + 1;
            
            if ( ( j >= 0 ) && ( j <= nX ) && ( i_0 - 1 <= i ) && ( i < jj ) && ( jj <= k ) )
               {    
                 bool b0 = ( jj == k ) || ( i >= i_0 ), b1 = base_pair( X[ i ], X[ jj ] );
                 
                 SL( t, i, k ) = b0 ? ( b1 ? 1 : -INF ) : 0;
                                
                 b0 = ( k == jj + 1 ) || ( i != i_0 - 1 );            
                 b1 = base_pair( X[ jj ], X[ k ] );   
                 
                 SR( t, i, k ) = b0 ? ( b1 ? 1 : -INF ) : 0;               
        
                 b0 = ( i == i_0 - 1 ) || ( t == 0 );
                 
                 if ( b0 ) SM( t, i, k ) = 0;  
                 else
                   {
                     int v1 = -INF, v2 = -INF;
                     
                     if ( jj < k ) 
                       {
                         int sr = SR( 0, i, k - 1 ), sm = SM( 0, i, k - 1 ), smax = SMAX( 0, i, k );
                         
                         v1 = max( sr, sm );
                         v1 = max( v1, smax );
                       } 
                     
                     if ( i - 1 < jj ) 
                       {
                         int sl = SL( 0, i - 1, k ), sm = SM( 0, i - 1, k );
                          
                         v2 = max( sl, sm );                           
                       }  
                                                
                     SM( t, i, k ) = max( v1, v2 );  
                   }
                   
                 int sl = SL( t, i, k ), sr = SR( t, i, k ), sm = SM( t, i, k );  
                                     
                 int smax = max( sl, sr );       
                        
                 smax = max( smax, sm );       
                        
                 SMAX( t, i, k ) = smax;       
                 
                 b0 = ( i_0 <= i ) && ( jj < k );
                 
                 int sp = SP( 0, i, k );
                 
                 SP( 0, i, k ) = b0 ? max( smax, sp ) : -INF;                            
               }  
          }


    Pochoir_kernel_2D( pRNA_fn, t, i, k )
      
       int j = t + 2 - i - k, jj = nX - j + 1;

       if ( ( j >= 0 ) && ( j <= nX ) && ( i < jj ) )
         {                   
           if ( ( i_0 - 1 < i ) && ( jj + 1 < k ) )
             {
	       int sr = SR( t + 1, i, k - 1 ), smr = SM( t + 1, i, k - 1 );
	       int sl = SL( t + 1, i - 1, k ), sml = SM( t + 1, i - 1, k );
	       int smax = SMAX( t + 1, i, k );
	       	       	      
               int v1 = max( sr, smr );
               int v2 = max( sl, sml );

               v1 = max( v1, smax );
                                                    
               SM( t + 2, i, k ) = max( v1, v2 );  

               SL( t + 2, i, k ) = sl = not_base_pair( X[ i ], X[ jj ] ) ? -INF : ( 1 + SMAX( t, i - 1, k ) );                       
               SR( t + 2, i, k ) = sr = not_base_pair( X[ jj ], X[ k ] ) ? -INF : ( 1 + SMAX( t, i, k - 1 ) );        

               v1 = max( sl, sr );       
                      
               smr = SM( t + 2, i, k );       
                      
               SMAX( t + 2, i, k ) = smax = max( v1, smr );       
               
               int sp = SP( 0, i, k );       
                          
               SP( 0, i, k ) = max( smax, sp );
             }       
           else if ( ( i_0 - 1 <= i ) && ( jj <= k ) )
                 {                   
                   int sl, sr, sm = 0;
                   
                   bool b0 = ( i_0 - 1 == i ) && ( k <= jj + 1 );
                   
                   sl = sr = b0 ? 0 : -INF;

                   b0 = base_pair( X[ i ], X[ jj ] );
                   bool b1 = ( jj == k ), b2 = ~b1 && ( ( i_0 - 1 < i ) || ( jj + 1 != k ) ) && b0;
                   b1 = b1 && b0;
                   
                   SL( t + 2, i, k ) = sl = b1 ? 1 : ( b2 ? ( 1 + SMAX( t, i - 1, k ) ) : sl );
                                              
                   b0 = base_pair( X[ jj ], X[ k ] );
                   b1 = ( i_0 - 1 == i );
                   b2 = ( jj + 1 <= k - 1 );                           
                   bool b3 = ( b1 && ( jj + 1 == k ) ) || ( ~b1 && ~b2 );
                   b3 = b3 && b0; 
                   b2 = b2 && b1 && b0;        
                        
                   SR( t + 2, i, k ) = sr = b3 ? 1 : ( b2 ? ( 1 + SMAX( t, i, k - 1 ) ) : sr );     
                                     
                   if ( i > i_0 - 1 ) 
                     {
                       int v1 = -INF, v2 = -INF;
                       
                       if ( jj < k ) 
                         {
                           int sr = SR( t + 1, i, k - 1 ), sm = SM( t + 1, i, k - 1 ), smax = SMAX( t + 1, i, k );
                           
                           v1 = max( sr, sm );
                           v1 = max( v1, smax );
                         } 
                       
                       if ( i - 1 < jj ) 
                         {
                           int sl = SL( t + 1, i - 1, k ), sm = SM( t + 1, i - 1, k );
                           
                           v2 = max( sl, sm );
                         }  
                       
                       sm = max( v1, v2 );  
                     }
          
                   SM( t + 2, i, k ) = sm;  
                                                  
                   int v = max( sl, sr );       
                          
                   int smax = max( v, sm );;
                          
                   SMAX( t + 2, i, k ) = smax;       
                              
                   b0 = ( i_0 <= i ) && ( jj < k );           
                   
                   int sp = SP( 0, i, k );
                   
                   SP( 0, i, k ) = b0 ? max( smax, sp ) : -INF;
                 }
         }
                      	      
    Pochoir_kernel_end

    pRNA.registerShape( pRNA_shape );

    pRNA.registerArray( SL );
    pRNA.registerArray( SR );
    pRNA.registerArray( SM );
    pRNA.registerArray( SMAX );
    pRNA.registerArray( SP );
                    
    int t = 3 * nX - 1;

    pRNA.run( t, pRNA_fn );
}




void iterativeStencilRNAi0( int nX, char *X, int i_0, 
			    P_ARRAY_STRUCT_R2_T3 &pArray )
{
    cilk_for ( int k_0 = 1; k_0 <= nX; ++k_0 )
      for ( int i = i_0; i < k_0 - 1; ++i )
         pArray.interior( 0, i, k_0 ).SP = -INF;

    for ( int t = 0; t < 2; ++t )
      cilk_for ( int i = 0; i <= nX; ++i )
        for ( int k = 0; k <= nX; ++k )      
          {
            pArray.interior( t, i, k ).SMAX = -INF;
            
            int j = t - i - k, jj = nX - j + 1;
            
            if ( ( j >= 0 ) && ( j <= nX ) && ( i_0 - 1 <= i ) && ( i < jj ) && ( jj <= k ) )
               {    
                 bool b0 = ( jj == k ) || ( i >= i_0 ), b1 = base_pair( X[ i ], X[ jj ] );
                 
                 pArray.interior( t, i, k ).SL = b0 ? ( b1 ? 1 : -INF ) : 0;
                                
                 b0 = ( k == jj + 1 ) || ( i != i_0 - 1 );            
                 b1 = base_pair( X[ jj ], X[ k ] );   
                 
                 pArray.interior( t, i, k ).SR = b0 ? ( b1 ? 1 : -INF ) : 0;               
        
                 b0 = ( i == i_0 - 1 ) || ( t == 0 );
                 
                 if ( b0 ) pArray.interior( t, i, k ).SM = 0;  
                 else
                   {
                     int v1 = -INF, v2 = -INF;
                     
                     if ( jj < k ) 
                       {
                         int sr = pArray.interior( 0, i, k - 1 ).SR, sm = pArray.interior( 0, i, k - 1 ).SM, smax = pArray.interior( 0, i, k ).SMAX;
                         
                         v1 = max( sr, sm );
                         v1 = max( v1, smax );
                       } 
                     
                     if ( i - 1 < jj ) 
                       {
                         int sl = pArray.interior( 0, i - 1, k ).SL, sm = pArray.interior( 0, i - 1, k ).SM;
                          
                         v2 = max( sl, sm );                           
                       }  
                                                
                     pArray.interior( t, i, k ).SM = max( v1, v2 );  
                   }
                   
                 int sl = pArray.interior( t, i, k ).SL, sr = pArray.interior( t, i, k ).SR, sm = pArray.interior( t, i, k ).SM;  
                                     
                 int smax = max( sl, sr );       
                        
                 smax = max( smax, sm );       
                        
                 pArray.interior( t, i, k ).SMAX = smax;       
                 
                 b0 = ( i_0 <= i ) && ( jj < k );
                 
                 int sp = pArray.interior( 0, i, k ).SP;
                 
                 pArray.interior( 0, i, k ).SP = b0 ? max( smax, sp ) : -INF;                            
               }  
          }

    
    for ( int t = 0; t <= 3 * nX - 2; ++t )
      cilk_for ( int i = 0; i < nX + 1; ++i )
        for ( int k = 0; k < nX + 1; ++k )      
          {
             int j = t + 2 - i - k, jj = nX - j + 1;
      
             if ( ( j >= 0 ) && ( j <= nX ) && ( i < jj ) )
               {                   
                 if ( ( i_0 - 1 < i ) && ( jj + 1 < k ) )
                   {
        	     int sr = pArray.interior( t + 1, i, k - 1 ).SR, smr = pArray.interior( t + 1, i, k - 1 ).SM;
        	     int sl = pArray.interior( t + 1, i - 1, k ).SL, sml = pArray.interior( t + 1, i - 1, k ).SM;
        	     int smax = pArray.interior( t + 1, i, k ).SMAX;
      	       	       	      
                     int v1 = max( sr, smr );
                     int v2 = max( sl, sml );
      
                     v1 = max( v1, smax );
                                                           
                     pArray.interior( t + 2, i, k ).SM = max( v1, v2 );  
                     
                     pArray.interior( t + 2, i, k ).SL = sl = not_base_pair( X[ i ], X[ jj ] ) ? -INF : ( 1 + pArray( t, i - 1, k ).SMAX );                       
                     pArray.interior( t + 2, i, k ).SR = sr = not_base_pair( X[ jj ], X[ k ] ) ? -INF : ( 1 + pArray( t, i, k - 1 ).SMAX );        
                                                    
                     int v = max( sl, sr );       
                            
                     smr = pArray.interior( t + 2, i, k ).SM;       
                            
                     pArray.interior( t + 2, i, k ).SMAX = smax = max( v, smr );       
                     
                     int sp = pArray.interior( 0, i, k ).SP;       
                                
                     pArray.interior( 0, i, k ).SP = max( smax, sp );
                   }       
                 else if ( ( i_0 - 1 <= i ) && ( jj <= k ) )
                       {                   
                         int sl, sr, sm = 0;
                         
                         bool b0 = ( i_0 - 1 == i ) && ( k <= jj + 1 );
                         
                         sl = sr = b0 ? 0 : -INF;
      
                         b0 = base_pair( X[ i ], X[ jj ] );
                         bool b1 = ( jj == k ), b2 = ~b1 && ( ( i_0 - 1 < i ) || ( jj + 1 != k ) ) && b0;
                         b1 = b1 && b0;
                         
                         pArray.interior( t + 2, i, k ).SL = sl = b1 ? 1 : ( b2 ? ( 1 + pArray.interior( t, i - 1, k ).SMAX ) : sl );
                                                    
                         b0 = base_pair( X[ jj ], X[ k ] );
                         b1 = ( i_0 - 1 == i );
                         b2 = ( jj + 1 <= k - 1 );                           
                         bool b3 = ( b1 && ( jj + 1 == k ) ) || ( ~b1 && ~b2 );
                         b3 = b3 && b0; 
                         b2 = b2 && b1 && b0;        
                              
                         pArray.interior( t + 2, i, k ).SR = sr = b3 ? 1 : ( b2 ? ( 1 + pArray.interior( t, i, k - 1 ).SMAX ) : sr );     
                                           
                         if ( i > i_0 - 1 ) 
                           {
                             int v1 = -INF, v2 = -INF;
                             
                             if ( jj < k ) 
                               {
                                 int sr = pArray.interior( t + 1, i, k - 1 ).SR, sm = pArray.interior( t + 1, i, k - 1 ).SM, smax = pArray.interior( t + 1, i, k ).SMAX;
                                 
                                 v1 = max( sr, sm );
                                 v1 = max( v1, smax );
                               } 
                             
                             if ( i - 1 < jj ) 
                               {
                                 int sl = pArray.interior( t + 1, i - 1, k ).SL, sm = pArray.interior( t + 1, i - 1, k ).SM;
                                 
                                 v2 = max( sl, sm );
                               }  
                             
                             sm = max( v1, v2 );  
                           }
                
                         pArray.interior( t + 2, i, k ).SM = sm;  
                                                        
                         int v = max( sl, sr );       
                                
                         int smax = max( v, sm );;
                                
                         pArray.interior( t + 2, i, k ).SMAX = smax;       
                                    
                         b0 = ( i_0 <= i ) && ( jj < k );           
                         
                         int sp = pArray.interior( 0, i, k ).SP;
                         
                         pArray.interior( 0, i, k ).SP = b0 ? max( smax, sp ) : -INF;
                       }
               }
          }
}




void iterativeStencilRNAi0( int nX, char *X, int i_0, 
                   P_ARRAY_R2_T3 &SL, 
                   P_ARRAY_R2_T3 &SR, 
                   P_ARRAY_R2_T3 &SM, 
                   P_ARRAY_R2_T3 &SMAX,                    
                   P_ARRAY_R2_T3 &SP )
{
    cilk_for ( int k_0 = 1; k_0 <= nX; ++k_0 )
      for ( int i = i_0; i < k_0 - 1; ++i )
         SP.interior( 0, i, k_0 ) = -INF;

    for ( int t = 0; t < 2; ++t )
      cilk_for ( int i = 0; i <= nX; ++i )
        for ( int k = 0; k <= nX; ++k )      
          {
            SMAX.interior( t, i, k ) = -INF;
            
            int j = t - i - k, jj = nX - j + 1;
            
            if ( ( j >= 0 ) && ( j <= nX ) && ( i_0 - 1 <= i ) && ( i < jj ) && ( jj <= k ) )
               {    
                 bool b0 = ( jj == k ) || ( i >= i_0 ), b1 = base_pair( X[ i ], X[ jj ] );
                 
                 SL( t, i, k ) = b0 ? ( b1 ? 1 : -INF ) : 0;
                                
                 b0 = ( k == jj + 1 ) || ( i != i_0 - 1 );            
                 b1 = base_pair( X[ jj ], X[ k ] );   
                 
                 SR( t, i, k ) = b0 ? ( b1 ? 1 : -INF ) : 0;               
        
                 b0 = ( i == i_0 - 1 ) || ( t == 0 );
                 
                 if ( b0 ) SM.interior( t, i, k ) = 0;  
                 else
                   {
                     int v1 = -INF, v2 = -INF;
                     
                     if ( jj < k ) 
                       {
                         int sr = SR.interior( 0, i, k - 1 ), sm = SM.interior( 0, i, k - 1 ), smax = SMAX.interior( 0, i, k );
                         
                         v1 = max( sr, sm );
                         v1 = max( v1, smax );
                       } 
                     
                     if ( i - 1 < jj ) 
                       {
                         int sl = SL( 0, i - 1, k ), sm = SM( 0, i - 1, k );
                          
                         v2 = max( sl, sm );                           
                       }  
                                                
                     SM.interior( t, i, k ) = max( v1, v2 );  
                   }
                   
                 int sl = SL.interior( t, i, k ), sr = SR.interior( t, i, k ), sm = SM.interior( t, i, k );  
                                     
                 int smax = max( sl, sr );       
                        
                 smax = max( smax, sm );       
                        
                 SMAX.interior( t, i, k ) = smax;       
                 
                 b0 = ( i_0 <= i ) && ( jj < k );
                 
                 int sp = SP.interior( 0, i, k );
                 
                 SP.interior( 0, i, k ) = b0 ? max( smax, sp ) : -INF;                            
               }  
          }

    
    for ( int t = 0; t <= 3 * nX - 2; ++t )
      cilk_for ( int i = 0; i < nX + 1; ++i )
        for ( int k = 0; k < nX + 1; ++k )      
          {
             int j = t + 2 - i - k, jj = nX - j + 1;
      
             if ( ( j >= 0 ) && ( j <= nX ) && ( i < jj ) )
               {                   
                 if ( ( i_0 - 1 < i ) && ( jj + 1 < k ) )
                   {
        	     int sr = SR.interior( t + 1, i, k - 1 ), smr = SM.interior( t + 1, i, k - 1 );
        	     int sl = SL.interior( t + 1, i - 1, k ), sml = SM.interior( t + 1, i - 1, k );
        	     int smax = SMAX.interior( t + 1, i, k );
      	       	       	      
                     int v1 = max( sr, smr );
                     int v2 = max( sl, sml );
      
                     v1 = max( v1, smax );
                                                           
                     SM.interior( t + 2, i, k ) = max( v1, v2 );  
                     
                     SL.interior( t + 2, i, k ) = sl = not_base_pair( X[ i ], X[ jj ] ) ? -INF : ( 1 + SMAX( t, i - 1, k ) );                       
                     SR.interior( t + 2, i, k ) = sr = not_base_pair( X[ jj ], X[ k ] ) ? -INF : ( 1 + SMAX( t, i, k - 1 ) );        
                                                    
                     int v = max( sl, sr );       
                            
                     smr = SM.interior( t + 2, i, k );       
                            
                     SMAX.interior( t + 2, i, k ) = smax = max( v, smr );       
                     
                     int sp = SP.interior( 0, i, k );       
                                
                     SP.interior( 0, i, k ) = max( smax, sp );
                   }       
                 else if ( ( i_0 - 1 <= i ) && ( jj <= k ) )
                       {                   
                         int sl, sr, sm = 0;
                         
                         bool b0 = ( i_0 - 1 == i ) && ( k <= jj + 1 );
                         
                         sl = sr = b0 ? 0 : -INF;
      
                         b0 = base_pair( X[ i ], X[ jj ] );
                         bool b1 = ( jj == k ), b2 = ~b1 && ( ( i_0 - 1 < i ) || ( jj + 1 != k ) ) && b0;
                         b1 = b1 && b0;
                         
                         SL.interior( t + 2, i, k ) = sl = b1 ? 1 : ( b2 ? ( 1 + SMAX( t, i - 1, k ) ) : sl );
                                                    
                         b0 = base_pair( X[ jj ], X[ k ] );
                         b1 = ( i_0 - 1 == i );
                         b2 = ( jj + 1 <= k - 1 );                           
                         bool b3 = ( b1 && ( jj + 1 == k ) ) || ( ~b1 && ~b2 );
                         b3 = b3 && b0; 
                         b2 = b2 && b1 && b0;        
                              
                         SR.interior( t + 2, i, k ) = sr = b3 ? 1 : ( b2 ? ( 1 + SMAX( t, i, k - 1 ) ) : sr );     
                                           
                         if ( i > i_0 - 1 ) 
                           {
                             int v1 = -INF, v2 = -INF;
                             
                             if ( jj < k ) 
                               {
                                 int sr = SR.interior( t + 1, i, k - 1 ), sm = SM.interior( t + 1, i, k - 1 ), smax = SMAX.interior( t + 1, i, k );
                                 
                                 v1 = max( sr, sm );
                                 v1 = max( v1, smax );
                               } 
                             
                             if ( i - 1 < jj ) 
                               {
                                 int sl = SL.interior( t + 1, i - 1, k ), sm = SM.interior( t + 1, i - 1, k );
                                 
                                 v2 = max( sl, sm );
                               }  
                             
                             sm = max( v1, v2 );  
                           }
                
                         SM.interior( t + 2, i, k ) = sm;  
                                                        
                         int v = max( sl, sr );       
                                
                         int smax = max( v, sm );;
                                
                         SMAX.interior( t + 2, i, k ) = smax;       
                                    
                         b0 = ( i_0 <= i ) && ( jj < k );           
                         
                         int sp = SP.interior( 0, i, k );
                         
                         SP.interior( 0, i, k ) = b0 ? max( smax, sp ) : -INF;
                       }
               }
          }
}



int stencilRNA( int nX, char *X, bool recursive, bool useStruct )
{
    int S[ nX + 2 ][ nX + 2 ];

    for ( int i = 1; i < nX + 2; i++ )
      for ( int j = 0; j <= i; j++ )
	 S[ i ][ j ] = 0;

    if ( useStruct )
      {
        P_ARRAY_STRUCT_R2_T3 pArray( nX + 1, nX + 1 );
     
        for ( int i_0 = 1; i_0 <= nX; ++i_0 )
          {
            if ( recursive ) stencilRNAi0( nX, X, i_0, pArray );
            else iterativeStencilRNAi0( nX, X, i_0, pArray );
      
            for ( int k_0 = 1, v = -INF; k_0 <= nX; ++k_0 )
              {
                v = max( pArray( 0, i_0, k_0 ).SP, v );
            
                for ( int i = i_0 + 1; i < k_0 - 1; ++i )
                   v = max( pArray( 0, i, k_0 ).SP, v );
            
                pArray( 0, i_0, k_0 ).SP = v; 
              } 
            
            for ( int k_0 = 0; k_0 < i_0 + 2; ++k_0 )
               pArray( 1, i_0, k_0 ).SP = -INF;
                    
            for ( int k_0 = i_0 + 2; k_0 <= nX; ++k_0 )
               pArray( 1, i_0, k_0 ).SP = max( pArray( 1, i_0, k_0 - 1 ).SP, pArray( 0, i_0, k_0 ).SP );            
          }
          
//        for ( int j = 0; j <= nX + 1; ++j )
//           S[ nX + 1 ][ j ] = 0;
//      
//        for ( int i = 0; i <= nX + 1; ++i )
//           S[ i ][ 0 ] = 0;
      
        for ( int i = nX; i >= 1; --i )
          for ( int j = i + 1; j <= nX; ++j )
            {
              int v;
              
              if ( base_pair( X[ i ], X[ j ] ) ) v = 1 + S[ i + 1 ][ j - 1 ];
              else v = -INF;
              
              v = max( v, NODE( pArray( 1, i, j ) ).SP );
              
              for ( int k = i + 1; k <= j; k++ )
                v = max( v, S[ i ][ k - 1 ] + S[ k ][ j ] );
                
              S[ i ][ j ] = v; 
            }                      
      }
    else
      {
        P_ARRAY_R2_T3 SL( nX + 1, nX + 1 ), SR( nX + 1, nX + 1 ), SM( nX + 1, nX + 1 ), SMAX( nX + 1, nX + 1 );
        P_ARRAY_R2_T3 SP( nX + 1, nX + 1 );
      
        for ( int i_0 = 1; i_0 <= nX; ++i_0 )
          {
            if ( recursive ) stencilRNAi0( nX, X, i_0, SL, SR, SM, SMAX, SP );
            else iterativeStencilRNAi0( nX, X, i_0, SL, SR, SM, SMAX, SP );
      
            for ( int k_0 = 1, v = -INF; k_0 <= nX; ++k_0 )
              {
                v = max( SP( 0, i_0, k_0 ), v );
            
                for ( int i = i_0 + 1; i < k_0 - 1; ++i )
                   v = max( SP( 0, i, k_0 ), v );
            
                SP( 0, i_0, k_0 ) = v; 
              } 
            
            for ( int k_0 = 0; k_0 < i_0 + 2; ++k_0 )
               SP( 1, i_0, k_0 ) = -INF;
                    
            for ( int k_0 = i_0 + 2; k_0 <= nX; ++k_0 )
               SP( 1, i_0, k_0 ) = max( SP( 1, i_0, k_0 - 1 ), SP( 0, i_0, k_0 ) );            
          }

//        for ( int j = 0; j <= nX + 1; ++j )
//           S[ nX + 1 ][ j ] = 0;
//      
//        for ( int i = 0; i <= nX + 1; ++i )
//           S[ i ][ 0 ] = 0;
      
        for ( int i = nX; i >= 1; --i )
          for ( int j = i + 1; j <= nX; ++j )
            {
              int v;
              
              if ( base_pair( X[ i ], X[ j ] ) ) v = 1 + S[ i + 1 ][ j - 1 ];
              else v = -INF;
              
              v = max( v, SP( 1, i, j ) );
              
              for ( int k = i + 1; k <= j; k++ )
                v = max( v, S[ i ][ k - 1 ] + S[ k ][ j ] );
                
              S[ i ][ j ] = v; 
            }                            
      }  
      

    return S[ 1 ][ nX ];
}



int main( int argc, char *argv[ ] )
{
    printf( "\nStencil-based DP for RNA secondary structure prediction with simple pseudoknots ( run with option -h for help ).\n\n" );

    int nX;
    char *X = NULL, *RNA = NULL;    
    char *fnX = NULL;
    int runStandardDP, runIterativeStencil;

    if ( !read_command_line( argc, argv, nX, &fnX, runStandardDP, runIterativeStencil ) )
      {
        print_usage( argv[ 0 ] );
        return 1;
      }

    int gotSeq = 0;

    if ( nX < 1 )
      {
       if ( fnX == NULL )
         {
           printf( "Error: Missing input file name ( specify -s sequence-filename )!\n\n" );
           print_usage( argv[ 0 ] );
           return 1;         
         }
          
       gotSeq = read_sequence( fnX, &nX, &RNA );
      }
    else gotSeq = initRandSeq( nX, &RNA );
        
    RNA[ 0 ] = ' ';
    X = strdup( ( const char * ) RNA );
    convertToInts( nX, X );

    if ( gotSeq )
      {
        printf( "Sequence length = %d\n\n", nX );
      
        struct timeval start, end;

        printf( "Running pochoir-based DP ( with struct )..." );
        fflush( stdout );
               
        gettimeofday( &start, 0 );        
        int maxNumBP = stencilRNA( nX, X, true, true );    
        gettimeofday( &end, 0 );

        double t0 = tdiff( &end, &start );
              
        printf( "\n\nPochoir ( with struct ):\n" );
        if ( maxNumBP == -INF ) printf( "\t maximum number of base pairs = -inf\n" );    
        else printf( "\t maximum number of base pairs = %d\n", maxNumBP );    
        printf( "\t running time = %.3lf sec\n\n", t0 );    


        printf( "Running pochoir-based DP ( without struct )..." );
        fflush( stdout );
               
        gettimeofday( &start, 0 );        
        int maxNumBP2 = stencilRNA( nX, X, true, false );    
        gettimeofday( &end, 0 );

        double t1 = tdiff( &end, &start );
              
        printf( "\n\nPochoir ( without struct ):\n" );
        if ( maxNumBP2 == -INF ) printf( "\t maximum number of base pairs = -inf\n" );    
        else printf( "\t maximum number of base pairs = %d\n", maxNumBP2 );    
        if ( t0 > 0 ) printf( "\t running time = %.3lf sec ( %.3lf x Pochoir-Struct )\n\n", t1, t1 / t0 );    
        else printf( "\t running time = %.3lf sec\n\n", t1 );

      
        if ( runIterativeStencil )
          {
            printf( "Running iterative stencil ( with struct )..." );
            fflush( stdout );
                          
            gettimeofday( &start, 0 );        
            int maxNumBPITST = stencilRNA( nX, X, false, true );
            gettimeofday( &end, 0 );

            double t2 = tdiff( &end, &start );
          
            printf( "\n\nIterative Stencil ( with struct ):\n" );
            if ( maxNumBPITST == -INF ) printf( "\t maximum number of base pairs = -inf\n" );    
            else printf( "\t maximum number of base pairs = %d\n", maxNumBPITST );                
            if ( t0 > 0 ) printf( "\t running time = %.3lf sec ( %.3lf x Pochoir-Struct )\n\n", t2, t2 / t0 );    
            else printf( "\t running time = %.3lf sec\n\n", t2 );
            
            
            printf( "Running iterative stencil ( without struct )..." );
            fflush( stdout );
                          
            gettimeofday( &start, 0 );        
            int maxNumBPITST2 = stencilRNA( nX, X, false, false );
            gettimeofday( &end, 0 );

            double t3 = tdiff( &end, &start );
          
            printf( "\n\nIterative Stencil ( without struct ):\n" );
            if ( maxNumBPITST2 == -INF ) printf( "\t maximum number of base pairs = -inf\n" );    
            else printf( "\t maximum number of base pairs = %d\n", maxNumBPITST2 );                
            if ( t0 > 0 ) printf( "\t running time = %.3lf sec ( %.3lf x Pochoir-Struct )\n\n", t3, t3 / t0 );    
            else printf( "\t running time = %.3lf sec\n\n", t3 );                
          }
      }

    if ( RNA != NULL ) free( RNA );
    if ( X != NULL ) free( X );
       
    return 0;
}
