internal tile_chunk*
GetTileChunk(tile_map * tileMap, uint32 tileX, uint32 tileY)
{
    tile_chunk *tileChunk = 0;

    if ((tileX >= 0) && (tileX < tileMap->TileMapCountX) &&
        (tileY >= 0) && (tileY < tileMap->TileMapCountY))
    {
        tileChunk = &tileMap->TileChunks[tileY * tileMap->TileMapCountX + tileX];
    }

    return tileChunk;
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *tileMap, uint32 absTileX, uint32 absTileY)
{
    tile_chunk_position result;

    result.TileChunkX = absTileX >> tileMap->ChunkShift;
    result.TileChunkY = absTileY >> tileMap->ChunkShift;
    result.RelTileX = absTileX & tileMap->ChunkMask;
    result.RelTileY = absTileY & tileMap->ChunkMask;

    return result;
}

inline uint32
GetTileValueUnchecked(tile_map *tileMap, tile_chunk *tileChunk, uint32 tileX, uint32 tileY)
{
    Assert(tileChunk);
    Assert(tileX < tileMap->ChunkDim);
    Assert(tileY < tileMap->ChunkDim);
    
    uint32 tileValue = tileChunk->Tiles[tileY * tileMap->ChunkDim + tileX];
    return tileValue;
}

internal uint32
GetTileValue(tile_map * tileMap, tile_chunk *tileChunk, uint32 testX, uint32 testY)
{
    uint32 tileChunkValue = 0;
    
    if (tileChunk)
    {
        tileChunkValue = GetTileValueUnchecked(tileMap, tileChunk, testX, testY);
    }
    
    return tileChunkValue;
}

internal uint32
GetTileValue(tile_map * tileMap, tile_map_position wp)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, wp.AbsTileX, wp.AbsTileY);
    tile_chunk *tileChunk = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY);
    uint32 tileChunkValue = GetTileValue(tileMap, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY);
    
    return tileChunkValue;
}

internal uint32
GetTileValue(tile_map *tileMap, uint32 absTileX, uint32 absTileY)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, absTileX, absTileY);
    tile_chunk *tileChunk = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY);
    uint32 tileChunkValue = GetTileValue(tileMap, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY);

    return tileChunkValue;
}

internal bool32
IsTileMapPointEmpty(tile_map *tileMap, tile_map_position wp)
{
    uint32 tileChunkValue = GetTileValue(tileMap, wp);

    return tileChunkValue == 0;
}

inline void
RecanonicalizeCoord(tile_map *tileMap, uint32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = RoundReal32ToInt32(*relCoord / tileMap->TileSideInMeters);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * tileMap->TileSideInMeters;
}

inline tile_map_position
RecanonicalizePosition(tile_map *tileMap, tile_map_position pos)
{
    tile_map_position res = pos;
    
    RecanonicalizeCoord(tileMap, &res.AbsTileX, &res.TileRelX);
    RecanonicalizeCoord(tileMap, &res.AbsTileY, &res.TileRelY);

    return res;
}

inline void
SetTileValueUnchecked(tile_map *tileMap, tile_chunk *tileChunk, uint32 tileX, uint32 tileY, uint32 val)
{
    Assert(tileChunk);
    Assert(tileX < tileMap->ChunkDim);
    Assert(tileY < tileMap->ChunkDim);
    
    tileChunk->Tiles[tileY * tileMap->ChunkDim + tileX] = val;
}

internal void
SetTileValue(tile_map * tileMap, tile_chunk *tileChunk, uint32 testX, uint32 testY, uint32 val)
{
    uint32 tileChunkValue = 0;
    
    if (tileChunk)
    {
        SetTileValueUnchecked(tileMap, tileChunk, testX, testY, val);
    }
}

internal void
SetTileValue(memory_arena * arena, tile_map *tileMap, uint32 absTileX, uint32 absTileY, uint32 val)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, absTileX, absTileY);
    tile_chunk *tileChunk = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY);

    // if (!tileChunk)
    // {
        
    // }

    // todo: on demand tile chunkg creation

    SetTileValue(tileMap, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY, val);
}

