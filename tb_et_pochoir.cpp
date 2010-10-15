/* comment */

#include <cstdio>
#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cmath>

#include "expr_array.hpp"

using namespace std;
#define SIMPLE 0
#define N_RANK 3
#define N_SIZE 555
#define T_SIZE 505
void check_result(int t, int j, int i, double a, double b)
{
	if (a == b) {
		printf("a(%d, %d, %d) == b(%d, %d, %d) == %f : passed!\n", t, j, i, t, j, i, a);
	} else {
		printf("a(%d, %d, %d) = %f, b(%d, %d, %d) = %f : FAILED!\n", t, j, i, a, t, j, i, b);
	}
}

void check_result(int t, int i, double a, double b)
{
	if (a == b) {
		printf("a(%d, %d) == b(%d, %d) == %f : passed!\n", t, i, t, i, a);
	} else {
		printf("a(%d, %d) = %f, b(%d, %d) = %f : FAILED!\n", t, i, a, t, i, b);
	}

}

int main(void)
{
	const int BASE = 1024;
	SArray <double, 3> a(505, 555, 555), b(505, 555, 555);
	SArray <double, 2> c(505, 555), d(505, 555);
	uRange I(0, 553), J(1, 553), T(0, 505);
	int t;
	struct timeval start, end;

	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
#if DEBUG
		a(0, i, j) = i * N_SIZE + j;
		a(1, i, j) = 0;
#else
		a(0, i, j) = 1.0 * (rand() % BASE); 
		a(1, i, j) = 0; 
#endif
		b(0, i, j) = a(0, i, j);
		b(1, i, j) = 0; 
	} }
	for (int i = 0; i < N_SIZE; ++i) {
#if DEBUG 
		c(0, i) = i;
		c(1, i) = 0;
#else
		c(0, i) = 1.0 * (rand() % BASE); 
		c(1, i) = 0; 
#endif
        d(0, i) = c(0, i);
        d(1, i) = 0;
	} 

#if SIMPLE
	cout << endl << "\na(T+1, J, I) = 0.01 + a(T, J, I)" << endl;

	gettimeofday(&start, 0);
    
	{ // RegionT : Periodic
	size_t l_slope[2] = {1, 1};
	pochoir_p(T, I, J, l_slope, 
	[&a, &c](size_t t, size_t i, size_t j){
	a(t + 1, i, j) = 1.0e-2 + a(t, i - 1, j);
	c(t + 1, i) = 1.0e-2 + c(t, i);}
	// Derive Boundary : True
	, 
	[&a, &c](size_t t, size_t i, size_t j){
	size_t idx0 = (i - 0 + 554) % 554 + 0;
	size_t idx1 = (j - 1 + 553) % 553 + 1;
	size_t idx2 = (i - 0 + 554 - 1) % 554 + 0;
	a(t + 1, idx0, idx1) = 1.0e-2 + a(t, idx2, idx1);
	c(t + 1, idx0) = 1.0e-2 + c(t, idx0);}
	);}/* end of Pochoir block */
	
	{ // RegionT : Nonperiodic
	size_t l_slope[2] = {1, 1};
	pochoir(T, I, l_slope, 
	[&c](size_t t, size_t i){
	c(t + 1, i) = 1.0e-2 + c(t, i - 1);}
	// Derive Boundary : True
	, 
	[&c](size_t t, size_t i){
	c(t + 1, i) = 0;}
	);}/* end of Pochoir block */
	gettimeofday(&end, 0);
	std::cout << "Expression Template: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	gettimeofday(&start, 0);
	for (int t = 0; t < T_SIZE+1; ++t) {
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		b(t+1, i, j) = 0.01 + b(t, i, j);
	} } }
    for (int t = 0; t < T_SIZE+1; ++t) {
	for (int i = 1; i <= N_SIZE-2; ++i) {
		d(t+1, i) = 0.01 + d(t, i);
	} } 
	gettimeofday(&end, 0);

	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	t = T_SIZE+1;
    std::cout << "Checking a =~ b ..." << std::endl;
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		check_result(t, i, j, a(t, i, j), b(t, i, j));
	} } 
    std::cout << "Checking c =~ d ..." << std::endl;
	for (int i = 1; i <= N_SIZE-2; ++i) {
		check_result(t, i, c(t, i), d(t, i));
	}  
