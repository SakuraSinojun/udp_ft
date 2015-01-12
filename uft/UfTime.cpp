
#include "UfTime.h"
#include <sys/time.h>
#include <stdio.h>

long long UfTime::now()
{
    struct timeval  now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}


