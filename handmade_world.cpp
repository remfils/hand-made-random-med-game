internal world_chunk*
GetChunk(world *world, int32 tileX, int32 tileY, int32 tileZ, memory_arena *memoryArena = 0)
{
    Assert(tileX > -TILE_SAFE_MARGIN);
    Assert(tileY > -TILE_SAFE_MARGIN);
    Assert(tileZ > -TILE_SAFE_MARGIN);
    Assert(tileX < TILE_SAFE_MARGIN);
    Assert(tileY < TILE_SAFE_MARGIN);
    Assert(tileZ < TILE_SAFE_MARGIN);


     // TODO: better hash function!!!!
    uint32 hashValue = 19 * tileX + 7 * tileY + 3 * tileZ;
    uint32 hashSlot = hashValue & (ArrayCount(world->ChunkHash) - 1);

    Assert(hashSlot < ArrayCount(world->ChunkHash));

    world_chunk *tileChunk = world->ChunkHash + hashSlot;

    do
    {
        if (tileX == tileChunk->ChunkX &&
            tileY == tileChunk->ChunkY &&
            tileZ == tileChunk->ChunkZ)
        {
            break;
        }

        if (memoryArena && (tileChunk->ChunkX != 0) && (!tileChunk->NextInHash))
        {
            tileChunk->NextInHash = PushSize(memoryArena, world_chunk);
            tileChunk = tileChunk->NextInHash;
            tileChunk->ChunkX = TILE_UNINISIALIZED_COORD;
        }

        if (memoryArena && (tileChunk->ChunkX == TILE_UNINISIALIZED_COORD))
        {            
            tileChunk->ChunkX = tileX;
            tileChunk->ChunkY = tileY;
            tileChunk->ChunkZ = tileZ;
            tileChunk->NextInHash = 0;

            uint32 tileCount = world->ChunkDim * world->ChunkDim;

            // TODO: do we want to always initialize

            // for (uint32 tindex = 0;
            //      tindex < tileCount;
            //      ++tindex) {
            //     tileChunk->Tiles[tindex] = 1;
            // }
            
            break;
        }
        
        tileChunk = tileChunk->NextInHash;
    } while (tileChunk);

    return tileChunk;
}

inline void
RecanonicalizeCoord(world *world, int32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = RoundReal32ToInt32(*relCoord / world->TileSideInMeters);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * world->TileSideInMeters;
}

inline world_position
MapIntoTileSpace(world *world, world_position basePosition, v2 offset)
{
    world_position res = basePosition;

    res._Offset += offset;
    RecanonicalizeCoord(world, &res.AbsTileX, &res._Offset.X);
    RecanonicalizeCoord(world, &res.AbsTileY, &res._Offset.Y);

    return res;
}

internal bool
AreOnSameTile(world_position posA, world_position posB)
{
    bool32 result = posA.AbsTileX == posB.AbsTileX
        && posA.AbsTileY == posB.AbsTileY
        && posA.AbsTileZ == posB.AbsTileZ;

    return result;
}

internal world_diff
Subtract(world *world, world_position *a, world_position *b)
{
    world_diff result;

    v2 tilePos = {};
    tilePos.X = (real32)a->AbsTileX - (real32)b->AbsTileX;
    tilePos.Y = (real32)a->AbsTileY - (real32)b->AbsTileY;

    tilePos = tilePos * world->TileSideInMeters;
    result.dXY = tilePos + a->_Offset - b->_Offset;

    real32 tileDz = (real32)a->AbsTileZ - (real32)b->AbsTileZ;
    result.dZ = tileDz * world->TileSideInMeters;

    return result;
}


inline world_position
CenteredTilePoint(uint32 absX, uint32 absY, uint32 absZ)
{
    world_position result = {};

    result.AbsTileX = absX;
    result.AbsTileY = absY;
    result.AbsTileZ = absZ;

    return result;
}


internal void
InitializeWorld(world *world, real32 tileSideInMeters)
{
    world->TileSideInMeters = tileSideInMeters;
        
    world->ChunkShift = 4;
    world->ChunkMask = (1 << world->ChunkShift) - 1;
    world->ChunkDim = (1 << world->ChunkShift);
    
    for (uint32 tileChunkIndex = 0; tileChunkIndex < ArrayCount(world->ChunkHash); ++tileChunkIndex)
    {
        world->ChunkHash[tileChunkIndex].ChunkX = TILE_UNINISIALIZED_COORD;
    }
}
