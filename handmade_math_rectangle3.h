inline rectangle3
AddRadiusTo(rectangle3 rect, v3 dim)
{
    rectangle3 result;

    result.Min = rect.Min - dim;
    result.Max = rect.Max + dim;

    return result;
}

inline rectangle3
Offset(rectangle3 rect, v3 offset)
{
    rectangle3 result;

    result.Min = rect.Min + offset;
    result.Max = rect.Max + offset;

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
        (test.x >= rect.Min.x)
        && (test.y >= rect.Min.y)
        && (test.z >= rect.Min.z)
        
        && (test.x < rect.Max.x)
        && (test.y < rect.Max.y)
        && (test.z < rect.Max.z)
        );

    return result;
}

inline bool32
RectanglesIntersect(rectangle3 a, rectangle3 b)
{
    // NOTE: this is double checked - now works
    
    bool32 result = !(
                     (b.Max.x <= a.Min.x) ||
                     (b.Min.x >= a.Max.x) ||
                     
                     (b.Max.y <= a.Min.y) ||
                     (b.Min.y >= a.Max.y) ||
                     
                     (b.Max.z <= a.Min.z) ||
                     (b.Min.z >= a.Max.z)
                      );

    return result;

}

inline v3
GetBarycentric(rectangle3 a, v3 p)
{
    v3 result;

    result.x = SafeRatio_0(p.x - a.Min.x, a.Max.x - a.Min.x);
    result.y = SafeRatio_0(p.y - a.Min.y, a.Max.y - a.Min.y);
    result.z = SafeRatio_0(p.z - a.Min.z, a.Max.z - a.Min.z);

    return result;
}

inline v3
Clamp01(v3 value)
{
    v3 result;
    result.x = Clamp01(value.x);
    result.y = Clamp01(value.y);
    result.z = Clamp01(value.z);
    
    return result;
}

inline v3
GetDim(rectangle3 rect)
{
    v3 result = rect.Max - rect.Min;
    return result;
}
