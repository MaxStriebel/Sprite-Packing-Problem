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
#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof(Array[0]))

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
    //Error = 5 * Overlap;
    score Result =
    {
        .Score = Width * Height + Error,
        .RawScore = Width * Height,
        .Overlap = Overlap > 0
    };
    return Result;
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

int pcg32_range_r(pcg32_random_t *Rng, int min, int max)
{
    int Result = pcg32_boundedrand_r(Rng, max - min) + min;
    return Result;
}

void printRandom(problem *Problem, int Iterations, pcg32_random_t *Rng)
{
    printf("iteration, score, overlap\n");
    vector *Point = malloc((Problem->ShapeCount - 1) * sizeof (vector));
    for(int Iteration = 0; Iteration < Iterations; Iteration++)
    {
        for(int ShapeIndex = 1; ShapeIndex < Problem->ShapeCount; ShapeIndex++)
        {
            int X = pcg32_range_r(Rng, Problem->MinOrigin.X, Problem->MaxOrigin.X + 1);
            int Y = pcg32_range_r(Rng, Problem->MinOrigin.Y, Problem->MaxOrigin.Y + 1);
            Point[ShapeIndex - 1] = (vector){X, Y};
        }
        score Score = calculateScore(Problem, Point);
        printf("%i, %i, %i\n", Iteration, Score.Score, Score.Overlap);
    }
}

int main()
{
    pcg32_random_t Rng;
    pcg32_srandom_r(&Rng, 42u, 54u);
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
    printRandom(&Problem, 20000, &Rng);
    return 0;
    for(int i = 0; i < 10; i++)
        printf("random: 0x%X\n", pcg32_random_r(&Rng));
    return 0;
}
