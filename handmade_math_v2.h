////////////////////////////////////////////////////////////////////////////////////////////////////
// convertions
////////////////////////////////////////////////////////////////////////////////////////////////////

inline v2
ToV2(real32 x, real32 y)
{
    v2 res;
    res.x = x;
    res.y = y;
    return res;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// operators
////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// functions
////////////////////////////////////////////////////////////////////////////////////////////////////

inline v2
Perp(v2 v)
{
    v2 result = ToV2(-v.y, v.x);
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
