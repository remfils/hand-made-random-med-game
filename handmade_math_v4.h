#if !defined(HANDMADE_MATH_V4_H)
#define HANDMADE_MATH_V4_H

////////////////////////////////////////////////////////////////////////////////////////////////////
// convertions
////////////////////////////////////////////////////////////////////////////////////////////////////


inline v4
ToV4(real32 x, real32 y,real32 z, real32 w)
{
    v4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}

inline v4
ToV4(v3 a, real32 w=0.0f)
{
    v4 res;
    res.x = a.x;
    res.y = a.y;
    res.z = a.z;
    res.w = w;
    return res;
}

inline v4
ToV4(v2 a, real32 z=0.0f, real32 w=0.0f)
{
    v4 res;
    res.x = a.x;
    res.y = a.y;
    res.z = z;
    res.w = w;
    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// operators
////////////////////////////////////////////////////////////////////////////////////////////////////

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
operator+=(v4 &b, v4 a)
{
    b = b + a;
    return b;
}

inline v4
operator-(v4 a, v4 b)
{
    v4 res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    res.z = a.z - b.z;
    res.w = a.w - b.w;
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


////////////////////////////////////////////////////////////////////////////////////////////////////
// functions
////////////////////////////////////////////////////////////////////////////////////////////////////

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

#endif
