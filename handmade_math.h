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

inline v2
V2(real32 x, real32 y)
{
    v2 res;
    res.x = x;
    res.y = y;
    return res;
}
inline v3
V3(real32 x, real32 y, real32 z)
{
    v3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

inline v3
V3(v2 xy, real32 z)
{
    v3 res;
    res.x = xy.x;
    res.y = xy.y;
    res.z = z;
    return res;
}

inline v4
V4(real32 x, real32 y,real32 z, real32 w)
{
    v4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// v2 operations
////////////////////////////////////////////////////////////////////////////////////////////////////

inline v2
Perp(v2 v)
{
    v2 result = V2(-v.y, v.x);
    return result;
}

inline v2
V2i(int32 x, int32 y)
{
    v2 result = {(real32)x, (real32)y};
    return result;
}

inline v2
V2u(uint32 x, uint32 y)
{
    v2 result = {(real32)x, (real32)y};
    return result;
}

inline v2
operator-(v2 a)
{
    v2 res;
    res.x = -a.x;
    res.y = -a.y;
    return res;
}

inline v2
operator+(v2 a, v2 b)
{
    v2 res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    return res;
}

inline v2
operator+=(v2 &b, v2 a)
{
    b = b + a;
    return b;
}

inline v2
operator-(v2 a, v2 b)
{
    v2 res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    return res;
}

inline v2
operator*(real32 a, v2 b)
{
    v2 res;
    res.x = b.x * a;
    res.y = b.y * a;
    return res;
}

inline v2
operator*(v2 b, real32 a)
{
    v2 res;
    res.x = b.x * a;
    res.y = b.y * a;
    return res;
}

inline v2
operator*=(v2 &b, real32 a)
{
    b = b * a;
    return b;
}

inline real32
Inner(v2 a, v2 b)
{
    real32 res = a.x * b.x + a.y * b.y;
    return res;
}

inline real32
LengthSq(v2 a)
{
    real32 result = Inner(a, a);
    return result;
}

inline real32
Length(v2 a)
{
    real32 result = SquareRoot(LengthSq(a));
    return result;
}

inline v2
Hadamard(v2 a, v2 b)
{
    v2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// v3 operations
////////////////////////////////////////////////////////////////////////////////////////////////////

inline v3
operator-(v3 a)
{
    v3 res;
    res.x = -a.x;
    res.y = -a.y;
    res.z = -a.z;
    return res;
}

inline v3
operator+(v3 a, v3 b)
{
    v3 res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    res.z = a.z + b.z;
    return res;
}

inline v3
operator+=(v3 &b, v3 a)
{
    b = b + a;
    return b;
}

inline v3
operator-(v3 a, v3 b)
{
    v3 res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    res.z = a.z - b.z;
    return res;
}

inline v3
operator*(real32 a, v3 b)
{
    v3 res;
    res.x = b.x * a;
    res.y = b.y * a;
    res.z = b.z * a;
    return res;
}

inline v3
operator*(v3 b, real32 a)
{
    v3 res;
    res.x = b.x * a;
    res.y = b.y * a;
    res.z = b.z * a;
    return res;
}

inline v3
operator*=(v3 &b, real32 a)
{
    b = b * a;
    return b;
}

inline real32
Inner(v3 a, v3 b)
{
    real32 res = a.x * b.x + a.y * b.y + a.z * b.z;
    return res;
}

inline real32
LengthSq(v3 a)
{
    real32 result = Inner(a, a);
    return result;
}

inline real32
Length(v3 a)
{
    real32 result = SquareRoot(LengthSq(a));
    return result;
}

inline v3
Normalize(v3 a)
{
    v3 result = a * (1.0f / Length(a));
    return result;
}

inline v3
Hadamard(v3 a, v3 b)
{
    v3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// rectangle2
////////////////////////////////////////////////////////////////////////////////////////////////////

struct rectangle2
{
    v2 Min;
    v2 Max;
};


inline rectangle2
AddRadiusTo(rectangle2 rect, real32 radiusW, real32 radiusH)
{
    rectangle2 result;

    result.Min = rect.Min - V2(radiusW, radiusH);
    result.Max = rect.Max + V2(radiusW, radiusH);

    return result;
}

inline rectangle2
RectMinMax(v2 min, v2 max)
{
    rectangle2 rect;

    rect.Min = min;
    rect.Max = max;

    return rect;
}

inline rectangle2
RectCenterHalfDim(v2 center, v2 halfDim)
{
    rectangle2 rect;

    rect.Min = center - halfDim;
    rect.Max = center + halfDim;

    return rect;
}

inline rectangle2
RectCenterDim(v2 center, v2 dim)
{
    rectangle2 rect = RectCenterHalfDim(center, dim * 0.5f);
    return rect;
}

inline rectangle2
RectMinDim(v2 min, v2 dim)
{
    rectangle2 rect;

    rect.Min = min;
    rect.Max = min + dim;

    return rect;
}


inline v2 GetMinCorner(rectangle2 rect)
{
    v2 result = rect.Min;
    return result;
}

inline v2 GetMaxCorner(rectangle2 rect)
{
    v2 result = rect.Max;
    return result;
}

inline v2 GetCenter(rectangle2 rect)
{
    v2 result = 0.5f * (rect.Min + rect.Max);
    return result;
}

inline bool32
IsInRectangle(rectangle2 rect, v2 test)
{
    bool32 result = ((test.x >= rect.Min.x) &&
        (test.y >= rect.Min.y) &&
        (test.x < rect.Max.x) &&
        (test.y < rect.Max.y));

    return result;
}

inline v2
GetBarycentric(rectangle2 a, v2 p)
{
    v2 result;

    result.x = SafeRatio_0(p.x - a.Min.x, a.Max.x - a.Min.x);
    result.y = SafeRatio_0(p.y - a.Min.y, a.Max.y - a.Min.y);

    return result;
}

inline v2
Clamp01(v2 value)
{
    v2 result;
    result.x = Clamp01(value.x);
    result.y = Clamp01(value.y);
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// rectangle3
////////////////////////////////////////////////////////////////////////////////////////////////////

struct rectangle3
{
    v3 Min;
    v3 Max;
};


inline rectangle3
AddRadiusTo(rectangle3 rect, v3 dim)
{
    rectangle3 result;

    result.Min = rect.Min - dim;
    result.Max = rect.Max + dim;

    return result;
}

inline rectangle3
Offset(rectangle3 rect, v3 offset)
{
    rectangle3 result;

    result.Min = rect.Min + offset;
    result.Max = rect.Max + offset;

    return result;
}

inline rectangle3
RectMinMax(v3 min, v3 max)
{
    rectangle3 rect;

    rect.Min = min;
    rect.Max = max;

    return rect;
}

inline rectangle3
RectCenterHalfDim(v3 center, v3 halfDim)
{
    rectangle3 rect;

    rect.Min = center - halfDim;
    rect.Max = center + halfDim;

    return rect;
}

inline rectangle3
RectCenterDim(v3 center, v3 dim)
{
    rectangle3 rect = RectCenterHalfDim(center, dim * 0.5f);
    return rect;
}

inline rectangle3
RectMinDim(v3 min, v3 dim)
{
    rectangle3 rect;

    rect.Min = min;
    rect.Max = min + dim;

    return rect;
}


inline v3 GetMinCorner(rectangle3 rect)
{
    v3 result = rect.Min;
    return result;
}

inline v3 GetMaxCorner(rectangle3 rect)
{
    v3 result = rect.Max;
    return result;
}

inline v3 GetCenter(rectangle3 rect)
{
    v3 result = 0.5f * (rect.Min + rect.Max);
    return result;
}

inline bool32
IsInRectangle(rectangle3 rect, v3 test)
{
    bool32 result = (
        (test.x >= rect.Min.x)
        && (test.y >= rect.Min.y)
        && (test.z >= rect.Min.z)
        
        && (test.x < rect.Max.x)
        && (test.y < rect.Max.y)
        && (test.z < rect.Max.z)
        );

    return result;
}

inline bool32
RectanglesIntersect(rectangle3 a, rectangle3 b)
{
    // NOTE: this is double checked - now works
    
    bool32 result = !(
                     (b.Max.x <= a.Min.x) ||
                     (b.Min.x >= a.Max.x) ||
                     
                     (b.Max.y <= a.Min.y) ||
                     (b.Min.y >= a.Max.y) ||
                     
                     (b.Max.z <= a.Min.z) ||
                     (b.Min.z >= a.Max.z)
                      );

    return result;

}

inline v3
GetBarycentric(rectangle3 a, v3 p)
{
    v3 result;

    result.x = SafeRatio_0(p.x - a.Min.x, a.Max.x - a.Min.x);
    result.y = SafeRatio_0(p.y - a.Min.y, a.Max.y - a.Min.y);
    result.z = SafeRatio_0(p.z - a.Min.z, a.Max.z - a.Min.z);

    return result;
}

inline v3
Clamp01(v3 value)
{
    v3 result;
    result.x = Clamp01(value.x);
    result.y = Clamp01(value.y);
    result.z = Clamp01(value.z);
    
    return result;
}


inline v4
operator+(v4 a, v4 b)
{
    v4 res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    res.z = a.z + b.z;
    res.w = a.w + b.w;
    return res;
}

inline v4
operator*(v4 b, real32 a)
{
    v4 res;
    res.x = b.x * a;
    res.y = b.y * a;
    res.z = b.z * a;
    res.w = b.w * a;
    return res;
}

inline v4
operator*(real32 a, v4 b)
{
    v4 res;
    res.x = b.x * a;
    res.y = b.y * a;
    res.z = b.z * a;
    res.w = b.w * a;
    return res;
}

inline v4
Lerp(v4 a, real32 t, v4 b)
{
    v4 result = (1.0f - t) * a + t * b;
    return result;
}

inline v4
Hadamard(v4 a, v4 b)
{
    v4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

inline real32
Inner(v4 a, v4 b)
{
    real32 res = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return res;
}


inline real32
LengthSq(v4 a)
{
    real32 result = Inner(a, a);
    return result;
}

inline real32
Length(v4 a)
{
    real32 result = SquareRoot(LengthSq(a));
    return result;
}

inline v4
Normalize(v4 a)
{
    v4 result = a * (1.0f / Length(a));
    return result;
}


#define HANDMADE_MATH_H
#endif
