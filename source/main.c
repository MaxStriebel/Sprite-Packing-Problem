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
#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof(Array[0]))
#define sizeof_member(type, member) sizeof(((type *)0)->member)
#define typeof_member(type, member) typeof(((type *)0)->member)

#include "spritePacking.c"
#include "genetic.c"

#if 0
#define PRINT_ITERATIONS
int ScoreIteration = 0;

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
    vector MinOrigin;
    vector MaxOrigin;
    vector CenterOffset;
    int CellWidth;
    int CellHeight;
    int CellCount;
    uint8_t *Cells;
}problem;

typedef struct
{
    vector *Point;
    int Score;
    double Weight;
}individual;

typedef struct
{
    int currentPopulationSize;
    int nextPopulationSize;
}settings;

int pcg32_range(int min, int max)
{
    int Result = pcg32_boundedrand(max - min) + min;
    return Result;
}



vector vector_add(vector A, vector B)
{
    vector Result = {A.X + B.X, A.Y + B.Y};
    return Result;
}

vector vector_sub(vector A, vector B)
{
    vector Result = {A.X - B.X, A.Y - B.Y};
    return Result;
}

vector vector_min(vector A, vector B)
{
    vector Result = {MIN(A.X, B.X), MIN(A.Y, B.Y)};
    return Result;
}

