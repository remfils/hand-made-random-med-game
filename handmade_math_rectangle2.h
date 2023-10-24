inline rectangle2
AddRadiusTo(rectangle2 rect, real32 radiusW, real32 radiusH)
{
    rectangle2 result;

    result.Min = rect.Min - ToV2(radiusW, radiusH);
    result.Max = rect.Max + ToV2(radiusW, radiusH);

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


inline v2
GetMinCorner(rectangle2 rect)
{
    v2 result = rect.Min;
    return result;
}

inline v2
GetMaxCorner(rectangle2 rect)
{
    v2 result = rect.Max;
    return result;
}

inline v2
GetCenter(rectangle2 rect)
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
GetDim(rectangle2 rect)
{
    v2 result = rect.Max - rect.Min;
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
