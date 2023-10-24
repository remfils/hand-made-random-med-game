#if !defined(HANDMADE_MATH_H)

union v2
{
    struct {
        real32 x, y;
    };
    real32 E[2];
};

union v3
{
    struct {
        real32 x, y, z;
    };
    struct {
        real32 r, g, b;
    };
    struct {
        v2 xy;
        real32 Ignore_;
    };
    real32 E[3];
};

union v4
{
    struct {
        real32 x, y, z, w;
    };
    struct {
        real32 r, g, b, a;
    };
    struct {
        v3 xyz;
        real32 w;
    };
    struct {
        v2 xy;
        real32 ignored1_;
        real32 ignored2_;
    };
    struct {
        v3 rgb;
        real32 a;
    };
    real32 E[4];
};

struct rectangle2
{
    v2 Min;
    v2 Max;
};

struct rectangle3
{
    v3 Min;
    v3 Max;
};



inline real32
Square(real32 a)
{
    real32 result = a * a;
    return result;
}

inline real32
Lerp(real32 a, real32 t, real32 b)
{
    real32 result = (1.0f - t) * a + t * b;
    return result;
}

inline real32
Clamp(real32 min, real32 value, real32 max)
{
    real32 result = value;
    if (result < min) {
        result = min;
    }
    if (result > max) {
        result = max;
    }
    return result;
}

inline real32
Clamp01(real32 value)
{
    real32 result = Clamp(0, value, 1);
    return result;
}

inline real32
Clamp01MapToRange(real32 min, real32 t, real32 max)
{
    real32 range = max - min;
    real32 result = 0;
    if (range != 0) {
        result = Clamp01((t - min) / range);
    }
    return result;
}



inline real32
SafeRatio_N(real32 numerator, real32 divisor, real32 n)
{
    real32 result = n;

    if (divisor != 0) {
        result = numerator / divisor;
    }

    return result;
}

inline real32
SafeRatio_0(real32 numerator, real32 divisor)
{
    real32 result = SafeRatio_N(numerator, divisor, 0.0f);
    return result;
}

inline real32
SafeRatio_1(real32 numerator, real32 divisor)
{
    real32 result = SafeRatio_N(numerator, divisor, 1.0f);
    return result;
}


#include "handmade_math_v2.h"
#include "handmade_math_v3.h"
#include "handmade_math_v4.h"
#include "handmade_math_rectangle2.h"
#include "handmade_math_rectangle3.h"


#define HANDMADE_MATH_H
#endif
