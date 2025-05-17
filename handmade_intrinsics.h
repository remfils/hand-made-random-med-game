#if COMPILER_MSVC

#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier();

inline uint32
AtomicCompareExchange(uint32 volatile *value, uint32 expected, uint32 newValue)
{
    uint32 result = InterlockedCompareExchange(value, newValue, expected);
    return result;
}
#else
#endif

// TODO: move to math
inline real32
FloorReal32(real32 val)
{
    real32 res = floorf(val);
    return res;
}

inline int32
FloorReal32ToInt32(real32 val)
{
    int32 res = (int32)floorf(val);
    return res;
}


inline real32
CeilReal32(real32 val)
{
    real32 res = ceilf(val);
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

inline real32
SignOf(real32 value)
{
    real32 result = value >= 0.0f ? 1.0f : -1.0f;
    return result;
}

struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};
inline bit_scan_result FindLeastSignificantSetBit(uint32 Value)
{
    bit_scan_result result = {};
    for (uint32 test = 0; test < 32; ++test)
    {
        if (Value & (1 << test))
        {
            result.Index = test;
            result.Found = true;
            break;
        }
    }
    return result;
}

inline uint32 RotateLeft(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
    uint32 Result = _rotl(Value, Amount);
#else
    Amount &= 31;
    uint32 Result = (Value << Amount) | (Value >> (32 - Amount));
#endif
    return Result;
}

inline uint32 RotateRight(uint32 Value, int32 Amount)
{
#if COMPILER_MSVC
    uint32 Result = _rotr(Value, Amount);
#else
    Amount &= 31;
    uint32 Result = (Value << Amount) | (Value >> (32 - Amount));
#endif
    return Result;
}
