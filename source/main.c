#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#include "pcg_basic.h"
#include "data.h"

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(a, min, max) (MAX(MIN(a, max), min))
#define array_length(Array) (sizeof(Array) / sizeof(Array[0]))

#include "spritePacking.c"
#include "genetic.h"
#include "random.h"


#define SPRITES(name) (Sprites){name ## _Width, name ## _Height, name, #name}

void printProblem(Sprites sprites)
{
    char buffer[512];
    system("mkdir -p data/problems");
    snprintf(buffer, sizeof(buffer), "data/problems/%s.csv", sprites.name);
    FILE *file = fopen(buffer, "w");
    spritePacking_printProblem(sprites.width, sprites.height, sprites.indexes, file);
    fclose(file);
}

void evaluateRandom(Problem *problems, 
                    int problemCount,
                    char *folderName,
                    SpritePackerSettings packerSettings, 
                    RandomSettings randomSettings)
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "mkdir -p data/%s/scores", folderName);
    system(buffer);
    snprintf(buffer, sizeof(buffer), "mkdir -p data/%s/best", folderName);
    system(buffer);
    for(int problemIndex = 0; problemIndex < problemCount; problemIndex++)
    {
        Problem *problem = &problems[problemIndex];
        spritePacking_setSettings(problem, packerSettings);
        snprintf(buffer, sizeof(buffer), "data/%s/scores/%s_0.csv", folderName, problem->name);
        randomSettings.scoreFile = fopen(buffer, "w");
        snprintf(buffer, sizeof(buffer), "data/%s/best/%s_0.csv", folderName, problem->name);
        randomSettings.bestResultFile = fopen(buffer, "w");
        fprintf(randomSettings.scoreFile, "optimum\n");
        fprintf(randomSettings.scoreFile, "%i\n", problem->width * problem->height);
        random_run(problem, &randomSettings);
        fclose(randomSettings.scoreFile);
        fclose(randomSettings.bestResultFile);
    }
    snprintf(buffer, sizeof(buffer), "python3 source/graph.py data/%s/scores", folderName);
    system(buffer);
    snprintf(buffer, sizeof(buffer), "python3 source/graph.py data/%s/best -t image", folderName);
    system(buffer);
}

void evaluateGenetic(Problem *problems, 
                     int problemCount,
                     char *folderName,
                     SpritePackerSettings packerSettings, 
                     GeneticSettings geneticSettings)
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "mkdir -p data/%s/scores", folderName);
    system(buffer);
    snprintf(buffer, sizeof(buffer), "mkdir -p data/%s/best", folderName);
    system(buffer);
    for(int problemIndex = 0; problemIndex < problemCount; problemIndex++)
    {
        Problem *problem = &problems[problemIndex];
        spritePacking_setSettings(problem, packerSettings);
        snprintf(buffer, sizeof(buffer), "data/%s/scores/%s_0.csv", folderName, problem->name);
        geneticSettings.scoreFile = fopen(buffer, "w");
        snprintf(buffer, sizeof(buffer), "data/%s/best/%s_0.csv", folderName, problem->name);
        geneticSettings.bestResultFile = fopen(buffer, "w");
        fprintf(geneticSettings.scoreFile, "optimum\n");
        fprintf(geneticSettings.scoreFile, "%i\n", problem->width * problem->height);
        genetic_run(problem, &geneticSettings);
        fclose(geneticSettings.scoreFile);
        fclose(geneticSettings.bestResultFile);
    }
    snprintf(buffer, sizeof(buffer), "python3 source/graph.py data/%s/scores", folderName);
    system(buffer);
    snprintf(buffer, sizeof(buffer), "python3 source/graph.py data/%s/best -t image", folderName);
    system(buffer);
}

int main()
{
    char buffer[512];
    Problem problems[3] = 
    {
        spritePacking_createProblemFromIndexes(SPRITES(Box0)),
        spritePacking_createProblemFromIndexes(SPRITES(Box1)),
        spritePacking_createProblemFromIndexes(SPRITES(Blob1))
    };

    printProblem(SPRITES(Box0));
    printProblem(SPRITES(Box1));
    printProblem(SPRITES(Box2));
    printProblem(SPRITES(Blob1));
    system("python3 source/graph.py data/problems/ -t image --info");

    evaluateRandom(problems, array_length(problems), "random_noError",
            (SpritePackerSettings) {
                .positionEncoding = POS_CARTESIAN,
                .disableErrorTerm = true},
            (RandomSettings) {
                .maxIteration = 15000,
            });
    evaluateRandom(problems, array_length(problems), "random",
            (SpritePackerSettings) {.positionEncoding = POS_CARTESIAN},
            (RandomSettings) {
                .maxIteration = 15000,
            });
    evaluateRandom(problems, array_length(problems), "random_dir",
            (SpritePackerSettings) {.positionEncoding = MOV_DIRECTION},
            (RandomSettings) {
                .maxIteration = 15000,
            });
    evaluateGenetic(problems, array_length(problems), "mGA",
        (SpritePackerSettings) {.positionEncoding = POS_CARTESIAN },
        (GeneticSettings) {
            .maxIteration = 15000,
            .populationSize = 5,
            .eliteCount = 1,
            .randomSelection = true,
            .mutationRate = 0,
            .restartWhenSameScore = true,
        });
    evaluateGenetic(problems, array_length(problems), "GA",
        (SpritePackerSettings) {.positionEncoding = POS_CARTESIAN },
        (GeneticSettings) {
            .maxIteration = 15000,
            .populationSize = 100,
            .eliteCount = 5,
            .mutationRate = .1,
            .mutationDistance = .1,
            .restartProbability = 1./1500,
            .restartWhenSameScore = true,
        });
    evaluateGenetic(problems, array_length(problems), "GA_dir",
        (SpritePackerSettings) {.positionEncoding = MOV_DIRECTION },
        (GeneticSettings) {
            .maxIteration = 15000,
            .populationSize = 100,
            .eliteCount = 5,
            .mutationRate = 0.05,
            .mutationDistance = 0.3,
            .restartProbability = 1./1500,
            .restartWhenSameScore = true,
        });
    evaluateGenetic(problems, array_length(problems), "GA_mov",
        (SpritePackerSettings) {.positionEncoding = MOV_CARTESIAN },
        (GeneticSettings) {
            .maxIteration = 15000,
            .populationSize = 100,
            .eliteCount = 5,
            .mutationRate = 0.05,
            .mutationDistance = 0.3,
            .restartProbability = 1./1000,
            .restartWhenSameScore = true,
        });
    return 0;
}
