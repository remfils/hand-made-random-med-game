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
IsCanonical(world *world, real32 tileRel)
{
    bool32 result = ((tileRel >= -0.5 * world->ChunkSideInMeters) &&
        (tileRel <= 0.5 * world->ChunkSideInMeters));
    return result;
}

inline bool32
IsCanonical(world *world, v2 offset)
{
    bool32 result = IsCanonical(world, offset.X) && IsCanonical(world, offset.Y);
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

        if (memoryArena && (tileChunk->ChunkX != 0) && (!tileChunk->NextInHash))
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
RecanonicalizeCoord(world *world, int32* tileCoord, real32 *relCoord)
{
    // todo: player can go back
    int32 offset = RoundReal32ToInt32(*relCoord / world->ChunkSideInMeters);
    *tileCoord += offset;
    *relCoord = *relCoord - offset * world->ChunkSideInMeters;

    Assert(IsCanonical(world, *relCoord));
}

inline world_position
MapIntoChunkSpace(world *world, world_position basePosition, v2 offset)
{
    world_position res = basePosition;

    res._Offset += offset;
    RecanonicalizeCoord(world, &res.ChunkX, &res._Offset.X);
    RecanonicalizeCoord(world, &res.ChunkY, &res._Offset.Y);

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

internal world_diff
Subtract(world *world, world_position *a, world_position *b)
{
    world_diff result;

    v2 tilePos = {};
    tilePos.X = (real32)a->ChunkX - (real32)b->ChunkX;
    tilePos.Y = (real32)a->ChunkY - (real32)b->ChunkY;

    tilePos = tilePos * world->ChunkSideInMeters;
    result.dXY = tilePos + a->_Offset - b->_Offset;

    real32 tileDz = (real32)a->ChunkZ - (real32)b->ChunkZ;
    result.dZ = tileDz * world->ChunkSideInMeters;

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
    world->ChunkSideInMeters = (real32) TILES_PER_CHUNK * tileSideInMeters;
    world->FirstFree = 0;
            
    for (uint32 tileChunkIndex = 0; tileChunkIndex < ArrayCount(world->ChunkHash); ++tileChunkIndex)
    {
        world->ChunkHash[tileChunkIndex].ChunkX = TILE_UNINISIALIZED_COORD;
        world->ChunkHash[tileChunkIndex].FirstBlock = {};
        world->ChunkHash[tileChunkIndex].FirstBlock.EntityCount = 0;
    }
}


inline void
ChangeEntityLocationRaw(memory_arena *arena, world *world, uint32 lowEntityIndex, world_position *oldP, world_position *newP)
{
    Assert(!oldP || IsValid(*oldP));
    Assert(!newP || IsValid(*newP));

    if (oldP && newP && AreInSameChunk(oldP, newP))
    {
        // leave entity as is
    }
    else
    {
        if (oldP)
        {
            // pull out entity out of old block

            world_chunk *chunk = GetWorldChunk(world, oldP->ChunkX, oldP->ChunkY, oldP->ChunkZ);

            Assert(chunk);
            if (chunk)
            {
                bool32 isBlockFound = false;
                world_entity_block *firstBlock = &chunk->FirstBlock;
                for (world_entity_block *block = firstBlock; block; block = block->Next)
                {
                    for (uint32 index=0; index < block->EntityCount; index++)
                    {
                        if (block->LowEntityIndex[index] == lowEntityIndex)
                        {
                            block->LowEntityIndex[index] =
                                firstBlock->LowEntityIndex[--firstBlock->EntityCount];

                            if (firstBlock->EntityCount == 0 && firstBlock->Next)
                            {
                                world_entity_block *nextBlock = firstBlock->Next;
                                *firstBlock = *nextBlock;
                                
                                nextBlock->Next = world->FirstFree;
                                world->FirstFree = nextBlock;
                            }

                            isBlockFound = true;
                            break;
                        }
                    }
                    if (isBlockFound)
                    {
                        break;
                    }
                }
            }
        }

        // add entity to new chunk

        if (newP)
        {
            world_chunk *chunk = GetWorldChunk(world, newP->ChunkX, newP->ChunkY, newP->ChunkZ, arena);
            world_entity_block *block = &chunk->FirstBlock;
            if (block->EntityCount == ArrayCount(block->LowEntityIndex))
            {
                // out of room, get new block

                world_entity_block *oldBlock = world->FirstFree;

                if (oldBlock)
                {
                    world->FirstFree = oldBlock->Next;
                }
                else
                {
                    oldBlock = PushSize(arena, world_entity_block);
                }

                *oldBlock = *block;
                block->Next = oldBlock;
                block->EntityCount = 0;
            }
            Assert(block->EntityCount < ArrayCount(block->LowEntityIndex));

            block->LowEntityIndex[block->EntityCount++] = lowEntityIndex;
        }
    }
}

inline void
ChangeEntityLocation(memory_arena *arena, world *world, uint32 lowEntityIndex, low_entity *low, world_position *oldP, world_position *newP)
{
    ChangeEntityLocationRaw(arena, world, lowEntityIndex, oldP, newP);

    if (newP)
    {
        low->WorldP = *newP;
    }
    else
    {
        low->WorldP = NullPosition();
    }
}



inline world_position
ChunkPositionFromTilePosition(world * world, int32 absTileX, int32 absTileY, int32 absTileZ)
{
    world_position result = {};
    result.ChunkX = absTileX / TILES_PER_CHUNK;
    result.ChunkY = absTileY / TILES_PER_CHUNK;
    result.ChunkZ = absTileZ / TILES_PER_CHUNK;

    result._Offset.X = (real32)((absTileX - TILES_PER_CHUNK / 2) - (result.ChunkX * TILES_PER_CHUNK)) * world->TileSideInMeters;
    result._Offset.Y = (real32)((absTileY - TILES_PER_CHUNK / 2) - (result.ChunkY * TILES_PER_CHUNK)) * world->TileSideInMeters;
    // TODO: move to 3dz

    return result;
}
