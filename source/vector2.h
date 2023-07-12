#ifndef VECTOR2_H
#define VECTOR2_H

typedef struct
{
    union
    {
        struct
        {
            int x;
            int y;
        };
        struct
        {
            float r;
            float phi;
        };
        int i[2];
    };
}Vector2;

Vector2 vector2_add(Vector2 a, Vector2 b)
{
    Vector2 result = {.x = a.x + b.x, 
                      .y = a.y + b.y};
    return result;
}

Vector2 vector2_sub(Vector2 a, Vector2 b)
{
    Vector2 result = {.x = a.x - b.x, 
                      .y = a.y - b.y};
    return result;
}

Vector2 vector2_min(Vector2 a, Vector2 b)
{
    Vector2 result = {.x = MIN(a.x, b.x), 
                      .y = MIN(a.y, b.y)};
    return result;
}

Vector2 vector2_max(Vector2 a, Vector2 b)
{
    Vector2 result = {.x = MAX(a.x, b.x), 
                      .y = MAX(a.y, b.y)};
    return result;
}

#endif
