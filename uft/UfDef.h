
#pragma once

#include "UfTypes.h"


typedef enum {
    UFS_SUCCESS,
    UFS_DISCONNECT,
    UFS_TIMEOUT,
    UFS_FAIL,
    UFS_OTHER,
} UfState;


#define UFGRAPHIC_EMPTY   '.'
#define UFGRAPHIC_COMPLETED 'O'
#define UFGRAPHIC_UNCOMPLETED 'o'


