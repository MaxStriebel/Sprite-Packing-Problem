#include "vector2.h"
#include "problem.h"
#include "pcg_basic.h"

typedef enum
{
    POS_POLAR, //Default Cartesian
    POS_RELATIVE //Default Absolute
}PositionFlags;

typedef enum
{
    ENC_INT,
    ECN_GREY
}EncodingType;

typedef struct
{
    Vector2 dim;
    uint8_t *cells;
}Sprite;

typedef struct
{
    PositionFlags positionFlags;
    EncodingType encodingType;
    int spriteCount;
    Sprite *sprites;
    
    Vector2 bounds;
    int cellCount;
    uint8_t *cells;
}SpritePacking;


typedef struct
{
    Vector2 *positions;
    int *tree;
}Chromosom;


Chromosom getChromosom(void *data, int spriteCount)
{
    Chromosom result = 
    {
        .positions = (Vector2 *)data,
        .tree      = (int *)((Vector2 *)data + spriteCount)
    };
    return result;
}

void spritePacking_initializeChromosom(Problem *problem, void *chromosomData)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    assert((packer->positionFlags & (POS_POLAR | POS_RELATIVE)) == 0);
    Chromosom chromosom = getChromosom(chromosomData, packer->spriteCount);
    for(int spriteIndex = 0; spriteIndex < packer->spriteCount; spriteIndex++)
    {
        for(int dimension = 0; dimension < 2; dimension++)
        {
            int bounds = packer->bounds.i[dimension];
            int spriteSize = packer->sprites[spriteIndex].dim.i[dimension];
            int *value = &chromosom.positions[spriteIndex].i[dimension];
            *value = pcg32_range(0, bounds - spriteSize);
        }
    }
}

void spritePacking_crossover(Problem *problem, 
               void *motherData, void *fatherData, 
               void *child0Data, void *child1Data)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    Chromosom mother = getChromosom(motherData, packer->spriteCount);
    Chromosom father = getChromosom(fatherData, packer->spriteCount);
    Chromosom child0 = getChromosom(child0Data, packer->spriteCount);
    Chromosom child1 = getChromosom(child1Data, packer->spriteCount);

    if(packer->positionFlags & POS_RELATIVE)
    {
        //TODO: convert to relative positions and crossover tree
    }
    else
    {
        int Crossover = pcg32_boundedrand(packer->spriteCount);
        memcpy(child0.positions, mother.positions, Crossover * sizeof(Vector2));
        memcpy(child0.positions + Crossover, father.positions + Crossover,
                (packer->spriteCount - Crossover) * sizeof(Vector2));
        memcpy(child1.positions, father.positions, Crossover * sizeof(Vector2));
        memcpy(child1.positions + Crossover, mother.positions + Crossover,
                (packer->spriteCount - Crossover) * sizeof(Vector2));
    }
}

void spritePacking_mutate(Problem *problem, 
                          float mutationRate, 
                          float mutationDistance, 
                          void *chromosomData)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    assert((packer->positionFlags & (POS_POLAR | POS_RELATIVE)) == 0);
    Chromosom chromosom = getChromosom(chromosomData, packer->spriteCount);
    for(int spriteIndex = 0; spriteIndex < packer->spriteCount; spriteIndex++)
    {
        for(int dimension = 0; dimension < 2; dimension++)
        {
            if(pcg32_fraction() <= mutationRate)
            {
                int bounds = packer->bounds.i[dimension];
                int maxDistance = bounds * mutationDistance;
                int change =  pcg32_range(-maxDistance, maxDistance + 1);
                int *value = &chromosom.positions[spriteIndex]
                                       .i[dimension];
                int newValue = *value + change;
                int spriteSize = packer->sprites[spriteIndex].dim.i[dimension];
                *value = CLAMP(newValue, 0, bounds - spriteSize - 1);
            }
        }
    }
}

Sprite shape_allocate(int width, int height)
{
    Sprite result =
    {
        .dim =
        {
            .x= width,
            .y = height,
        },
        .cells = calloc(width * height, sizeof (result.cells[0]))
    };
    return result;
}

