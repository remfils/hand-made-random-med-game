#define IvalidP V2(10000.0f, 10000.0f)

inline bool32
IsSet(sim_entity *entity, uint32 flag)
{
    bool32 result = entity->Flags & flag;
    return result;
}

inline void
AddFlag(sim_entity *entity, uint32 flag)
{
    entity->Flags |= flag;
}

inline void
ClearFlag(sim_entity *entity, uint32 flag)
{
    entity->Flags &= ~flag;
}

inline void
MakeEntityNonSpacial(sim_entity *simEntity)
{
    simEntity->P = IvalidP;
    AddFlag(simEntity, EntityFlag_Nonspacial);
}

inline void
MakeEntitySpacial(sim_entity *simEntity, v2 pos, v2 dP)
{
    simEntity->P = pos;
    simEntity->dP = dP;
    ClearFlag(simEntity, EntityFlag_Nonspacial);
}



inline void
StoreEntityReference(entity_reference *ref)
{
    if (ref->Ptr)
    {
        ref->Index = ref->Ptr->StorageIndex;
    }
}

inline sim_entity_hash *
GetHashFromStorageIndex(sim_region *simRegion, uint32 storeIndex)
{
    Assert(storeIndex);

    sim_entity_hash *result = 0;
    if(storeIndex)
    {
        uint32 hashValue = storeIndex;
        for(uint32 offset=0;
            offset < ArrayCount(simRegion->Hash);
            ++offset)
        {
            uint32 hashMask = (ArrayCount(simRegion->Hash) - 1);
            uint32 hashIndex = ((hashValue + offset) & hashMask);
            sim_entity_hash *entry = simRegion->Hash + hashIndex;

            if (entry->Index == storeIndex || entry->Index == 0) {
                result = entry;
                break;
            }
        }
    }

    return result;
}

inline sim_entity*
GetEntityByStorageIndex(sim_region *simRegion, uint32 storageIndex)
{
    sim_entity_hash *entry = GetHashFromStorageIndex(simRegion, storageIndex);
    sim_entity *result = entry->Ptr;
    return result;
}

inline v2
GetSimSpaceP(sim_region *simRegion, low_entity *stored)
{
    // TODO(casey): set to singlaling NAN in debug mode?
    v2 result = IvalidP;

    if (!IsSet(&stored->Sim, EntityFlag_Nonspacial))
    {
        world_diff diff = Subtract(simRegion->World, &stored->WorldP, &simRegion->Origin);
        result = diff.dXY;
    }
    
    return result;
}

internal low_entity *
GetLowEntity(game_state *gameState, uint32 lowIndex)
{
    low_entity *result = 0;
    if (lowIndex > 0 && lowIndex < gameState->LowEntityCount)
    {
        result = gameState->LowEntities + lowIndex;
    }

    return result;
}

internal sim_entity*
AddEntity(game_state * gameState, sim_region *region, uint32 sourceIndex, low_entity *source, v2 *simP);
inline void
LoadEntityReference(game_state *gameState, sim_region *simRegion, entity_reference *ref)
{
    if (ref->Index)
    {
        sim_entity_hash *simHash = GetHashFromStorageIndex(simRegion, ref->Index);
        if (simHash->Ptr == 0)
        {
            simHash->Index = ref->Index;
            low_entity *low = GetLowEntity(gameState, ref->Index);
            v2 pos = GetSimSpaceP(simRegion, low);
            simHash->Ptr = AddEntity(gameState, simRegion, ref->Index, low, &pos);
        }

        ref->Ptr = simHash->Ptr;
    }
}

internal sim_entity*
AddEntityRaw(game_state * gameState, sim_region *simRegion, uint32 sourceIndex, low_entity *source)
{
    sim_entity *simEntity = 0;

    sim_entity_hash *simHash = GetHashFromStorageIndex(simRegion, sourceIndex);
    if (simHash->Ptr == 0)
    {
        if (simRegion->EntityCount < simRegion->MaxEntityCount)
        {
            simEntity = simRegion->Entities + simRegion->EntityCount++;

            simHash->Index = sourceIndex;
            simHash->Ptr = simEntity;

            if (source)
            {
                // TODO: decompress this
                *simEntity = source->Sim;
                LoadEntityReference(gameState, simRegion, &simEntity->Sword);
            }
        
            simEntity->StorageIndex = sourceIndex;
            simEntity->Updatable = false;
        }   
        else
        {
            InvalidCodePath;
        }
    }
    else
    {
        simEntity = simHash->Ptr;
    }

    return simEntity;
}

