#if !defined(HANDMADE_MATH_H)
#define HANDMADE_MATH_H

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
        real32 ignored0_;
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

struct rectangle2i
{
    int32 MinX;
    int32 MinY;
    int32 MaxX;
    int32 MaxY;
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

inline v4
SRGB255_ToLinear1(v4 color)
{
    v4 result;

    real32 inv255 = 1.0f / 255.0f;

    result.r = Square(color.r * inv255);
    result.g = Square(color.g * inv255);
    result.b = Square(color.b * inv255);
    result.a = color.a * inv255;
    
    return result;
}

inline v4
Linear_1_ToSRGB255(v4 color)
{
    v4 result;

    real32 val255 = 255.0f;
    
    result.r = SquareRoot(color.r) * val255;
    result.g = SquareRoot(color.g) * val255;
    result.b = SquareRoot(color.b) * val255;
    result.a = color.a * val255;

    return result;
}


#include "handmade_math_v2.h"
#include "handmade_math_v3.h"
#include "handmade_math_v4.h"
#include "handmade_math_rectangle2.h"
#include "handmade_math_rectangle3.h"


#endif
