#if !defined(HANDMADE_MATH_H)

union v2
{
    struct {
        real32 X, Y;
    };
    real32 E[2];
};
union v3
{
    struct {
        real32 X, Y, Z;
    };
    struct {
        real32 R, G, B;
    };
    struct {
        v2 XY;
        real32 Ignore_;
    };
    real32 E[3];
};
union v4
{
    struct {
        real32 X, Y, Z, W;
    };
    struct {
        real32 R, G, B, A;
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
    res.X = x;
    res.Y = y;
    return res;
}
inline v3
V3(real32 x, real32 y, real32 z)
{
    v3 res;
    res.X = x;
    res.Y = y;
    res.Z = z;
    return res;
}

inline v3
V3(v2 xy, real32 z)
{
    v3 res;
    res.X = xy.X;
    res.Y = xy.Y;
    res.Z = z;
    return res;
}

inline v4
V4(real32 x, real32 y,real32 z, real32 w)
{
    v4 res;
    res.X = x;
    res.Y = y;
    res.Z = z;
    res.W = w;
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
operator-(v2 a)
{
    v2 res;
    res.X = -a.X;
    res.Y = -a.Y;
    return res;
}

inline v2
operator+(v2 a, v2 b)
{
    v2 res;
    res.X = a.X + b.X;
    res.Y = a.Y + b.Y;
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
    res.X = a.X - b.X;
    res.Y = a.Y - b.Y;
    return res;
}

inline v2
operator*(real32 a, v2 b)
{
    v2 res;
    res.X = b.X * a;
    res.Y = b.Y * a;
    return res;
}

inline v2
operator*(v2 b, real32 a)
{
    v2 res;
    res.X = b.X * a;
    res.Y = b.Y * a;
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
    real32 res = a.X * b.X + a.Y * b.Y;
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
    result.X = a.X * b.X;
    result.Y = a.Y * b.Y;
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// v3 operations
////////////////////////////////////////////////////////////////////////////////////////////////////

inline v3
operator-(v3 a)
{
    v3 res;
    res.X = -a.X;
    res.Y = -a.Y;
    res.Z = -a.Z;
    return res;
}

inline v3
operator+(v3 a, v3 b)
{
    v3 res;
    res.X = a.X + b.X;
    res.Y = a.Y + b.Y;
    res.Z = a.Z + b.Z;
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
    res.X = a.X - b.X;
    res.Y = a.Y - b.Y;
    res.Z = a.Z - b.Z;
    return res;
}

inline v3
operator*(real32 a, v3 b)
{
    v3 res;
    res.X = b.X * a;
    res.Y = b.Y * a;
    res.Z = b.Z * a;
    return res;
}

inline v3
operator*(v3 b, real32 a)
{
    v3 res;
    res.X = b.X * a;
    res.Y = b.Y * a;
    res.Z = b.Z * a;
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
    real32 res = a.X * b.X + a.Y * b.Y + a.Z * b.Z;
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
Hadamard(v3 a, v3 b)
{
    v3 result;
    result.X = a.X * b.X;
    result.Y = a.Y * b.Y;
    result.Z = a.Z * b.Z;
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
    bool32 result = ((test.X >= rect.Min.X) &&
        (test.Y >= rect.Min.Y) &&
        (test.X < rect.Max.X) &&
        (test.Y < rect.Max.Y));

    return result;
}

inline v2
GetBarycentric(rectangle2 a, v2 p)
{
    v2 result;

    result.X = SafeRatio_0(p.X - a.Min.X, a.Max.X - a.Min.X);
    result.Y = SafeRatio_0(p.Y - a.Min.Y, a.Max.Y - a.Min.Y);

    return result;
}

inline v2
Clamp01(v2 value)
{
    v2 result;
    result.X = Clamp01(value.X);
    result.Y = Clamp01(value.Y);
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
        (test.X >= rect.Min.X)
        && (test.Y >= rect.Min.Y)
        && (test.Z >= rect.Min.Z)
        
        && (test.X < rect.Max.X)
        && (test.Y < rect.Max.Y)
        && (test.Z < rect.Max.Z)
        );

    return result;
}

inline bool32
RectanglesIntersect(rectangle3 a, rectangle3 b)
{
    // NOTE: this is double checked - now works
    
    bool32 result = !(
                     (b.Max.X <= a.Min.X) ||
                     (b.Min.X >= a.Max.X) ||
                     
                     (b.Max.Y <= a.Min.Y) ||
                     (b.Min.Y >= a.Max.Y) ||
                     
                     (b.Max.Z <= a.Min.Z) ||
                     (b.Min.Z >= a.Max.Z)
                      );

    return result;

}

inline v3
GetBarycentric(rectangle3 a, v3 p)
{
    v3 result;

    result.X = SafeRatio_0(p.X - a.Min.X, a.Max.X - a.Min.X);
    result.Y = SafeRatio_0(p.Y - a.Min.Y, a.Max.Y - a.Min.Y);
    result.Z = SafeRatio_0(p.Z - a.Min.Z, a.Max.Z - a.Min.Z);

    return result;
}

inline v3
Clamp01(v3 value)
{
    v3 result;
    result.X = Clamp01(value.X);
    result.Y = Clamp01(value.Y);
    result.Z = Clamp01(value.Z);
    
    return result;
}


#define HANDMADE_MATH_H
#endif
