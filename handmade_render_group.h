/****************************************************************************************************

NOTE

  1. everywhere outside render Y always up, X to right.
  
  2. All bitmaps including render target are bottom up - first row
  pointer point to the bottom-most row when viewed on screen. 
 
  3. All inputs to renderer are in meters (NOT pixels) unless
  specified otherwise. All in pixel values MUST be marked explicitly

  4. Z is a special coord - it is broken up into descrete slices and
  renderer understands these slices


tag to search by to handle z
// TODO: ZHANDLING

 ****************************************************************************************************/



#define BITMAP_BYTES_PER_PIXEL 4
struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
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

enum render_entry_type
{
    RenderEntryType_render_entry_clear,
    RenderEntryType_render_entry_bitmap,
    RenderEntryType_render_entry_rectangle,
    RenderEntryType_render_entry_screen_dot,
    RenderEntryType_render_entry_coordinate_system,
    RenderEntryType_render_entry_saturation
};

// NOTE: render group entry is a efficient "discriminated union"
struct render_entry_header
{
    render_entry_type Type;
};

struct render_entity_basis
{
    render_basis *Basis;
    v2 Offset;
    real32 OffsetZ;
    real32 EntitiyZC;
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
    real32 Alpha;
    render_entity_basis EntityBasis;
};

struct render_group
{
    real32 MetersToPixels;

    render_basis *DefaultBasis;
    
    uint32 PushBufferSize;
    uint32 MaxPushBufferSize;
    uint8 *PushBufferBase;
};
