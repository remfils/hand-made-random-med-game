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

#define HANDMADE_MATH_H
#endif
