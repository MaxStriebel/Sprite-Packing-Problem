#include <stdio.h>
#include <stdlib.h>

#include "pcg_basic.h"


typedef struct
{
    int min;
    int max;
}range;

typedef struct
{
    int length;
    int *values;
}vector;

typedef struct
{
    int N;
    int k;
    range *ranges;
    int rangeCount;
}problem;

vector createVector(int length)
{
    vector result =
    {
        .length = length,
        .values = calloc(length, sizeof (int))
    };
    return result;
}

problem createProblem(int N, int K, int RangeCount)
{
    problem result = 
    {
        .N = N,
        .k = K,
        .ranges = calloc(RangeCount, sizeof (range)),
        .rangeCount = RangeCount
    };
    return result;
}

int calculateScore(problem *Problem, vector Point)
{
    int result = 0;
    for(int i = 0; i < Problem->rangeCount; i++)
    {
        range r = Problem->ranges[i];
        for(int j = 0; j < Point.length; j++)
        {
            if(Point.values[j] >= r.min && Point.values[j] <= r.max)
            {
                result++;
                break;
            }
        }
    }
    return result;
}

void printGrid(problem *Problem)
{
    printf("x, y, score\n");
    vector Point = createVector(2);
    for(int y = 0; y < Problem->N; y++)
    {
        Point.values[1] = y;
        for(int x = 0; x < Problem->N; x++)
        {
            Point.values[0] = x;
            int score = calculateScore(Problem, Point);
            printf("%i, %i, %i\n", x, y, score);
        }
    }
}
            
int main()
{
    problem Problem = createProblem(10, 2, 5);
    Problem.ranges = (range[]){{1, 5}, {6, 9}, {0, 3}, {4, 8}, {2, 5}};
    printGrid(&Problem);
    return 0;
    vector p0 = {.length = 2, .values = (int[]){2, 6}};
    vector p1 = {.length = 2, .values = (int[]){4, 8}};
    printf("{2, 6}: %i\n", calculateScore(&Problem, p0));
    printf("{4, 8}: %i\n", calculateScore(&Problem, p1));
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 42u, 54u);
    for(int i = 0; i < 10; i++)
        printf("random: 0x%X\n", pcg32_random_r(&rng));
    return 0;
}
