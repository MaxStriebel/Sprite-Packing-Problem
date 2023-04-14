#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "pcg_basic.h"

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

typedef struct
{
    int Width;
    int Height;
    uint8_t *Cells;
}shape;

typedef struct
{
    int X;
    int Y;
}vector;

typedef struct
{
    int ShapeCount;
    shape *Shapes;
    vector CenterOffset;
    int CellWidth;
    int CellHeight;
    int CellCount;
    uint8_t *Cells;
}problem;

shape shape_allocate(int Width, int Height)
{
    shape Result =
    {
        .Width = Width,
        .Height = Height,
        .Cells = calloc(Width * Height, sizeof (Result.Cells[0]))
    };
    return Result;
}

problem problem_allocate(int ShapeCount, int MaxExtends)
{
    assert(MaxExtends >= 0);
    int CellWidth = 2 * MaxExtends + 1;
    int CellHeight = CellWidth;
    int CellCount = CellWidth * CellHeight;
    problem Result =
    {
        .ShapeCount = ShapeCount,
        .Shapes = calloc(ShapeCount, sizeof (Result.Shapes[0])),
        .CenterOffset = {MaxExtends + 1, MaxExtends + 1},
        .CellWidth = CellWidth,
        .CellHeight = CellHeight,
        .CellCount = CellCount,
        .Cells = calloc(CellCount, sizeof (Result.Cells[0]))
    };
    return Result;
}

vector vector_add(vector A, vector B)
{
    vector Result = {A.X + B.X, A.Y + B.Y};
    return Result;
}

int calculateScore(problem *Problem, vector *Offsets)
{
    memset(Problem->Cells, 0, Problem->CellCount);
    for(int i = 0; i < Problem->ShapeCount; i++)
    {
        vector Offset = {0, 0};
        if(i > 0) Offset = Offsets[i - 1];
        Offset = vector_add(Offset, Problem->CenterOffset);
        uint8_t *TargetLine = Problem->Cells + (Offset.X + Offset.Y * Problem->CellWidth);
        shape Shape = Problem->Shapes[i];
        uint8_t *SourceCell = Shape.Cells;
        for(int y = 0; y < Shape.Height; y++)
        {
            uint8_t *TargetCell = TargetLine;
            for(int x = 0; x < Shape.Width; x++)
            {
                *TargetCell += *SourceCell;
                TargetCell++;
                SourceCell++;
            }
            TargetLine += Problem->CellWidth;
        }
    }
    int MinX = INT_MAX;
    int MinY = INT_MAX;
    int MaxX = INT_MIN;
    int MaxY = INT_MIN;
    int Overlap = 0;
    uint8_t *Cell = Problem->Cells;
    for(int y = 0; y < Problem->CellHeight; y++)
    {
        for(int x = 0; x < Problem->CellWidth; x++)
        {
            if(*Cell)
            {
                MinX = MIN(MinX, x);
                MinY = MIN(MinY, y);
                MaxX = MAX(MaxX, x);
                MaxY = MAX(MaxY, y);
                Overlap += *Cell - 1;
            }
            Cell++;
        }
    }
    int Width = MaxX - MinX;
    int Height = MaxY - MinY;
    int Error = MAX(Width, Height) * Overlap;
    Error = 5 * Overlap;
    return Width * Height + Error;
}

void printGrid(problem *Problem, int Min, int Max)
{
    printf("x, y, score\n");
    for(int y = Min; y <= Max; y++)
    {
        for(int x = Min; x < Max; x++)
        {
            vector Point = {x, y};
            int score = calculateScore(Problem, &Point);
            printf("%i, %i, %i\n", x, y, score);
        }
    }
}
            
int main()
{
    problem Problem = problem_allocate(2, 10);
    shape Shape0 = {2, 2, (uint8_t[]){1, 1, 1, 0}};
    shape Shape1 = {2, 2, (uint8_t[]){0, 0, 0, 1}};
    Problem.Shapes = (shape []){Shape0, Shape1};
    printGrid(&Problem, -8, 8);
    return 0;
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 42u, 54u);
    for(int i = 0; i < 10; i++)
        printf("random: 0x%X\n", pcg32_random_r(&rng));
    return 0;
}
