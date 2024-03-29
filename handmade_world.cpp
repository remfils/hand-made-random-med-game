inline world_position
NullPosition(void)
{
    world_position result;
    result.ChunkX = TILE_UNINISIALIZED_COORD;
    return result;
}

inline bool32
IsValid(world_position wp)
{
    bool32 result = wp.ChunkX != TILE_UNINISIALIZED_COORD;
    return result;
}

inline bool32
IsCanonical(real32 chunkDim, real32 tileRel)
{
    real32 eps = 0.001f;
    bool32 result = ((tileRel >= -(0.5 * chunkDim + eps)) &&
        (tileRel <= (0.5 * chunkDim + eps)));
    return result;
}

inline bool32
IsCanonical(world *world, v3 offset)
{
    bool32 result = IsCanonical(world->ChunkDimInMeters.x, offset.x)
        && IsCanonical(world->ChunkDimInMeters.y, offset.y)
        && IsCanonical(world->ChunkDimInMeters.z, offset.z);
    return result;
}


internal world_chunk*
GetWorldChunk(world *world, int32 tileX, int32 tileY, int32 tileZ, memory_arena *memoryArena = 0)
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

    world_chunk *tileChunk = &world->ChunkHash[hashSlot];

    do
    {
        if (tileX == tileChunk->ChunkX &&
            tileY == tileChunk->ChunkY &&
            tileZ == tileChunk->ChunkZ)
        {
            break;
        }

        if (memoryArena && (tileChunk->ChunkX != TILE_UNINISIALIZED_COORD) && (!tileChunk->NextInHash))
        {
            tileChunk->NextInHash = PushStruct(memoryArena, world_chunk);
            tileChunk->NextInHash->ChunkX = TILE_UNINISIALIZED_COORD;
        }

        if (memoryArena && (tileChunk->ChunkX == TILE_UNINISIALIZED_COORD))
        {            
            tileChunk->ChunkX = tileX;
            tileChunk->ChunkY = tileY;
            tileChunk->ChunkZ = tileZ;
            tileChunk->NextInHash = 0;

            break;
        }
        
        tileChunk = tileChunk->NextInHash;
    } while (tileChunk);

    return tileChunk;
}

inline void
RecanonicalizeCoord(real32 chunkDim, int32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = RoundReal32ToInt32(*relCoord / chunkDim);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * chunkDim;

    Assert(IsCanonical(chunkDim, *relCoord));
}

inline world_position
MapIntoChunkSpace(world *world, world_position basePosition, v3 offset)
{
    world_position res = basePosition;

    res._Offset += offset;
    RecanonicalizeCoord(world->ChunkDimInMeters.x, &res.ChunkX, &res._Offset.x);
    RecanonicalizeCoord(world->ChunkDimInMeters.y, &res.ChunkY, &res._Offset.y);
    RecanonicalizeCoord(world->ChunkDimInMeters.z, &res.ChunkZ, &res._Offset.z);

    return res;
}

internal bool
AreInSameChunk(world_position *posA, world_position *posB)
{
    bool32 result = posA->ChunkX == posB->ChunkX
        && posA->ChunkY == posB->ChunkY
        && posA->ChunkZ == posB->ChunkZ;

    return result;
}

internal v3
Subtract(world *world, world_position *a, world_position *b)
{
    v3 dTilePos = {};
    dTilePos.x = (real32)a->ChunkX - (real32)b->ChunkX;
    dTilePos.y = (real32)a->ChunkY - (real32)b->ChunkY;
    dTilePos.z = (real32)a->ChunkZ - (real32)b->ChunkZ;

    v3 result = Hadamard(world->ChunkDimInMeters, dTilePos) + (a->_Offset - b->_Offset);

    return result;
}


inline world_position
CenteredTilePoint(uint32 absX, uint32 absY, uint32 absZ)
{
    world_position result = {};

    result.ChunkX = absX;
    result.ChunkY = absY;
    result.ChunkZ = absZ;

    return result;
}

inline world_position
CenteredTilePoint(world_chunk *chunk)
{
    world_position result = CenteredTilePoint(chunk->ChunkX, chunk->ChunkY, chunk->ChunkZ);

    return result;
}



internal void
InitializeWorld(world *world, v3 chunkDepthInMeters)
{
    world->ChunkDimInMeters = chunkDepthInMeters;
    world->FirstFree = 0;
            
    for (uint32 tileChunkIndex = 0; tileChunkIndex < ArrayCount(world->ChunkHash); ++tileChunkIndex)
    {
        world->ChunkHash[tileChunkIndex].ChunkX = TILE_UNINISIALIZED_COORD;
        world->ChunkHash[tileChunkIndex].FirstBlock = {};
        world->ChunkHash[tileChunkIndex].FirstBlock.EntityCount = 0;
    }
}
