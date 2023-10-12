struct render_basis
{
    v3 P;
};

enum render_entry_type
{
    RenderEntryType_render_entry_clear,
    RenderEntryType_render_entry_bitmap,
    RenderEntryType_render_entry_rectangle
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

struct render_entry_clear
{
    render_entry_header Header;
    v4 Color;
};

struct render_entry_rectangle
{
    render_entry_header Header;
    v2 Dim;
    v4 Color;
    render_entity_basis EntityBasis;
};

struct render_entry_bitmap
{
    render_entry_header Header;
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
