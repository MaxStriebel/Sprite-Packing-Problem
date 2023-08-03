#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "problem.h"
#include "random.h"

typedef struct
{
    void *chromosom;
    int score;
    double weight;
}Individual;

typedef struct
{
    Problem *problem;
    RandomSettings *settings;
    Individual current;
    Individual best;
    uint64_t iteration;
}Context;

static void calculateAndPrintScore(Context *context, Individual *individual)
{
    Score score = context->problem->calculateScore(context->problem, individual->chromosom);
    individual->score = score.score;
    if(context->settings->scoreFile)
        fprintf(context->settings->scoreFile, "%li, %i, %i, %i\n", 
                context->iteration, score.score, score.rawScore, score.overlap);
    if(score.overlap == 0 && context->best.score > score.score)
    {
        memcpy(context->best.chromosom, 
               individual->chromosom, 
               context->problem->chromosomSize);
        context->best.score = score.score;
    }
    context->iteration++;
}

static void printCSVHeader(Context *context)
{
    if(context->settings->scoreFile)
        fprintf(context->settings->scoreFile, "iteration,score,rawScore,overlap\n");
}

void random_run(Problem *problem, RandomSettings *settings)
{
    Context context = 
    {
        .problem = problem,
        .settings = settings,
        .current.chromosom = malloc(problem->chromosomSize),
        .best = 
        {
            .chromosom = malloc(problem->chromosomSize),
            .score = INT_MAX,
        }
    };
    printCSVHeader(&context);
    while(context.iteration < settings->maxIteration)
    {
        problem->initializeChromosom(problem, context.current.chromosom);
        calculateAndPrintScore(&context, &context.current);
    }
    if(problem->printChromosom && settings->bestResultFile)
        problem->printChromosom(problem, context.best.chromosom, 
                                settings->bestResultFile);
}

