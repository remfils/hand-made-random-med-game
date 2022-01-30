struct tile_map_diff
{
    v2 dXY;
    real32 dZ;
};

struct tile_map_position
{
    int32 AbsTileX;
    int32 AbsTileY;
    int32 AbsTileZ;

    v2 _Offset;
};

struct tile_chunk_position
{
    int32 TileChunkX;
    int32 TileChunkY;
    int32 TileChunkZ;

    int32 RelTileX;
    int32 RelTileY;
};

struct tile_chunk
{
    int32 TileChunkX;
    int32 TileChunkY;
    int32 TileChunkZ;

    uint32 *Tiles;

    tile_chunk *NextInHash;
};


struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;
    
    tile_chunk TileChunkHash[4096];
};

internal void
InitializeTileMap(tile_map *tileMap, real32 tileSideInMeters)
{
    tileMap->TileSideInMeters = tileSideInMeters;
        
    tileMap->ChunkShift = 4;
    tileMap->ChunkMask = (1 << tileMap->ChunkShift) - 1;
    tileMap->ChunkDim = (1 << tileMap->ChunkShift);
    
    for (uint32 tileChunkIndex = 0; tileChunkIndex < ArrayCount(tileMap->TileChunkHash); ++tileChunkIndex)
    {
        tileMap->TileChunkHash[tileChunkIndex].TileChunkX = 0;
    }
}
