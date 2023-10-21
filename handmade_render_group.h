/****************************************************************************************************

NOTE

  1. everywhere outside render Y always up, X to right.
  
  2. All bitmaps including render target are bottom up - first row
  pointer point to the bottom-most row when viewed on screen. 
 
  3. All inputs to renderer are in meters (NOT pixels) unless
  specified otherwise. All in pixel values MUST be marked explicitly

  4. Z is a special coord - it is broken up into descrete slices and
  renderer understands these slices

  5. All color values in v4 are NOT premultiplied


tag to search by to handle z
// TODO: ZHANDLING

 ****************************************************************************************************/



#define BITMAP_BYTES_PER_PIXEL 4
struct loaded_bitmap
{
    int32 AlignX, AlignY;
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};


enum render_entry_type
{
    RenderEntryType_render_entry_clear,
    RenderEntryType_render_entry_bitmap,
    RenderEntryType_render_entry_rectangle,
    RenderEntryType_render_entry_screen_dot,
    RenderEntryType_render_entry_coordinate_system,
    RenderEntryType_render_entry_saturation
};

struct render_environment_map
{
    loaded_bitmap LevelsOfDetails[4];
    real32 PositionZ;
};

struct render_basis
{
    v3 P;
};

struct render_entity_basis
{
    render_basis *Basis;
    v3 Offset;
    real32 Scale;
};

// NOTE: render group entry is a efficient "discriminated union"
struct render_entry_header
{
    render_entry_type Type;
};

struct render_entry_screen_dot
{
    v2 P;
    real32 Width;
    v4 Color;
};

struct render_entry_clear
{
    v4 Color;
};


// NOTE: this is only for tests
struct render_entry_coordinate_system
{
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;

    loaded_bitmap *Texture;
    loaded_bitmap *NormalMap;

    render_environment_map *Top;
    render_environment_map *Middle;
    render_environment_map *Bottom;
};

struct render_entry_saturation
{
    real32 Level;
};

struct render_entry_rectangle
{
    v2 Dim;
    v4 Color;
    render_entity_basis EntityBasis;
};

struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;
    v4 Color;
    render_entity_basis EntityBasis;
};

struct render_group
{
    real32 GlobalAlpha;
    real32 MetersToPixels;

    render_basis *DefaultBasis;
    
    uint32 PushBufferSize;
    uint32 MaxPushBufferSize;
    uint8 *PushBufferBase;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// renderer api
////////////////////////////////////////////////////////////////////////////////////////////////////


#if 0
inline void PushRenderElement_(render_group *grp, uint32 size, render_entry_type type);
inline void PushBitmap(render_group *grp, loaded_bitmap *bmp, v2 offset, real32 offsetZ, v2 align, real32 alpha = 1.0f, real32 EntitiyZC=1.0f);
inline void PushScreenSquareDot(render_group *grp, v2 screenPosition, real32 width, v4 color);
inline void PushPieceRect(render_group *grp, v2 offset, real32 offsetZ, v2 dim, v4 color);
inline void PushPieceRectOutline(render_group *grp, v2 offset, real32 offsetZ, v2 dim, v4 color);
inline void PushSaturationFilter(render_group *grp, real32 level);
inline void PushDefaultRenderClear(render_group *grp);
#endif