vector vector_max(vector A, vector B)
{
    vector Result = {MAX(A.X, B.X), MAX(A.Y, B.Y)};
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

problem problem_createFromShapes(int ShapeCount, shape *Shapes)
{
    assert(ShapeCount > 0);
    int TotalWidth = 0;
    int TotalHeight = 0;
    int MaxWidth = 0;
    int MaxHeight = 0;
    for(int i = 0; i < ShapeCount; i++)
    {
        TotalWidth += Shapes[i].Width;
        TotalHeight += Shapes[i].Height;
        MaxWidth = MAX(MaxWidth, Shapes[i].Width);
        MaxHeight = MAX(MaxHeight, Shapes[i].Height);
    }
    vector CenterOffset = {TotalWidth - Shapes[0].Width, 
                           TotalHeight - Shapes[0].Height};
    int CellWidth = 2 * TotalWidth - Shapes[0].Width + MaxWidth;
    int CellHeight = 2 * TotalHeight - Shapes[0].Height + MaxHeight;
    int CellCount = CellWidth * CellHeight;
    problem Result = 
    {
        .ShapeCount = ShapeCount,
        .Shapes = Shapes,
        .MinOrigin = {-CenterOffset.X, -CenterOffset.Y},
        .MaxOrigin = {TotalWidth, TotalHeight},
        .CenterOffset = CenterOffset,
        .CellWidth = CellWidth,
        .CellHeight = CellHeight,
        .CellCount = CellCount,
        .Cells = calloc(CellCount, sizeof (Result.Cells[0]))
    };
    return Result;
}

problem problem_createFromIndexes(int Width, int Height, uint8_t *Indexes)
{
    int MaxIndex = 0;
    for(int i = 0; i < Width * Height; i++)
        MaxIndex = MAX(MaxIndex, Indexes[i]);
    int ShapeCount = MaxIndex + 1;
    vector *MinShapes = malloc(ShapeCount * sizeof (vector));
    vector *MaxShapes = malloc(ShapeCount * sizeof (vector));
    for(int i = 0; i < ShapeCount; i++)
    {
        MinShapes[i] = (vector){INT_MAX, INT_MAX};
        MaxShapes[i] = (vector){INT_MIN, INT_MIN};
    }
    uint8_t *Index = Indexes;
    for(int y = 0; y < Height; y++)
    {
        for(int x = 0; x < Width; x++)
        {
            assert(*Index >= 0);
            assert(*Index < ShapeCount);
            MinShapes[*Index] = vector_min(MinShapes[*Index], (vector){x, y});
            MaxShapes[*Index] = vector_max(MaxShapes[*Index], (vector){x, y});
            Index++;
        }
    }
    shape *Shapes = malloc(ShapeCount * sizeof (shape));
    for(int i = 0; i < ShapeCount; i++)
    {
        vector Size = vector_sub(MaxShapes[i], MinShapes[i]);
        shape Shape = shape_allocate(Size.X + 1, Size.Y + 1);
        uint8_t *Cell = Shape.Cells;
        for(int y = MinShapes[i].Y; y <= MaxShapes[i].Y; y++)
        {
            for(int x = MinShapes[i].X; x <= MaxShapes[i].X; x++)
            {
                *Cell = Indexes[x + y * Width] == i;
                Cell++;
            }
        }
        Shapes[i] = Shape;
    }
    problem Result = problem_createFromShapes(ShapeCount, Shapes);
    return Result;
}

typedef struct
{
    int Score;
    int RawScore;
    bool Overlap;
}score;


score calculateScore(problem *Problem, vector *Offsets)
{
    memset(Problem->Cells, 0, Problem->CellCount);
    for(int i = 0; i < Problem->ShapeCount; i++)
    {
        vector Offset = {0, 0};
        if(i > 0) Offset = Offsets[i - 1];
        Offset = vector_add(Offset, Problem->CenterOffset);
        uint8_t *TargetLine = Problem->Cells + (Offset.X + Offset.Y * Problem->CellWidth);
        shape Shape = Problem->Shapes[i];
        assert(Offset.X + Shape.Width <= Problem->CellWidth);
        assert(Offset.Y + Shape.Height <= Problem->CellHeight);
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
    Error = 10000 * Overlap;
    score Result =
    {
        .Score = Width * Height + Error,
        .RawScore = Width * Height,
        .Overlap = Overlap > 0
    };
#ifdef PRINT_ITERATIONS
    printf("%i, %i, %i\n", ScoreIteration++, Result.Score, Result.Overlap);
#endif
    return Result;
}


void genetic_optimize(problem *Problem, settings *Settings, individual *Current, individual *Next)
{
    assert(Settings->currentPopulationSize >= 2);
    assert(Settings->nextPopulationSize >= 1);
    Next[0].Score = INT_MAX;
    double InverseSum = 0;
    for(int i = 0; i < Settings->currentPopulationSize; i++)
    {   int Score = calculateScore(Problem, Current[i].Point).Score;
        Current[i].Score = Score;
        InverseSum += 1.0 / Score;
        if(Next[0].Score > Score)
            Next[0] = Current[i];
    }
    for(int i = 0; i < Settings->currentPopulationSize; i++)
        Current[i].Weight = 1.0 / (Current[i].Score * InverseSum);
    for(int NextIndex = 1; NextIndex < Settings->nextPopulationSize; NextIndex++)
    {
        individual Mother = individual_getRandomWeighted(Current, Settings->currentPopulationSize);
        individual Father = individual_getRandomWeighted(Current, Settings->currentPopulationSize);
        //TODO: check that mother != father
        typedef typeof_member(vector, X) atomType;
        int VectorSize = (Problem->ShapeCount - 1) * sizeof (vector);
        int Crossover = pcg32_range(1, VectorSize / sizeof(atomType));
        memcpy(Next[NextIndex].Point, Mother.Point, Crossover * sizeof(atomType));
        memcpy(((atomType *)Next[NextIndex].Point) + Crossover, 
               ((atomType *)Father.Point) + Crossover,
               VectorSize - Crossover * sizeof(atomType));
        int X = pcg32_range(Problem->MinOrigin.X, Problem->MaxOrigin.X + 1);
        int Y = pcg32_range(Problem->MinOrigin.Y, Problem->MaxOrigin.Y + 1);
        Next[NextIndex].Point[pcg32_range(0, Problem->ShapeCount - 1)] = (vector){X, Y};
    }
}

void printEvolution(problem *Problem, int PopulationSize, int Iterations)
{
    individual *Current = calloc(PopulationSize, sizeof (individual));
    individual *Next    = calloc(PopulationSize, sizeof (individual));
    for(int IndividualIndex = 0; IndividualIndex < PopulationSize; IndividualIndex++)
    {
        Current[IndividualIndex].Point = malloc((Problem->ShapeCount - 1) * sizeof (vector));
        Next[IndividualIndex].Point    = malloc((Problem->ShapeCount - 1) * sizeof (vector));
        for(int VectorIndex = 0; VectorIndex < Problem->ShapeCount - 1; VectorIndex++)
        {
            int X = pcg32_range(Problem->MinOrigin.X, Problem->MaxOrigin.X + 1);
            int Y = pcg32_range(Problem->MinOrigin.Y, Problem->MaxOrigin.Y + 1);
            Current[IndividualIndex].Point[VectorIndex] = (vector){X, Y};
        }
    }
    settings Settings =
    {
        .currentPopulationSize = PopulationSize,
        .nextPopulationSize = PopulationSize
    };
    while(ScoreIteration < Iterations)
    {
        genetic_optimize(Problem, &Settings, Current, Next);
        individual *Tmp = Current;
        Current = Next;
        Next = Tmp;
    }
}

void printGrid(problem *Problem)
{
    assert(Problem->ShapeCount == 2);
    printf("x, y, score\n");
    for(int y = Problem->MinOrigin.Y; y <= Problem->MaxOrigin.Y; y++)
    {
        for(int x = Problem->MinOrigin.X; x <= Problem->MaxOrigin.X; x++)
        {
            vector Point = {x, y};
            int score = calculateScore(Problem, &Point).Score;
            printf("%i, %i, %i\n", x, y, score);
        }
    }
}

void printRandom(problem *Problem, int Iterations)
{
    vector *Point = malloc((Problem->ShapeCount - 1) * sizeof (vector));
    for(int Iteration = 0; Iteration < Iterations; Iteration++)
    {
        for(int ShapeIndex = 1; ShapeIndex < Problem->ShapeCount; ShapeIndex++)
        {
            int X = pcg32_range(Problem->MinOrigin.X, Problem->MaxOrigin.X + 1);
            int Y = pcg32_range(Problem->MinOrigin.Y, Problem->MaxOrigin.Y + 1);
            Point[ShapeIndex - 1] = (vector){X, Y};
        }
        calculateScore(Problem, Point);
    }
}

int main()
{

#ifdef PRINT_ITERATIONS
    printf("iteration, score, overlap\n");
#endif

    pcg32_srandom(42u, 54u);
#if 0
    shape Shape0 = {2, 2, (uint8_t[]){1, 1, 1, 0}};
    shape Shape1 = {2, 2, (uint8_t[]){0, 0, 0, 1}};
#else
    shape Shape0 = {3, 4, (uint8_t[]){0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1}};
    shape Shape1 = {2, 4, (uint8_t[]){1, 1, 1, 1, 1, 0, 1, 1}};
#endif
    shape Shapes[] = {Shape0, Shape1};

    //problem Problem = problem_createFromShapes(ARRAY_COUNT(Shapes), Shapes);
    problem Problem = problem_createFromIndexes(Box_Width, Box_Height, Box);
    //printGrid(&Problem);
    //printRandom(&Problem, 20000);
    printEvolution(&Problem, 10, 20000);
    return 0;
}
#endif

int main()
{
    Problem problem = spritePacking_createProblemFromIndexes(Box1_Width, 
                                                             Box1_Height,
                                                             Box1);
    char buffer[512];
    for(int i = 0; i < 10; i++)
    {
        snprintf(buffer, 512, "data/Score/genetic_%i.csv", i);
        FILE *scoreFile = fopen(buffer, "w");
        snprintf(buffer, 512, "data/Result/genetic_%i.csv", i);
        FILE *bestResultFile = fopen(buffer, "w");
        GeneticSettings settings =
        {
            .scoreFile = scoreFile,
            .bestResultFile = bestResultFile,
            .maxIteration = 15000,
            .populationSize = 100,
            .eliteCount = 5,
            .mutationRate = 0.01,
            .mutationDistance = 0.1
        };
        fprintf(scoreFile, "iterations,populationSize,eliteCount,muationRate,"
                           "mutationDistance\n");
        fprintf(scoreFile, "%li, %i, %i, %f, %f\n", 
                settings.maxIteration, settings.populationSize,
                settings.eliteCount, settings.mutationRate,
                settings.mutationDistance);
        genetic_run(&problem, &settings);
        fclose(scoreFile);
        fclose(bestResultFile);
    }
    system("python3 source/graph.py data/Score/ -o 100");
    system("python3 source/graph.py data/Result/ -t image");

    return 0;
}
