struct tile_map_position
{
    uint32 AbsTileX;
    uint32 AbsTileY;

    // this is tile relative X and Y
    real32 TileRelX;
    real32 TileRelY;
};




struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_chunk
{
    
    uint32 *Tiles;
};


struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;
    
    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;
    
    real32 LowerLeftX;
    real32 LowerLeftY;
        
    uint32 TileMapCountX;
    uint32 TileMapCountY;

    tile_chunk *TileChunks;
};
