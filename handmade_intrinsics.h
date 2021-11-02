// todo: remove math.h ref
#include "math.h"

// todo: remove math.h

inline int32
FloorReal32ToInt32(real32 val)
{
    int32 res = (int32)floorf(val);
    return res;
}

inline int32
CeilReal32ToInt32(real32 val)
{
    int32 res = (int32)ceilf(val);
    return res;
}

inline int32
TruncateReal32ToInt32(real32 val)
{
    return (int32)val;
}

inline int32
RoundReal32ToInt32(real32 value)
{
    int32 result = (int32)roundf(value);

    return(result);
}


inline real32
Sin(real32 ang)
{
    real32 res = sinf(ang);
    return res;
}

inline real32
Cos(real32 ang)
{
    real32 res = cosf(ang);
    return res;
}

inline real32
ATan2(real32 y, real32 x)
{
    real32 res = atan2f(y, x);
    return res;
}

inline real32
AbsoluteValue(real32 val)
{
    real32 res = (real32)fabs(val);
    return res;
}

inline real32
SquareRoot(real32 val)
{
    real32 res = sqrtf(val);
    return res;
}


inline int32
SignOf(int32 value)
{
    int32 result = value >= 0 ? 1 : -1;
    return result;
}
