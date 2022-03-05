#if !defined(HANDMADE_WORLD_H)
#define HANDMADE_WORLD_H


#define TILE_SAFE_MARGIN (INT32_MAX/64)
#define TILE_UNINISIALIZED_COORD INT32_MAX


struct world_diff
{
    v2 dXY;
    real32 dZ;
};

struct world_position
{
    // TODO: how to remove abstile* 
    
    int32 AbsTileX;
    int32 AbsTileY;
    int32 AbsTileZ;

    v2 _Offset;
};

struct world_entity_block
{
    world_entity_block *Next;
    uint32 EntityCount;
    uint32 LowEntityIndex[16];
};

struct world_chunk
{
    int32 ChunkX, ChunkY, ChunkZ;

    world_entity_block *FirstBlock;

    world_chunk *NextInHash;
};



struct world
{
    real32 TileSideInMeters;

    int32 ChunkShift;
    int32 ChunkMask;
    int32 ChunkDim;
    world_chunk ChunkHash[4096];
};

#endif
