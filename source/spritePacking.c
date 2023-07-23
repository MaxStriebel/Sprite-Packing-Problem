#include "vector2.h"
#include "problem.h"
#include "pcg_basic.h"

typedef enum
{
    POS_ABSOLUTE_CARTESIAN,
    POS_ABSOLUTE_DIRECTION
}PositionEncoding;

typedef enum
{
    ENC_INT,
    //ECN_GREY
}EncodingType;

typedef struct
{
    Vector2 dim;
    uint8_t *cells;
}Sprite;

typedef struct
{
    PositionEncoding positionEncoding;
    EncodingType encodingType;
    int spriteCount;
    Sprite *sprites;
    
    Vector2 bounds;
    int cellCount;
    uint8_t *cells;
}SpritePacking;

typedef struct
{
    int index;
    Vector2 position;
    float direction; //[0, 1]
}Chromosom;

static bool containsIndex(Chromosom *chromosom, int segment[2], int index)
{
    for(int i = segment[0]; i <= segment[1]; i++)
        if(chromosom[i].index == index)
            return true;
    return false;
}

static int findIndex(Chromosom *chromosom, int spriteCount, int index)
{
    for(int i = 0; i < spriteCount; i++)
        if(chromosom[i].index == index)
            return i;
    return -1;
}

static void testIndecies(Chromosom *chromosom, int spriteCount)
{
    for(int i = 0; i < spriteCount; i++)
        assert(findIndex(chromosom, spriteCount, i) >= 0);
}

void spritePacking_initializeChromosom(Problem *problem, void *chromosomData)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    Chromosom *chromosom = (Chromosom *)chromosomData;
    for(int spriteIndex = 0; spriteIndex < packer->spriteCount; spriteIndex++)
    {
        Vector2 bounds = packer->bounds;
        Vector2 spriteSize = packer->sprites[spriteIndex].dim;
        chromosom[spriteIndex] = (Chromosom)
        {
            .index = spriteIndex,
            .position.x = pcg32_range(0, bounds.x - spriteSize.x),
            .position.y = pcg32_range(0, bounds.y - spriteSize.y),
            .direction = pcg32_fraction(),
        };
    }
    if(packer->positionEncoding == POS_ABSOLUTE_DIRECTION)
    {
        for(int i = 0; i < packer->spriteCount - 1; i++)
        {
            int j = pcg32_range(i, packer->spriteCount);
            Chromosom tmp = chromosom[i];
            chromosom[i] = chromosom[j];
            chromosom[j] = tmp;
        }
    }
}

static void orderCrossover(Chromosom *child, 
                           Chromosom *mother, 
                           Chromosom *father, 
                           int segment[2],
                           int spriteCount)
{
    memcpy(&child[segment[0]], &mother[segment[0]], 
            sizeof(Chromosom[segment[1] - segment[0] + 1]));
    int fatherIndex = 0;
    for(int childIndex = 0; childIndex < segment[0]; childIndex++)
    {
        while(containsIndex(child, segment, father[fatherIndex].index))
            fatherIndex++;
        assert(fatherIndex < spriteCount);
        child[childIndex] = father[fatherIndex];
        fatherIndex++;
    }
    for(int childIndex = segment[1] + 1; childIndex < spriteCount; childIndex++)
    {
        while(containsIndex(child, segment, father[fatherIndex].index))
            fatherIndex++;
        assert(fatherIndex < spriteCount);
        child[childIndex] = father[fatherIndex];
        fatherIndex++;
    }
    testIndecies(child, spriteCount);
}

void spritePacking_crossover(Problem *problem, 
               void *motherData, void *fatherData, 
               void *child0Data, void *child1Data)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    Chromosom *mother = (Chromosom *)motherData;
    Chromosom *father = (Chromosom *)fatherData;
    Chromosom *child0 = (Chromosom *)child0Data;
    Chromosom *child1 = (Chromosom *)child1Data;

    if(packer->positionEncoding == POS_ABSOLUTE_CARTESIAN) 
    {   
        //Single crossover point without reordering 
        int crossover = pcg32_boundedrand(packer->spriteCount);
        memcpy(child0, mother, sizeof(Chromosom[crossover]));
        memcpy(&child0[crossover], &father[crossover], 
                sizeof(Chromosom[packer->spriteCount - crossover]));
        memcpy(child1, father, sizeof(Chromosom[crossover]));
        memcpy(&child1[crossover], &mother[crossover], 
                sizeof(Chromosom[packer->spriteCount - crossover]));
    }
    else if(packer->positionEncoding == POS_ABSOLUTE_DIRECTION)
    {
        //Single segment order crossover
        int p0 = pcg32_boundedrand(packer->spriteCount);
        int p1 = pcg32_boundedrand(packer->spriteCount);
        int segment[2] = 
        {
            MIN(p0, p1),
            MAX(p0, p1),
        };
        orderCrossover(child0, mother, father, segment, packer->spriteCount);
        orderCrossover(child1, father, mother, segment, packer->spriteCount);
    }
    else
        assert(false);
}

