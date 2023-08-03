#ifndef _GENETIC_H
#define _GENETIC_H

#include "problem.h"

typedef struct
{
    FILE *scoreFile;
    FILE *bestResultFile;
    uint64_t maxIteration;
    int populationSize;
    int eliteCount;
    bool randomSelection;
    float mutationRate;
    float mutationDistance;
    float restartProbability;
    bool restartWhenSameScore;
}GeneticSettings;

void genetic_run(Problem *problem, GeneticSettings *settings);

#endif
