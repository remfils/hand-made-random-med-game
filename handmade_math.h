#if !defined(HANDMADE_MATH_H)

union v2
{
    struct {
        real32 X, Y;
    };
    real32 E[2];
};

inline v2
V2(real32 x, real32 y)
{
    v2 res;
    res.X = x;
    res.Y = y;
    return res;
}

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

struct rectangle2
{
    v2 Min;
    v2 Max;
};

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

#define HANDMADE_MATH_H
#endif
