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

#ifndef INT
  #define INT unsigned char
#endif

#ifndef base_pair
  #undef base_pair
#endif  

//#define base_pair( a1, a2 ) ( a1 + a2 == 0 )
//
//#define not_base_pair( a1, a2 ) ( a1 + a2 )

#define base_pair( a1, a2 ) ( ( a1 & a2 ) == 0 )

#define not_base_pair( a1, a2 ) ( a1 & a2 )


#ifndef MAX_SEQ_LENGTH
  #define MAX_SEQ_LENGTH  1000000
#endif

#ifndef INF
  #define INF 10000000 /*( 10 * MAX_SEQ_LENGTH )*/
#endif

typedef struct
{
   int SL, SR, SMAX, SP;
} NODE;

typedef Pochoir_Array< NODE, N_RANK, 3 > pArrayR2T3;

//#define TEST_TYPEDEF

#ifdef TEST_TYPEDEF
  #define P_ARRAY_R2_T3 pArrayR2T3
#else
  #define P_ARRAY_R2_T3 Pochoir_Array< NODE, N_RANK, 3 >
#endif  

#define min2( x, y ) ( ( y ) ^ ( ( ( x ) ^ ( y ) ) & -( ( x ) < ( y ) ) ) ) 
#define max2( x, y ) ( ( x ) ^ ( ( ( x ) ^ ( y ) ) & -( ( x ) < ( y ) ) ) ) 

#define min3( x, y ) ( ( y ) + ( ( ( x ) - ( y ) ) & ( ( ( x ) - ( y ) ) >> 31 ) ) )
#define max3( x, y ) ( ( x ) - ( ( ( x ) - ( y ) ) & ( ( ( x ) - ( y ) ) >> 31 ) ) )

//enum alphabet{ A = -1, U = 1, G = -2, C = 2 };

enum alphabet{ A = 49, C = 88, G = 164, U = 194 };  /* A = 00110001b, U = 11000010b, G = 10100100b, C = 01011000b */

enum err_msgs{ SEQUENCE_READ, LENGTH_READ, NO_SEQUENCE, SEQUENCE_TOO_LONG, INVALID_SEQUENCE_LENGTH, FILE_OPEN_ERROR, MEM_ALLOC_ERROR };


int inline maxx( int a, int b ) 
{ 
  __asm__ ( 
           "cmp     %0, %1\n\t"  
           "cmovge  %1, %0\n\t" 
          : "+r"(a) :  
            "%r"(b) 
          ); 
          
  return a; 
} 

