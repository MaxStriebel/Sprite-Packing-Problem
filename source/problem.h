#ifndef PROBLEM_H
#define PROBLEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct
{
    int score;
    int rawScore;
    int overlap;
}Score;

typedef struct Problem Problem;

struct Problem
{
    void *data;
    char *name;
    int width;
    int height;
    size_t chromosomSize;
    void (*initializeChromosom)(Problem *, void *);
    Score (*calculateScore)(Problem *, void *);
    void (*crossover)(Problem *problem, void *mother, void *father, 
                                        void *child0, void *child1);
    void (*mutate)(Problem *problem, 
                   float mutationRate,
                   float muationDistance,
                   void *chromosom);
    void (*printChromosom)(Problem *problem, 
                           void *chromosomData, 
                           FILE *file);
};

#endif
