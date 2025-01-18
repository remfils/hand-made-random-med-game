#if !defined(HANDMADE_MATH_V3_H)
#define HANDMADE_MATH_V3_H


////////////////////////////////////////////////////////////////////////////////////////////////////
// convertions
////////////////////////////////////////////////////////////////////////////////////////////////////

inline v3
ToV3(real32 x, real32 y, real32 z)
{
    v3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

inline v3
ToV3(v2 xy, real32 z=0.0f)
{
    v3 res;
    res.x = xy.x;
    res.y = xy.y;
    res.z = z;
    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// operators
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// functions
////////////////////////////////////////////////////////////////////////////////////////////////////

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

#endif