int inline maxxx( int a, int b, int c ) 
{ 
  __asm__ ( 
           "cmp     %0, %1\n\t"  
           "cmovge  %1, %0\n\t" 
           "cmp     %0, %2\n\t" 
           "cmovge  %2, %0\n\t"  
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


int convertToInts( int nX, char *RNA, INT **X )
{
  ( *X ) = ( INT * ) malloc( ( nX + 2 ) * sizeof( INT ) );
  
  if ( ( *X == NULL ) ) 
    {
      printf( "Error: Memory allocation error!\n\n" );      
      return 0;
    }   

  for ( int i = 1; i <= nX; ++i )
    {
      switch ( RNA[ i ] )
        {
          case 'A' : ( *X )[ i ] = A; break;

          case 'U' : ( *X )[ i ] = U; break;

          case 'G' : ( *X )[ i ] = G; break;

          case 'C' : ( *X )[ i ] = C; break;
        }
    }
    
  return 1;  
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


void stencilRNAi0( int nX, INT *X, int i_0, 
                   P_ARRAY_R2_T3 &pArray )
{
    Pochoir< NODE, N_RANK, 3 > pRNA;    
    Pochoir_Shape< N_RANK > pRNA_shape[ 6 ] = { { 2, 0, 0 }, 
                                                { 1, 0, 0 }, { 1, 0, -1 }, { 1, -1, 0 },
                                                { 0, -1, 0 }, { 0, 0, -1 } };    

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
                 
                 int sl = b0 ? ( b1 ? 1 : -INF ) : 0;
                                
                 b0 = ( k == jj + 1 ) || ( i != i_0 - 1 );            
                 b1 = base_pair( X[ jj ], X[ k ] );   
                 
                 int sr = b0 ? ( b1 ? 1 : -INF ) : 0;               
        
                 int sm = 0;
        
                 b0 = ( i > i_0 - 1 ) && ( t > 0 );
                                
                 if ( b0 )
                   {
                     b0 = ( jj < k );

		     int v1 = b0 ? max( pArray.interior( 0, i, k - 1 ).SR, pArray.interior( 0, i, k ).SMAX ) : -INF; 
                     
                     b0 = ( i - 1 < jj );
                     
                     int v2 = b0 ? pArray.interior( 0, i - 1, k ).SL : -INF;
                                                
                     sm = max( v1, v2 );  
                   }
                   
                 sl = max( sl, sm );  
                 sr = max( sr, sm );                   
                                      
                 int smax = max( sl, sr );       
                   
                 pArray.interior( t, i, k ).SL = sl;
                 pArray.interior( t, i, k ).SR = sr;
                 pArray.interior( t, i, k ).SMAX = smax;       
                 
                 b0 = ( i_0 <= i ) && ( jj < k );
                 
                 int sp = pArray.interior( 0, i, k ).SP;
                 
                 pArray.interior( 0, i, k ).SP = b0 ? max( smax, sp ) : -INF;                            
               }  
          }


    Pochoir_kernel_2D( pRNA_fn, t, i, k )
      
       int j = t + 2 - i - k, jj = nX - j + 1;

       if ( ( j >= 0 ) && ( j <= nX ) && ( i < jj ) )
         {                   
           if ( ( i_0 - 1 < i ) && ( jj + 1 < k ) )
             {
	       int sr = pArray( t + 1, i, k - 1 ).SR, sl = pArray( t + 1, i - 1, k ).SL;
	       int smax = pArray( t + 1, i, k ).SMAX;
	       	       	      
               int sm = maxxx( sr, sl, smax );        
                                                    
               sl = not_base_pair( X[ i ], X[ jj ] ) ? -INF : ( 1 + pArray( t, i - 1, k ).SMAX );                       
               sr = not_base_pair( X[ jj ], X[ k ] ) ? -INF : ( 1 + pArray( t, i, k - 1 ).SMAX );        

	       pArray( t + 2, i, k ).SL = sl = max( sl, sm );	
	       pArray( t + 2, i, k ).SR = max( sr, sm );		       

               pArray( t + 2, i, k ).SMAX = smax = max( sl, sr );       
               
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
                   
                   sl = b1 ? 1 : ( b2 ? ( 1 + pArray( t, i - 1, k ).SMAX ) : sl );
                                              
                   b0 = base_pair( X[ jj ], X[ k ] );
                   b1 = ( i_0 - 1 == i );
                   b2 = ( jj + 1 <= k - 1 );                           
                   bool b3 = ( b1 && ( jj + 1 == k ) ) || ( ~b1 && ~b2 );
                   b3 = b3 && b0; 
                   b2 = b2 && b1 && b0;        
                        
                   sr = b3 ? 1 : ( b2 ? ( 1 + pArray( t, i, k - 1 ).SMAX ) : sr );     
                                     
                   if ( i > i_0 - 1 ) 
                     {
                       b0 = ( jj < k );
                       
                       int v1 = b0 ? max( pArray( t + 1, i, k - 1 ).SR, pArray( t + 1, i, k ).SMAX)  : -INF;
                       
                       b0 = ( i - 1 < jj );
                       
                       int v2 = b0 ? pArray( t + 1, i - 1, k ).SL : -INF;
                       
                       sm = max( v1, v2 );  
                     }
          
                   pArray( t + 2, i, k ).SL = sl = max( sl, sm );
                   pArray( t + 2, i, k ).SR = sr = max( sr, sm );
                                                  
                   int smax = max( sl, sr );
                          
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



void iterativeStencilRNAi0( int nX, INT *X, int i_0, 
                            P_ARRAY_R2_T3 &pArray )
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
                 
                 int sl = b0 ? ( b1 ? 1 : -INF ) : 0;
                                
                 b0 = ( k == jj + 1 ) || ( i != i_0 - 1 );            
                 b1 = base_pair( X[ jj ], X[ k ] );   
                 
                 int sr = b0 ? ( b1 ? 1 : -INF ) : 0;               
        
                 int sm = 0;
        
                 b0 = ( i > i_0 - 1 ) && ( t > 0 );
                                
                 if ( b0 )
                   {
                     b0 = ( jj < k );

		     int v1 = b0 ? max( pArray.interior( 0, i, k - 1 ).SR, pArray.interior( 0, i, k ).SMAX ) : -INF; 
                     
                     b0 = ( i - 1 < jj );
                     
                     int v2 = b0 ? pArray.interior( 0, i - 1, k ).SL : -INF;
                                                
                     sm = max( v1, v2 );  
                   }
                   
                 sl = max( sl, sm );  
                 sr = max( sr, sm );                   
                                     
                 int smax = max( sl, sr );       
                   
                 pArray.interior( t, i, k ).SL = sl;
                 pArray.interior( t, i, k ).SR = sr;
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
        	     int sr = pArray.interior( t + 1, i, k - 1 ).SR, sl = pArray.interior( t + 1, i - 1, k ).SL;
        	     int smax = pArray.interior( t + 1, i, k ).SMAX;
      	       	       	      
                     int sm = maxxx( sr, sl, smax );        
                                                          
                     sl = not_base_pair( X[ i ], X[ jj ] ) ? -INF : ( 1 + pArray.interior( t, i - 1, k ).SMAX );                       
                     sr = not_base_pair( X[ jj ], X[ k ] ) ? -INF : ( 1 + pArray.interior( t, i, k - 1 ).SMAX );        
      
      	             pArray.interior( t + 2, i, k ).SL = sl = max( sl, sm );	
      	             pArray.interior( t + 2, i, k ).SR = max( sr, sm );		       
      
                     pArray.interior( t + 2, i, k ).SMAX = smax = max( sl, sr );       
                     
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
                         
                         sl = b1 ? 1 : ( b2 ? ( 1 + pArray.interior( t, i - 1, k ).SMAX ) : sl );
                                                    
                         b0 = base_pair( X[ jj ], X[ k ] );
                         b1 = ( i_0 - 1 == i );
                         b2 = ( jj + 1 <= k - 1 );                           
                         bool b3 = ( b1 && ( jj + 1 == k ) ) || ( ~b1 && ~b2 );
                         b3 = b3 && b0; 
                         b2 = b2 && b1 && b0;        
                              
                         sr = b3 ? 1 : ( b2 ? ( 1 + pArray.interior( t, i, k - 1 ).SMAX ) : sr );     
                                           
                         if ( i > i_0 - 1 ) 
                           {
                             b0 = ( jj < k );
                             
                             int v1 = b0 ? max( pArray.interior( t + 1, i, k - 1 ).SR, pArray.interior( t + 1, i, k ).SMAX ) : -INF;
                             
                             b0 = ( i - 1 < jj );
                             
                             int v2 = b0 ? pArray.interior( t + 1, i - 1, k ).SL : -INF;
                             
                             sm = max( v1, v2 );  
                           }
                
                         pArray.interior( t + 2, i, k ).SL = sl = max( sl, sm );
                         pArray.interior( t + 2, i, k ).SR = sr = max( sr, sm );
                                                        
                         int smax = max( sl, sr );
                                
                         pArray.interior( t + 2, i, k ).SMAX = smax;       
                                    
                         b0 = ( i_0 <= i ) && ( jj < k );           
                         
                         int sp = pArray.interior( 0, i, k ).SP;
                         
                         pArray.interior( 0, i, k ).SP = b0 ? max( smax, sp ) : -INF;
                       }
               }
          }
}