void spritePacking_mutate(Problem *problem, 
                          float mutationRate, 
                          float mutationDistance, 
                          void *chromosomData)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    Chromosom *chromosom = (Chromosom *)chromosomData;
    for(int spriteIndex = 0; spriteIndex < packer->spriteCount; spriteIndex++)
    {
        for(int dimension = 0; dimension < 2; dimension++)
        {
            if(pcg32_fraction() <= mutationRate)
            {
                if(packer->positionEncoding == POS_ABSOLUTE_CARTESIAN)
                {
                    int bounds = packer->bounds.i[dimension];
                    int maxDistance = bounds * mutationDistance;
                    int change =  pcg32_range(-maxDistance, maxDistance + 1);
                    int *value = &chromosom[spriteIndex].position.i[dimension];
                    int newValue = *value + change;
                    int spriteSize = packer->sprites[spriteIndex].dim.i[dimension];
                    *value = CLAMP(newValue, 0, bounds - spriteSize - 1);
                }
                else if(packer->positionEncoding == POS_ABSOLUTE_DIRECTION)
                {
                    if(dimension == 0)
                    {
                        float change = (pcg32_fraction() - 0.5) * 2 * mutationDistance;
                        float *value = &chromosom[spriteIndex].direction;
                        *value = CLAMP(*value + change, 0, 1);
                    }
                    else
                    {
                        int p0 = pcg32_boundedrand(packer->spriteCount);
                        int p1 = pcg32_boundedrand(packer->spriteCount);
                        Chromosom tmp = chromosom[p0];
                        chromosom[p0] = chromosom[p1];
                        chromosom[p1] = tmp;
                    }
                }
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

bool doesSpriteFit(SpritePacking *packer, Sprite sprite, int xOffset, int yOffset)
{
    assert(xOffset >= 0);
    assert(yOffset >= 0);
    assert(xOffset + sprite.dim.x < packer->bounds.x);
    assert(yOffset + sprite.dim.y < packer->bounds.y);

    uint8_t *targetLine = packer->cells + (xOffset + yOffset * packer->bounds.x);
    uint8_t *sourceCell = sprite.cells;
    for(int y = 0; y < sprite.dim.y; y++)
    {
        uint8_t *targetCell = targetLine;
        for(int x = 0; x < sprite.dim.x; x++)
        {
            if(*targetCell && *sourceCell)
                return false;
            targetCell++;
            sourceCell++;
        }
        targetLine += packer->bounds.x;
    }
    return true;
}

void blitSprite(SpritePacking *packer, Sprite sprite, int xOffset, int yOffset)
{
    assert(xOffset >= 0);
    assert(yOffset >= 0);
    assert(xOffset + sprite.dim.x < packer->bounds.x);
    assert(yOffset + sprite.dim.y < packer->bounds.y);

    uint8_t *targetLine = packer->cells + (xOffset + yOffset * packer->bounds.x);
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

static void calculatePositions(SpritePacking *packer, Chromosom *chromosom)
{
    for(int i = 0; i < packer->spriteCount; i++)
    {
        int index = findIndex(chromosom, packer->spriteCount, i);
        assert(index >= 0);
        float direction = chromosom[index].direction;
        assert(direction >= 0);
        assert(direction <= 1);
        Sprite sprite = packer->sprites[index];
        Vector2 bounds = vector2_sub(packer->bounds, sprite.dim);
        bool horizontal = direction < 0.5;
        int dx, dy;
        if(horizontal)
        {
            dx = packer->bounds.x;
            dy = direction * 2 * packer->bounds.y;
        }
        else
        {
            dx = packer->bounds.y;
            dy = (1 - direction) * 2 * packer->bounds.x;
        }
        int y = 0;
        int D = 2 * dy - dx;
        for(int x = 0; x < dx; x++)
        {
            int realX = horizontal ? x : y;
            int realY = horizontal ? y : x;
            if(doesSpriteFit(packer, sprite, realX, realY) || x + 1 == dx)
            {
                chromosom[index].position = (Vector2){.x = realX, .y = realY};
                blitSprite(packer, sprite, realX, realY);
                break;
            }
            if(D > 0)
            {
                y++;
                D -= 2 * dx;
            }
            D += 2 * dy;
        }
    }
}

Score spritePacking_calculateScore(Problem *problem, void *chromosomData)
{
    SpritePacking *packer = (SpritePacking *)problem->data;
    Chromosom *chromosom = (Chromosom *)chromosomData;
    memset(packer->cells, 0, packer->cellCount);
    if(packer->positionEncoding == POS_ABSOLUTE_CARTESIAN)
    {
        for(int i = 0; i < packer->spriteCount; i++)
        {
            Vector2 position = chromosom[i].position;
            Sprite sprite = packer->sprites[i];
            blitSprite(packer, sprite, position.x, position.y);
        }
    }
    else
        calculatePositions(packer, chromosom);
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
    int width = maxX - minX + 1;
    int height = maxY - minY + 1;
    int error = MIN(width, height) * overlap;
    error = width * height * overlap;
    Score Result =
    {
        .score = width * height + error,
        .rawScore = width * height,
        .overlap = overlap
    };
    return Result;
}

void spritePacking_printChromosom(Problem *problem, void *chromosomData, FILE *file)
{
    assert(file);
    SpritePacking *packer = (SpritePacking *)problem->data;
    Chromosom *chromosom = (Chromosom *)chromosomData;
    fprintf(file, "x,y,index\n");
    for(int i = 0; i < packer->spriteCount; i++)
    {
        Vector2 pos = chromosom[i].position;
        Sprite sprite = packer->sprites[i];
        for(int y = 0; y < sprite.dim.y; y++)
        {
            for(int x = 0; x < sprite.dim.x; x++)
            {
                if(sprite.cells[y * sprite.dim.x + x])
                    fprintf(file, "%i, %i, %i\n", x + pos.x, y + pos.y, i);
            }
        }
    }
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
        .positionEncoding = POS_ABSOLUTE_CARTESIAN,
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
        .chromosomSize = sizeof(Chromosom[packing->spriteCount]),
        .initializeChromosom = spritePacking_initializeChromosom,
        .calculateScore = spritePacking_calculateScore,
        .crossover = spritePacking_crossover,
        .mutate = spritePacking_mutate,
        .printChromosom = spritePacking_printChromosom
    };
    return problem;
}
