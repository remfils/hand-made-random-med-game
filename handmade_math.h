#if !defined(HANDMADE_MATH_H)

struct v2
{
    union {
        struct {
            real32 X, Y;
        };
        real32 E[2];
    };
    real32 &operator[](int idx) {return((&X)[idx]);}

    inline v2 &operator*=(real32 a);
    inline v2 &operator+=(v2 a);
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

inline v2 &v2::
operator+=(v2 a)
{
    *this = *this + a;
    return *this;
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

inline v2 &v2::
operator*=(real32 a)
{
    *this = *this * a;
    return *this;
}



#define HANDMADE_MATH_H
#endif
