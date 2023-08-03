#ifndef _RANDOM_H
#define _RANDOM_H

#include "problem.h"

typedef struct
{
    FILE *scoreFile;
    FILE *bestResultFile;
    uint64_t maxIteration;
}RandomSettings;

void random_run(Problem *problem, RandomSettings *settings);

#endif
