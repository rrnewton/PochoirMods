////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#include <Windows.h>

static inline unsigned long long getticks()
{
     return GetTickCount();
}

static inline double getseconds()
{
     return getticks() * 1.0e-3;
}
#endif

#if defined __unix__ || defined __APPLE__
#include <time.h>
#include <sys/time.h>
#include <algorithm>

using std::min;
using std::max;

static inline unsigned long long getticks()
{
     struct timeval t;
     gettimeofday(&t, 0);
     return t.tv_sec * 1000000ULL + t.tv_usec;
}

static inline double getseconds()
{
     return getticks() * 1.0e-6;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