int stencilRNA( int nX, INT *X, bool recursive )
{
    P_ARRAY_R2_T3 pArray( nX + 1, nX + 1 );
    int S[ nX + 2][ nX + 2 ];

    for ( int i_0 = 1; i_0 <= 1/*nX*/; ++i_0 )
      {
        if ( recursive ) stencilRNAi0( nX, X, i_0, pArray );
//        else iterativeStencilRNAi0( nX, X, i_0, pArray );

        for ( int k_0 = 1, v = -INF; k_0 <= nX; ++k_0 )
          {
            v = max( pArray.interior( 0, i_0, k_0 ).SP, v );
        
            for ( int i = i_0 + 1; i < k_0 - 1; ++i )
               v = max( pArray.interior( 0, i, k_0 ).SP, v );
        
            pArray.interior( 0, i_0, k_0 ).SP = v; 
          } 
        
        for ( int k_0 = 0; k_0 < i_0 + 2; ++k_0 )
           pArray.interior( 1, i_0, k_0 ).SP = -INF;
                
        for ( int k_0 = i_0 + 2; k_0 <= nX; ++k_0 )
           pArray.interior( 1, i_0, k_0 ).SP = max( pArray.interior( 1, i_0, k_0 - 1 ).SP, pArray.interior( 0, i_0, k_0 ).SP );            
      }
      
    for ( int j = 0; j <= nX + 1; ++j )
       S[ nX + 1 ][ j ] = 0;

    for ( int i = 0; i <= nX + 1; ++i )
       S[ i ][ 0 ] = 0;

    for ( int i = nX; i >= 1; --i )
      for ( int j = i + 1; j <= nX; ++j )
        {
          int v;
          
          if ( base_pair( X[ i ], X[ j ] ) ) v = 1 + S[ i + 1 ][ j - 1 ];
          else v = -INF;
          
          v = max( v, pArray.interior( 1, i, j ).SP );
          
          for ( int k = i + 1; k <= j; k++ )
            v = max( v, S[ i ][ k - 1 ] + S[ k ][ j ] );
            
          S[ i ][ j ] = v; 
        }  

    return S[ 1 ][ nX ];
}