#else
	cout << "a(T+1, J, I) = 0.125 * (a(T, J+1, I) - 2.0 * a(T, J, I) + a(T, J-1, I)) + 0.125 * (a(T, J, I+1) - 2.0 * a(T, J, I) + a(T, J, I-1)) + a(T, J, I)" << endl;

	gettimeofday(&start, 0);
    
	{ // RegionT : Periodic
	size_t l_slope[3] = {1, 1, 1};
	pochoir_p(T, I, J, l_slope, 
	[&a](size_t t, size_t i, size_t j){
	a(t + 1, i, j) = 0.125 * (a(t, i + 1, j) - 2.0 * a(t, i, j) + a(t, i - 1, j)) + 0.125 * (a(t, i, j + 1) - 2.0 * a(t, i, j) + a(t, i, j - 1)) + a(t, i, j);}
	// Derive Boundary : True
	, 
	[&a](size_t t, size_t i, size_t j){
	size_t idx0 = (i - 0 + 554) % 554 + 0;
	size_t idx1 = (j - 1 + 553) % 553 + 1;
	size_t idx2 = (i - 0 + 554 + 1) % 554 + 0;
	size_t idx3 = (i - 0 + 554 - 1) % 554 + 0;
	size_t idx4 = (j - 1 + 553 + 1) % 553 + 1;
	size_t idx5 = (j - 1 + 553 - 1) % 553 + 1;
	a(t + 1, idx0, idx1) = 0.125 * (a(t, idx2, idx1) - 2.0 * a(t, idx0, idx1) + a(t, idx3, idx1)) + 0.125 * (a(t, idx0, idx4) - 2.0 * a(t, idx0, idx1) + a(t, idx0, idx5)) + a(t, idx0, idx1);}
	);}/* end of Pochoir block */
	
	{ // RegionT : Periodic
	size_t l_slope[2] = {1, 1};
	pochoir_p(T, I, l_slope, 
	[&c](size_t t, size_t i){
	c(t + 1, i) = 0.1 * (c(t, i + 1) + 2.0 * c(t, i) + c(t, i - 1));}
	// Derive Boundary : True
	, 
	[&c](size_t t, size_t i){
	size_t idx0 = (i - 0 + 554) % 554 + 0;
	size_t idx1 = (i - 0 + 554 + 1) % 554 + 0;
	size_t idx2 = (i - 0 + 554 - 1) % 554 + 0;
	c(t + 1, idx0) = 0.1 * (c(t, idx1) + 2.0 * c(t, idx0) + c(t, idx2));}
	);}/* end of Pochoir block */
	gettimeofday(&end, 0);
	std::cout << "Expression Template: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	gettimeofday(&start, 0);
	for (int t = 0; t <= T_SIZE; ++t) {
    for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		b(t+1, i, j) = 0.125 * (b(t, i+1, j) - 2.0 * b(t, i, j) + b(t, i-1, j)) + 
			           0.125 * (b(t, i, j+1) - 2.0 * b(t, i, j) + b(t, i, j-1)) +
			           b(t, i, j);
	} } }
	for (int t = 0; t < T_SIZE+1; ++t) {
    for (int i = 1; i <= N_SIZE-2; ++i) {
		d(t+1, i) = 0.1 * (d(t, i+1) + 2.0 * d(t, i) + d(t, i-1));
	} } 
    gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	t = T_SIZE+1;
    std::cout << "Checking a =~ b ..." << std::endl;
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		check_result(t, i, j, a(t, i, j), b(t, i, j));
	} } 
    std::cout << "Checking c =~ d ..." << std::endl;
	for (int i = 1; i <= N_SIZE-2; ++i) {
		check_result(t, i, c(t, i), d(t, i));
	}  
#endif

#if 0
// comment
// comment
// comment
	cout << "a = " << a << endl;
// comment
// comment
#endif
	return 0;
}

