#include <stdint.h>
#include <float.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "problem.h"
#include "genetic.h"
#include "pcg_basic.h"

typedef struct
{
    void *chromosom;
    int score;
    double weight;
}Individual;

typedef struct
{
    Problem *problem;
    GeneticSettings *settings;
    Individual *current;
    Individual *next;
    Individual best;
    uint64_t iteration;
}Context;

static int individual_compareDesc(const void *a, const void *b)
{
    return ((Individual *)a)->weight < ((Individual *)b)->weight;
}

static Individual individual_getRandomWeighted(Individual *Individuals, int Count)
{
    float target = pcg32_random() / (double)UINT32_MAX;
    double Sum = 0;
    for(int i = 0; i < Count; i++)
    {
        Sum += Individuals[i].weight;
        if(Sum >= target)
            return Individuals[i];
    }
    assert(Sum > 0.99);
    return Individuals[Count - 1];
}

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

static bool currentHaveSameScore(Context *context)
{
    Problem *problem = context->problem;
    GeneticSettings *settings = context->settings;
    Individual *current = context->current;

    int score = current[0].score;
    for(int i = 0; i < settings->populationSize; i++)
        if(current[i].score != score)
            return false;
    return true;
}

static bool genetic_step(Context *context)
{
    Problem *problem = context->problem;
    GeneticSettings *settings = context->settings;
    Individual *current = context->current;
    Individual *next = context->next;

    double totalInvScore = 0;
    for(int i = 0; i < settings->populationSize; i++)
        totalInvScore += 1.0 / current[i].score;
    for(int i = 0; i < settings->populationSize; i++)
    {
        if(settings->randomSelection)
            current[i].weight = 1.0 / settings->populationSize;
        else
            current[i].weight = (1.0 / current[i].score) / totalInvScore;
    }
    qsort(current, settings->populationSize, sizeof (Individual), individual_compareDesc);
    for(int i = 0; i < settings->eliteCount; i++)
    {
        memcpy(next[i].chromosom, current[i].chromosom, problem->chromosomSize);
        next[i].score = current[i].score;
    }
    for(int i = settings->eliteCount; i < settings->populationSize; i+=2)
    {
        Individual mother = individual_getRandomWeighted(current, settings->populationSize);
        Individual father = individual_getRandomWeighted(current, settings->populationSize);
        //TODO: check that mother != father
        problem->crossover(problem, mother.chromosom, father.chromosom,
                           next[i].chromosom, next[i+1].chromosom);
        problem->mutate(problem, settings->mutationRate, 
                        settings->mutationDistance, next[i].chromosom);
        calculateAndPrintScore(context, next + i);
        if(i + 1 < settings->populationSize)
        {
            problem->mutate(problem, settings->mutationRate, 
                    settings->mutationDistance, next[i+1].chromosom);
            calculateAndPrintScore(context, next + i + 1);
        }
        if(pcg32_fraction() <= settings->restartProbability)
            return true;
    }
    return false;
}

void genetic_run(Problem *problem, GeneticSettings *settings)
{
    Context context = 
    {
        .problem = problem,
        .settings = settings,
        .current = calloc(settings->populationSize + 1, sizeof (Individual)),
        .next    = calloc(settings->populationSize + 1, sizeof (Individual))
    };
    printCSVHeader(&context);
    size_t individualCount = settings->populationSize * 2 + 3;
    char *chromosomes = malloc(individualCount * problem->chromosomSize);
    context.best = (Individual)
    {
        .chromosom = chromosomes,
        .score = INT_MAX,
    };
    chromosomes += problem->chromosomSize;
    for(int i = 0; i < settings->populationSize + 1; i++)
    {
        context.current[i].chromosom = chromosomes;
        context.next[i].chromosom    = chromosomes + problem->chromosomSize;
        chromosomes += problem->chromosomSize * 2;
        if(i < settings->populationSize)
        {
            problem->initializeChromosom(problem, context.current[i].chromosom);
            calculateAndPrintScore(&context, &context.current[i]);
        }
    }

    while(context.iteration < settings->maxIteration)
    {
        if((settings->restartWhenSameScore && currentHaveSameScore(&context))
                    || genetic_step(&context))
        {
            for(int i = 0; i < settings->populationSize; i++)
            {
                problem->initializeChromosom(problem, context.next[i].chromosom);
                calculateAndPrintScore(&context, &context.next[i]);
            }
        }
        Individual *Tmp = context.current;
        context.current = context.next;
        context.next = Tmp;
    }
    if(problem->printChromosom && settings->bestResultFile)
        problem->printChromosom(problem, context.best.chromosom, 
                                settings->bestResultFile);
}
