internal tile_chunk*
GetTileChunk(tile_map * tileMap, int32 tileX, int32 tileY, int32 tileZ, memory_arena *memoryArena = 0)
{
    int32 tileSafeMargin = INT32_MAX - 256;

    Assert(tileX > -tileSafeMargin);
    Assert(tileY > -tileSafeMargin);
    Assert(tileZ > -tileSafeMargin);
    Assert(tileX < tileSafeMargin);
    Assert(tileY < tileSafeMargin);
    Assert(tileZ < tileSafeMargin);


     // TODO: better hash function!!!!
    uint32 hashValue = 19 * tileX + 7 * tileY + 3 * tileZ;
    uint32 hashSlot = hashValue & (ArrayCount(tileMap->TileChunkHash) - 1);

    Assert(hashSlot < ArrayCount(tileMap->TileChunkHash));

    tile_chunk *tileChunk = tileMap->TileChunkHash + hashSlot;

    do
    {
        if (tileX == tileChunk->TileChunkX &&
            tileY == tileChunk->TileChunkY &&
            tileZ == tileChunk->TileChunkZ)
        {
            break;
        }

        if (memoryArena && (tileChunk->TileChunkX != 0) && (!tileChunk->NextInHash))
        {
            tileChunk->NextInHash = PushSize(memoryArena, tile_chunk);
            tileChunk = tileChunk->NextInHash;
            tileChunk->TileChunkX = 0;
        }

        if (memoryArena && (tileChunk->TileChunkX == 0))
        {            
            tileChunk->TileChunkX = tileX;
            tileChunk->TileChunkY = tileY;
            tileChunk->TileChunkZ = tileZ;
            tileChunk->NextInHash = 0;

            uint32 tileCount = tileMap->ChunkDim * tileMap->ChunkDim;
            tileChunk->Tiles = PushArray(memoryArena, tileCount, uint32);

            // TODO: do we want to always initialize

            for (uint32 tindex = 0;
                 tindex < tileCount;
                 ++tindex) {
                tileChunk->Tiles[tindex] = 1;
            }
            
            break;
        }
        
        tileChunk = tileChunk->NextInHash;
    } while (tileChunk);

    return tileChunk;
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *tileMap, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    tile_chunk_position result;

    result.TileChunkX = absTileX >> tileMap->ChunkShift;
    result.TileChunkY = absTileY >> tileMap->ChunkShift;
    result.TileChunkZ = absTileZ;
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
    
    if (tileChunk && tileChunk->Tiles)
    {
        tileChunkValue = GetTileValueUnchecked(tileMap, tileChunk, testX, testY);
    }
    
    return tileChunkValue;
}

internal uint32
GetTileValue(tile_map * tileMap, tile_map_position wp)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, wp.AbsTileX, wp.AbsTileY, wp.AbsTileZ);
    tile_chunk *tileChunk = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY, chunkPos.TileChunkZ);
    uint32 tileChunkValue = GetTileValue(tileMap, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY);
    
    return tileChunkValue;
}

internal uint32
GetTileValue(tile_map *tileMap, uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
    tile_chunk *tileChunk = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY, chunkPos.TileChunkZ);
    uint32 tileChunkValue = GetTileValue(tileMap, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY);

    return tileChunkValue;
}

internal bool32
IsTileValueEmpty(uint32 tileValue)
{
    bool32 isEmpty = tileValue == 1 || tileValue == 3 || tileValue == 4;
    return isEmpty;
}

internal bool32
IsTileMapPointEmpty(tile_map *tileMap, tile_map_position wp)
{
    uint32 tileChunkValue = GetTileValue(tileMap, wp);
    bool32 result = IsTileValueEmpty(tileChunkValue);
    return result;
}

inline void
RecanonicalizeCoord(tile_map *tileMap, int32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = RoundReal32ToInt32(*relCoord / tileMap->TileSideInMeters);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * tileMap->TileSideInMeters;
}

inline tile_map_position
MapIntoTileSpace(tile_map *tileMap, tile_map_position basePosition, v2 offset)
{
    tile_map_position res = basePosition;

    res._Offset += offset;
    RecanonicalizeCoord(tileMap, &res.AbsTileX, &res._Offset.X);
    RecanonicalizeCoord(tileMap, &res.AbsTileY, &res._Offset.Y);

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

internal void InitTileChunk(memory_arena *memoryArena, tile_map *tileMap, tile_chunk *tileChunk)
{
    uint32 tileCount = tileMap->ChunkDim * tileMap->ChunkDim;
    tileChunk->Tiles = PushArray(memoryArena, tileCount, uint32);

    for (uint32 tindex = 0;
         tindex < tileCount;
         ++tindex) {
        tileChunk->Tiles[tindex] = 1;
    }
}

internal void
SetTileValue(memory_arena * arena, tile_map *tileMap, uint32 absTileX, uint32 absTileY, uint32 absTileZ, uint32 val)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
    tile_chunk *tileChunk = GetTileChunk(tileMap, chunkPos.TileChunkX, chunkPos.TileChunkY, chunkPos.TileChunkZ, arena);

    if (!tileChunk->Tiles)
    {
        InitTileChunk(arena, tileMap, tileChunk);
    }

    SetTileValue(tileMap, tileChunk, chunkPos.RelTileX, chunkPos.RelTileY, val);
}


internal bool
AreOnSameTile(tile_map_position posA, tile_map_position posB)
{
    bool32 result = posA.AbsTileX == posB.AbsTileX
        && posA.AbsTileY == posB.AbsTileY
        && posA.AbsTileZ == posB.AbsTileZ;

    return result;
}

internal tile_map_diff
Subtract(tile_map *tileMap, tile_map_position *a, tile_map_position *b)
{
    tile_map_diff result;

    v2 tilePos = {};
    tilePos.X = (real32)a->AbsTileX - (real32)b->AbsTileX;
    tilePos.Y = (real32)a->AbsTileY - (real32)b->AbsTileY;

    tilePos = tilePos * tileMap->TileSideInMeters;
    result.dXY = tilePos + a->_Offset - b->_Offset;

    real32 tileDz = (real32)a->AbsTileZ - (real32)b->AbsTileZ;
    result.dZ = tileDz * tileMap->TileSideInMeters;

    return result;
}


inline tile_map_position
CenteredTilePoint(uint32 absTileX, uint32 absTileY, uint32 absTileZ)
{
    tile_map_position result = {};

    result.AbsTileX = absTileX;
    result.AbsTileY = absTileY;
    result.AbsTileZ = absTileZ;

    return result;
}
