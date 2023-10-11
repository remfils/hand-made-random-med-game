#if !defined(HANDMADE_WORLD_H)
#define HANDMADE_WORLD_H


#define TILE_SAFE_MARGIN (INT32_MAX/64)
#define TILE_UNINISIALIZED_COORD INT32_MAX


struct world_position
{
    // TODO: how to remove abstile* 
    
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;

    v3 _Offset;
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

    world_entity_block FirstBlock;

    world_chunk *NextInHash;
};



struct world
{
    uint32 TotalChunksAdded;

    v3 ChunkDimInMeters;
    world_chunk ChunkHash[100];

    world_entity_block *FirstFree;
};

#endif