internal sim_entity*
AddEntity(game_state * gameState, sim_region *simRegion, uint32 sourceIndex, low_entity *source, v2 *simP)
{
    sim_entity *dest = AddEntityRaw(gameState, simRegion, sourceIndex, source);
    if (dest)
    {
        if (simP)
        {
            dest->P = *simP;
            dest->Updatable = IsInRectangle(simRegion->UpdatableBounds, dest->P);
        }
        else
        {
            dest->P = GetSimSpaceP(simRegion, source);
        }
    }
    
    return dest;
}



internal sim_region*
BeginSim(memory_arena *simArena, game_state *gameState, world *world, world_position regionCenter, rectangle2 regionBounds)
{
    sim_region *simRegion = PushSize(simArena, sim_region);
    ZeroStruct(simRegion->Hash);

    // TODO: figure out what this is = max(speed + width) on all entities
    real32 updateSafetyMargin = 1.0f;


    simRegion->MaxEntityCount = 4096; // TODO: how many?
    simRegion->EntityCount = 0;
    simRegion->Entities = PushArray(simArena, simRegion->MaxEntityCount, sim_entity);

    simRegion->World = world;
    simRegion->Origin = regionCenter;
    simRegion->Bounds = regionBounds;
    simRegion->UpdatableBounds = AddRadiusTo(regionBounds, updateSafetyMargin, updateSafetyMargin);

    world_position minChunkP = MapIntoChunkSpace(simRegion->World, simRegion->Origin, GetMinCorner(simRegion->Bounds));
    world_position maxChunkP = MapIntoChunkSpace(simRegion->World, simRegion->Origin, GetMaxCorner(simRegion->Bounds));

    for (int32 chunkY=minChunkP.ChunkY;
         chunkY <= maxChunkP.ChunkY;
         chunkY++)
    {
        for (int32 chunkX=minChunkP.ChunkX;
             chunkX <= maxChunkP.ChunkX;
             chunkX++)
        {
            world_chunk *chunk = GetWorldChunk(simRegion->World, chunkX, chunkY, simRegion->Origin.ChunkZ);
            if (chunk)
            {
                for (world_entity_block *block = &chunk->FirstBlock;
                     block;
                     block = block->Next)
                {
                    for (uint32 entityIndex = 0;
                         entityIndex < block->EntityCount;
                         ++entityIndex)
                    {
                        uint32 lowEntityIndex = block->LowEntityIndex[entityIndex];
                        low_entity *low = gameState->LowEntities + lowEntityIndex;

                        if (!IsSet(&low->Sim, EntityFlag_Nonspacial))
                        {
                            v2 simSpaceP = GetSimSpaceP(simRegion, low);

                            if (IsInRectangle(simRegion->Bounds, simSpaceP))
                            {
                                AddEntity(gameState, simRegion, lowEntityIndex, low, &simSpaceP);
                            }    
                        }
                    }
                }
            }
        }
    }

    return simRegion;
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
ChangeEntityLocation(memory_arena *arena, world *world,
    uint32 lowEntityIndex, low_entity *low, world_position newPInit)
{
    world_position *oldP = 0;
    world_position *newP = 0;

    if (!IsSet(&low->Sim, EntityFlag_Nonspacial) && IsValid(low->WorldP))
    {
        oldP = &low->WorldP;
    }

    if (IsValid(newPInit))
    {
        newP = &newPInit;
    }

    ChangeEntityLocationRaw(arena, world, lowEntityIndex, oldP, newP);

    if (newP)
    {
        low->WorldP = *newP;
        ClearFlag(&low->Sim, EntityFlag_Nonspacial);
    }
    else
    {
        low->WorldP = NullPosition();
        AddFlag(&low->Sim, EntityFlag_Nonspacial);
    }
}


// TODO: low entities in world
internal void
EndSim(sim_region *region, game_state *gameState)
{
    world *world = gameState->World;

    sim_entity * simEntity = region->Entities;
    for (uint32 entityIndex=0;
         entityIndex < region->EntityCount;
         ++entityIndex, ++simEntity)
    {
        low_entity * stored = gameState->LowEntities + simEntity->StorageIndex;

        stored->Sim = *simEntity;

        StoreEntityReference(&stored->Sim.Sword);

        world_position newPosition = !IsSet(simEntity, EntityFlag_Nonspacial)
            ? MapIntoChunkSpace(region->World, region->Origin, simEntity->P)
            : NullPosition();

        // TODO: save state to store
        ChangeEntityLocation(&gameState->WorldArena, region->World, simEntity->StorageIndex, stored, newPosition);

        if (simEntity->StorageIndex == gameState->CameraFollowingEntityIndex)
        {
            world_position newCameraPosition = gameState->CameraPosition;
            newCameraPosition.ChunkZ = stored->WorldP.ChunkZ;

#if 0
            if (simEntity->P.Y > (5 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkY += 9;
            }
            if (simEntity->P.Y < -(5 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkY -= 9;
            }
            
            if (simEntity->P.X > (9 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkX += 17;
            }
            if (simEntity->P.X < -(9 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkX -= 17;
            }
#else
            gameState->CameraPosition = stored->WorldP;
#endif
        }
    }
}


inline move_spec
DefaultMoveSpec(void)
{
    move_spec moveSpec;
    moveSpec.UnitMaxAccelVector = true;
    moveSpec.Speed = 1.0f;
    moveSpec.Drag = 0.0f;
    return moveSpec;
}

internal bool32
TestWall(real32 wallX, real32 relX, real32 relY, real32 playerDeltaX, real32 playerDeltaY, real32 *tMin, real32 minY, real32 maxY)
{
    bool32 result = false;
    real32 tEpsilon = 0.001f;

    if (playerDeltaX != 0.0f)
    {
        real32 tResult = (wallX - relX) / playerDeltaX;
        real32 y = relY + tResult * playerDeltaY;
        if ((tResult >= 0.0f) && (*tMin > tResult))
        {
            if ((y >= minY) && (y <= maxY))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                result = true;
            }
        }
    }
    return result;
}


internal void
HandleCollision(sim_region *simRegion, sim_entity *a, sim_entity *b)
{
    if (a->Type == EntityType_Monster && b->Type == EntityType_Sword)
    {
        a->HitPointMax -= 1;
        MakeEntityNonSpacial(b);
    }
}

internal void
MoveEntity(sim_region *simRegion, sim_entity *movingEntity, real32 dt, move_spec *moveSpec, v2 ddPosition)
{
    Assert(!IsSet(movingEntity, EntityFlag_Nonspacial));

    // normlize ddPlayer
    real32 ddLen = SquareRoot(LengthSq(ddPosition));
    if (ddLen > 1)
    {
        ddPosition = (1.0f / ddLen) * ddPosition;
    }

    world *world = simRegion->World;
    
    ddPosition *= moveSpec->Speed;

    ddPosition += -moveSpec->Drag * movingEntity->dP;

    // NOTE: here you can add super speed fn
    v2 oldEntityPosition = movingEntity->P;
    v2 entityPositionDelta = 0.5f * dt * dt * ddPosition + movingEntity->dP * dt;
    movingEntity->dP = ddPosition * dt + movingEntity->dP;
    v2 newEntityPosition = oldEntityPosition + entityPositionDelta;


    real32 distanceRemaining = movingEntity->DistanceLimit;
    if (distanceRemaining == 0.0f)
    {
        // TODO: formalize number
        distanceRemaining = 10000.0f;
    }
    
    for (uint32 iteration = 0;
         iteration < 4;
         iteration++)
    {
        // get lower if collision detected
        real32 tMin = 1.0f;
        real32 entityDeltaLength = Length(entityPositionDelta);

        if (entityDeltaLength > 0)
        {
            if (entityDeltaLength > distanceRemaining)
            {
                tMin = distanceRemaining / entityDeltaLength;
            }
        
            v2 wallNormal = {0,0};
            sim_entity *hitEntity = 0;

            v2 desiredPosition = movingEntity->P + entityPositionDelta;

            bool32 stopsAtCollision = IsSet(movingEntity, EntityFlag_Collides);

            if (!IsSet(movingEntity, EntityFlag_Nonspacial))
            {

                // TODO: spacial partion
                for (uint32 testEntityIndex = 0;
                     testEntityIndex < simRegion->EntityCount;
                     ++testEntityIndex)
                {
                    sim_entity *testEntity = simRegion->Entities + testEntityIndex;
                    if (movingEntity != testEntity)
                    {
                        if (IsSet(testEntity, EntityFlag_Collides) && !IsSet(testEntity, EntityFlag_Nonspacial))
                        {
                            real32 diameterWidth = testEntity->Width + movingEntity->Width;
                            real32 diameterHeight = testEntity->Height + movingEntity->Height;

                            v2 minCorner = -0.5f * v2{diameterWidth, diameterHeight};
                            v2 maxCorner = 0.5f * v2{diameterWidth, diameterHeight};

                            v2 rel = movingEntity->P - testEntity->P;

                            if (TestWall(minCorner.X, rel.X, rel.Y, entityPositionDelta.X, entityPositionDelta.Y,
                                    &tMin, minCorner.Y, maxCorner.Y))
                            {
                                wallNormal = v2{-1,0};
                                hitEntity = testEntity;
                            }
                            if (TestWall(maxCorner.X, rel.X, rel.Y, entityPositionDelta.X, entityPositionDelta.Y,
                                    &tMin, minCorner.Y, maxCorner.Y))
                            {
                                wallNormal = v2{1,0};
                                hitEntity = testEntity;
                            }
                            if (TestWall(minCorner.Y, rel.Y, rel.X, entityPositionDelta.Y, entityPositionDelta.X,
                                    &tMin, minCorner.Y, maxCorner.Y))
                            {
                                wallNormal = v2{0,-1};
                                hitEntity = testEntity;
                            }
                            if (TestWall(maxCorner.Y, rel.Y, rel.X, entityPositionDelta.Y, entityPositionDelta.X,
                                    &tMin, minCorner.Y, maxCorner.Y))
                            {
                                wallNormal = v2{0,1};
                                hitEntity = testEntity;
                            }   
                        }
                    }
                }
            }

            movingEntity->P += tMin * entityPositionDelta;
            distanceRemaining -= tMin * entityDeltaLength;
            if (hitEntity)
            {
                entityPositionDelta = desiredPosition - movingEntity->P;

                if (stopsAtCollision)
                {
                    entityPositionDelta = entityPositionDelta - 1 * Inner(entityPositionDelta, wallNormal) * wallNormal;
                    movingEntity->dP = movingEntity->dP - 1 * Inner(movingEntity->dP, wallNormal) * wallNormal;
                }

                // TODO: collision table here
                
                sim_entity *a = movingEntity;
                sim_entity *b = hitEntity;
                if (a->Type > b->Type)
                {
                    sim_entity *temp = a;
                    a = b;
                    b = temp;
                }

                HandleCollision(simRegion, a, b);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if (movingEntity->DistanceLimit != 0)
    {
        movingEntity->DistanceLimit = distanceRemaining;
    }
    
    if (movingEntity->dP.X == 0 && movingEntity->dP.Y == 0)
    {
        // dont change direction if both zero
    }
    else if (AbsoluteValue(movingEntity->dP.X) > AbsoluteValue(movingEntity->dP.Y))
    {
        if (movingEntity->dP.X > 0)
        {
            movingEntity->FacingDirection = 0;
        }
        else
        {
            movingEntity->FacingDirection = 2;
        }
    }
    else
    {
        if (movingEntity->dP.Y > 0)
        {
            movingEntity->FacingDirection = 1;
        }
        else
        {
            movingEntity->FacingDirection = 3;
        }
    }
}