int main( int argc, char *argv[ ] )
{
    printf( "\nStencil-based DP for RNA secondary structure prediction with simple pseudoknots ( run with option -h for help ).\n\n" );

    int nX;
    INT *X = NULL;        
    char *RNA = NULL;    
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
        
    if ( gotSeq && convertToInts( nX, RNA, &X ) )
      {
        printf( "Sequence length = %d\n\n", nX );
      
        struct timeval start, end;

        printf( "Running pochoir-based DP..." );
        fflush( stdout );
               
        gettimeofday( &start, 0 );        
        int maxNumBP = stencilRNA( nX, X, true );    
        gettimeofday( &end, 0 );

        double t0 = tdiff( &end, &start );
              
        printf( "\n\nPochoir:\n" );
        if ( maxNumBP == -INF ) printf( "\t maximum number of base pairs = -inf\n" );    
        else printf( "\t maximum number of base pairs = %d\n", maxNumBP );    
        printf( "\t running time = %.3lf sec\n\n", t0 );    
      
        if ( runIterativeStencil )
          {
            printf( "Running iterative stencil..." );
            fflush( stdout );
                          
            gettimeofday( &start, 0 );        
            int maxNumBPITST = stencilRNA( nX, X, false );
            gettimeofday( &end, 0 );

            double t1 = tdiff( &end, &start );
          
            printf( "\n\nIterative Stencil:\n" );
            if ( maxNumBPITST == -INF ) printf( "\t maximum number of base pairs = -inf\n" );    
            else printf( "\t maximum number of base pairs = %d\n", maxNumBPITST );                
            if ( t0 > 0 ) printf( "\t running time = %.3lf sec ( %.3lf x Pochoir )\n\n", t1, t1 / t0 );    
            else printf( "\t running time = %.3lf sec\n\n", t1 );    
          }
      }

    if ( RNA != NULL ) free( RNA );
    if ( X != NULL ) free( X );
       
    return 0;
}