Score spritePacking_calculateScore(Problem *problem, void *chromosomData)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    assert((packer->positionFlags & (POS_POLAR | POS_RELATIVE)) == 0);
    Chromosom chromosom = getChromosom(chromosomData, packer->spriteCount);
    memset(packer->cells, 0, packer->cellCount);
    for(int i = 0; i < packer->spriteCount; i++)
    {
        Vector2 position = chromosom.positions[i];
        Sprite sprite = packer->sprites[i];
        assert(position.x >= 0);
        assert(position.y >= 0);
        assert(position.x + sprite.dim.x < packer->bounds.x);
        assert(position.y + sprite.dim.y < packer->bounds.y);

        uint8_t *targetLine = packer->cells + 
                             (position.x + position.y * packer->bounds.x);
        uint8_t *sourceCell = sprite.cells;
        for(int y = 0; y < sprite.dim.y; y++)
        {
            uint8_t *targetCell = targetLine;
            for(int x = 0; x < sprite.dim.x; x++)
            {
                *targetCell += *sourceCell;
                targetCell++;
                sourceCell++;
            }
            targetLine += packer->bounds.x;
        }
    }
    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;
    int overlap = 0;
    uint8_t *cell = packer->cells;
    for(int y = 0; y < packer->bounds.y; y++)
    {
        for(int x = 0; x < packer->bounds.x; x++)
        {
            if(*cell)
            {
                minX = MIN(minX, x);
                minY = MIN(minY, y);
                maxX = MAX(maxX, x);
                maxY = MAX(maxY, y);
                overlap += *cell - 1;
            }
            cell++;
        }
    }
    int width = maxX - minX;
    int height = maxY - minY;
    int error = MIN(width, height) * overlap;
    //error = width * height * overlap;
    Score Result =
    {
        .score = width * height + error,
        .rawScore = width * height,
        .overlap = overlap
    };
    return Result;
}

SpritePacking *spritePacking_createFromShapes(int spriteCount, Sprite *sprites)
{
    assert(spriteCount > 0);
    int totalWidth = 0;
    int totalHeight = 0;
    int maxWidth = 0;
    int maxHeight = 0;
    for(int i = 0; i < spriteCount; i++)
    {
        totalWidth += sprites[i].dim.x;
        totalHeight += sprites[i].dim.y;
        maxWidth = MAX(maxWidth, sprites[i].dim.x);
        maxHeight = MAX(maxHeight, sprites[i].dim.y);
    }
    int cellWidth = totalWidth;
    int cellHeight = totalHeight;
    int cellCount = cellWidth * cellHeight;
    SpritePacking *result = malloc(sizeof (SpritePacking));
    *result = (SpritePacking)
    {
        .encodingType = ENC_INT,
        .spriteCount = spriteCount,
        .sprites = sprites,
        .bounds.x = cellWidth,
        .bounds.y = cellHeight,
        .cellCount = cellCount,
        .cells = calloc(cellCount, sizeof (result->cells[0]))
    };
    return result;
}

SpritePacking *spritePacking_createFromIndexes(int width, 
                                             int height, 
                                             uint8_t *indexes)
{
    int maxIndex = 0;
    for(int i = 0; i < width * height; i++)
        maxIndex = MAX(maxIndex, indexes[i]);
    int spriteCount = maxIndex + 1;
    Vector2 *minShapes = malloc(spriteCount * sizeof (Vector2));
    Vector2 *maxShapes = malloc(spriteCount * sizeof (Vector2));
    for(int i = 0; i < spriteCount; i++)
    {
        minShapes[i] = (Vector2){.x = INT_MAX, .y = INT_MAX};
        maxShapes[i] = (Vector2){.x = INT_MIN, .y = INT_MIN};
    }
    uint8_t *index = indexes;
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            assert(*index >= 0);
            assert(*index < spriteCount);
            minShapes[*index] = vector2_min(minShapes[*index], (Vector2){.x = x, .y = y});
            maxShapes[*index] = vector2_max(maxShapes[*index], (Vector2){.x = x, .y = y});
            index++;
        }
    }
    Sprite *sprites = malloc(spriteCount * sizeof (Sprite));
    for(int i = 0; i < spriteCount; i++)
    {
        Vector2 size = vector2_sub(maxShapes[i], minShapes[i]);
        Sprite sprite = shape_allocate(size.x + 1, size.y + 1);
        uint8_t *cell = sprite.cells;
        for(int y = minShapes[i].y; y <= maxShapes[i].y; y++)
        {
            for(int x = minShapes[i].x; x <= maxShapes[i].x; x++)
            {
                *cell = indexes[x + y * width] == i;
                cell++;
            }
        }
        sprites[i] = sprite;
    }
    SpritePacking *result = spritePacking_createFromShapes(spriteCount, sprites);
    return result;
}

Problem spritePacking_createProblemFromIndexes(int width, int height, uint8_t *indexes)
{
    SpritePacking *packing = spritePacking_createFromIndexes(width, height, indexes);
    Problem problem = 
    {
        .data = packing,
        .chromosomSize = (sizeof (Vector2) + sizeof (int)) * packing->spriteCount,
        .initializeChromosom = spritePacking_initializeChromosom,
        .calculateScore = spritePacking_calculateScore,
        .crossover = spritePacking_crossover,
        .mutate = spritePacking_mutate
    };
    return problem;
}
