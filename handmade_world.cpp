#define TILES_PER_CHUNK 16

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
    bool32 result = IsCanonical(world->ChunkDimInMeters.X, offset.X)
        && IsCanonical(world->ChunkDimInMeters.Y, offset.Y)
        && IsCanonical(world->ChunkDimInMeters.Z, offset.Z);
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
            tileChunk->NextInHash = PushSize(memoryArena, world_chunk);
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
    RecanonicalizeCoord(world->ChunkDimInMeters.X, &res.ChunkX, &res._Offset.X);
    RecanonicalizeCoord(world->ChunkDimInMeters.Y, &res.ChunkY, &res._Offset.Y);
    RecanonicalizeCoord(world->ChunkDimInMeters.Z, &res.ChunkZ, &res._Offset.Z);

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
    dTilePos.X = (real32)a->ChunkX - (real32)b->ChunkX;
    dTilePos.Y = (real32)a->ChunkY - (real32)b->ChunkY;
    dTilePos.Z = (real32)a->ChunkZ - (real32)b->ChunkZ;

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



internal void
InitializeWorld(world *world, real32 tileSideInMeters)
{
    world->TileSideInMeters = tileSideInMeters;
    world->TileDepthInMeters = tileSideInMeters;
    world->ChunkDimInMeters = {
        (real32) TILES_PER_CHUNK * tileSideInMeters,
        (real32) TILES_PER_CHUNK * tileSideInMeters,
        (real32) tileSideInMeters
    };
    world->FirstFree = 0;
            
    for (uint32 tileChunkIndex = 0; tileChunkIndex < ArrayCount(world->ChunkHash); ++tileChunkIndex)
    {
        world->ChunkHash[tileChunkIndex].ChunkX = TILE_UNINISIALIZED_COORD;
        world->ChunkHash[tileChunkIndex].FirstBlock = {};
        world->ChunkHash[tileChunkIndex].FirstBlock.EntityCount = 0;
    }
}


inline world_position
ChunkPositionFromTilePosition(world * world, int32 absTileX, int32 absTileY, int32 absTileZ, v3 additionalOffset=V3(0,0,0))
{
    world_position basePosition = {};

    v3 offset = world->TileSideInMeters * V3((real32)absTileX, (real32)absTileY, (real32)absTileZ);

    world_position result = MapIntoChunkSpace(world, basePosition, additionalOffset+ offset);

    return result;
}
