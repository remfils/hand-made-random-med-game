struct render_basis
{
    v3 P;
};


struct entity_visible_piece
{
    render_basis *Basis;
    
    loaded_bitmap *Bitmap;
    v2 Offset;
    real32 OffsetZ;
    real32 EntitiyZC;
    real32 Alpha;

    v2 Dim;
    v4 Color;
};

struct render_group
{
    real32 MetersToPixels;

    render_basis *DefaultBasis;
    
    uint32 PieceCount;
    uint32 PushBufferSize;
    uint32 MaxPushBufferSize;
    uint8 *PushBufferBase;
};
