#define IvalidP ToV3(10000.0f, 10000.0f, 10000.0f)

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
MakeEntitySpacial(sim_entity *simEntity, v3 pos, v3 dP)
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

inline v3
GetSimSpaceP(sim_region *simRegion, low_entity *stored)
{
    // TODO(casey): set to singlaling NAN in debug mode?
    v3 result = IvalidP;

    if (!IsSet(&stored->Sim, EntityFlag_Nonspacial))
    {
        result = Subtract(simRegion->World, &stored->WorldP, &simRegion->Origin);
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
AddEntity(game_state * gameState, sim_region *region, uint32 sourceIndex, low_entity *source, v3 *simP);
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
            v3 pos = GetSimSpaceP(simRegion, low);
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

inline bool32
EntityOverlapsRectangle(v3 entityP, sim_entity_collision_volume volume, rectangle3 rect)
{
    rectangle3 grown = AddRadiusTo(rect, 0.5 * volume.Dim);
    bool32 result = IsInRectangle(grown, entityP + volume.Offset);
    return result;
}

internal sim_entity*
AddEntity(game_state * gameState, sim_region *simRegion, uint32 sourceIndex, low_entity *source, v3 *simP)
{
    sim_entity *dest = AddEntityRaw(gameState, simRegion, sourceIndex, source);
    if (dest)
    {
        if (simP)
        {
            dest->P = *simP;
            dest->Updatable = EntityOverlapsRectangle(dest->P, dest->Collision->TotalVolume, simRegion->UpdatableBounds);
        }
        else
        {
            dest->P = GetSimSpaceP(simRegion, source);
        }
    }
    
    return dest;
}

internal sim_region*
BeginSim(memory_arena *simArena, game_state *gameState, world *world, world_position regionCenter, rectangle3 regionBounds, real32 dt)
{
    TIMED_FUNCTION;

    sim_region *simRegion = PushStruct(simArena, sim_region);
    ZeroStruct(simRegion->Hash);

    // TODO: figure out what this is = max(speed + width) on all entities
    // TODO: enforce these
    simRegion->MaxEntityRadius = 5.0f;
    simRegion->MaxEntityVelocity = 30.0f;
    real32 updateSafetyMargin = simRegion->MaxEntityRadius + dt * simRegion->MaxEntityVelocity;
    real32 updateSafetyMarginZ = 1.0f;


    simRegion->MaxEntityCount = 4096; // TODO: how many?
    simRegion->EntityCount = 0;
    simRegion->Entities = PushArray(simArena, simRegion->MaxEntityCount, sim_entity);

    simRegion->World = world;
    simRegion->Origin = regionCenter;
    simRegion->UpdatableBounds = AddRadiusTo(regionBounds, ToV3(simRegion->MaxEntityRadius, simRegion->MaxEntityRadius, 0.0f));
    simRegion->Bounds = AddRadiusTo(simRegion->UpdatableBounds, ToV3(updateSafetyMargin, updateSafetyMargin, updateSafetyMarginZ));

    world_position minChunkP = MapIntoChunkSpace(simRegion->World, simRegion->Origin, GetMinCorner(simRegion->Bounds));
    world_position maxChunkP = MapIntoChunkSpace(simRegion->World, simRegion->Origin, GetMaxCorner(simRegion->Bounds));

    for (int32 chunkZ=minChunkP.ChunkZ;
         chunkZ <= maxChunkP.ChunkZ;
         chunkZ++) {
        for (int32 chunkY=minChunkP.ChunkY;
             chunkY <= maxChunkP.ChunkY;
             chunkY++) {
            for (int32 chunkX=minChunkP.ChunkX;
                 chunkX <= maxChunkP.ChunkX;
                 chunkX++) {
                world_chunk *chunk = GetWorldChunk(simRegion->World, chunkX, chunkY, chunkZ);
                if (chunk) {
                    for (world_entity_block *block = &chunk->FirstBlock;
                         block;
                         block = block->Next) {
                        for (uint32 entityIndex = 0;
                             entityIndex < block->EntityCount;
                             ++entityIndex) {
                            uint32 lowEntityIndex = block->LowEntityIndex[entityIndex];
                            low_entity *low = gameState->LowEntities + lowEntityIndex;

                            if (!IsSet(&low->Sim, EntityFlag_Nonspacial))
                            {
                                v3 simSpaceP = GetSimSpaceP(simRegion, low);

                                if (EntityOverlapsRectangle(simSpaceP, low->Sim.Collision->TotalVolume, simRegion->Bounds))
                                {
                                    AddEntity(gameState, simRegion, lowEntityIndex, low, &simSpaceP);
                                }    
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
    TIMED_FUNCTION;
    
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
                    oldBlock = PushStruct(arena, world_entity_block);
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
    TIMED_FUNCTION;

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
            if (simEntity->P.y > (5 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkY += 9;
            }
            if (simEntity->P.y < -(5 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkY -= 9;
            }
            
            if (simEntity->P.x > (9 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkX += 17;
            }
            if (simEntity->P.x < -(9 * world->TileSideInMeters))
            {
                newCameraPosition.ChunkX -= 17;
            }
#else
            //real32 offsetZ = gameState->CameraPosition._Offset.z;
            gameState->CameraPosition = stored->WorldP;
            //gameState->CameraPosition._Offset.z = offsetZ;
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
ClearCollisionRulesFor(game_state *gameState, uint32 storageIndex)
{
    // TODO: better clear rule handling

    for (uint32 hashBucket = 0;
         hashBucket < ArrayCount(gameState->CollisionRuleHash);
         hashBucket++)
    {
        for (pairwise_collision_rule **rule = &gameState->CollisionRuleHash[hashBucket];
             *rule;
             )
        {
            pairwise_collision_rule *nextRule = (*rule)->NextInHash;
            if (((*rule)->StorageIndexA == storageIndex) ||
                ((*rule)->StorageIndexB == storageIndex))
            {
                pairwise_collision_rule *removedRule = *rule;
                *rule = (*rule)->NextInHash;

                removedRule->NextInHash = gameState->FirstFreeCollisionRule;
                gameState->FirstFreeCollisionRule = removedRule;
            }
            else
            {
                rule = &(*rule)->NextInHash;
            }
        }
    }
}

internal void
AddCollisionRule(game_state * gameState, uint32 storageIndexA, uint32 storageIndexB, bool32 canCollide)
{
    if (storageIndexA > storageIndexB)
    {
        uint32 temp = storageIndexA;
        storageIndexA = storageIndexB;
        storageIndexB = temp;
    }

    // TODO: beter hash function
    pairwise_collision_rule *foundRule = 0;
    uint32 hashBucket = storageIndexA  & (ArrayCount(gameState->CollisionRuleHash) -1);
    for (pairwise_collision_rule *rule = gameState->CollisionRuleHash[hashBucket];
         rule;
         rule = rule->NextInHash)
    {
        if (rule->StorageIndexA == storageIndexA) {
            foundRule = rule;
            break;
        }
    }

    if (!foundRule)
    {
        foundRule = gameState->FirstFreeCollisionRule;

        if (foundRule) {
            gameState->FirstFreeCollisionRule = foundRule->NextInHash;
        }
        else
        {
            foundRule = PushStruct(&gameState->WorldArena, pairwise_collision_rule);
        }
        foundRule->NextInHash = gameState->CollisionRuleHash[hashBucket];
        gameState->CollisionRuleHash[hashBucket] = foundRule;
    }

    if (foundRule)
    {
        foundRule->StorageIndexA = storageIndexA;
        foundRule->StorageIndexB = storageIndexB;
        foundRule->CanCollide = canCollide;
    }
}

// rename CanCollid
internal bool32
CanCollide(game_state * gameState, sim_entity *a, sim_entity *b)
{
    bool32 result = false;
    if (a != b)
    {
        if (IsSet(a, EntityFlag_Collides) && IsSet(b, EntityFlag_Collides)) {
            if ((!IsSet(a, EntityFlag_Nonspacial)) && (!IsSet(b, EntityFlag_Nonspacial)))
            {
                result = true;
            }

            if (a->StorageIndex > b->StorageIndex)
            {
                sim_entity *temp = a;
                a = b;
                b = temp;
            }

            if (
                (a->Type == EntityType_Stairwell && b->Type == EntityType_Hero)
                || (b->Type == EntityType_Stairwell && a->Type == EntityType_Hero))
            {
                int32 debug = 3;
            }

            // TODO: beter hash function
            uint32 hashBucket = a->StorageIndex  & (ArrayCount(gameState->CollisionRuleHash) -1);
            for (pairwise_collision_rule *rule = gameState->CollisionRuleHash[hashBucket];
                 rule;
                 rule = rule->NextInHash)
            {
                if (rule->StorageIndexA == a->StorageIndex) {
                    result = rule->CanCollide;
                    break;
                }
            }
        }
    }

    return result;
}

internal bool32
HandleCollision(game_state *gameState, sim_entity *a, sim_entity *b)
{
    bool32 stopsOnCollision = false;

    if (a->Type == EntityType_Sword)
    {
        stopsOnCollision = false;
        AddCollisionRule(gameState, a->StorageIndex, b->StorageIndex, false);
    }
    else
    {
        stopsOnCollision = true;
    }

    if (a->Type > b->Type)
    {
        sim_entity *temp = a;
        a = b;
        b = temp;
    }

    if (a->Type == EntityType_Monster && b->Type == EntityType_Sword)
    {
        a->HitPointMax -= 1;
    }

    // TODO: stops on collision
    return stopsOnCollision;
}

inline v3
GetEntityGroundPoint(sim_entity *entity, v3 forEnityP)
{
    v3 result = forEnityP;
    return result;
}

inline v3
GetEntityGroundPoint(sim_entity *entity)
{
    v3 result = GetEntityGroundPoint(entity, entity->P);
    return result;
}

inline real32
GetEntityPositionOnStairs(sim_entity *stairsEntity, v3 entityGroundPosition)
{
    rectangle2 regionRect = RectCenterDim(stairsEntity->P.xy, stairsEntity->WalkableDim);
    v2 bary = Clamp01(GetBarycentric(regionRect, entityGroundPosition.xy));

    real32 result = 0.0f;
    result = stairsEntity->P.z + bary.y * stairsEntity->WalkableHeight;

    return result;
}

internal bool32
TestSpeculativeCollision(sim_entity *movingEntity, sim_entity *region, v3 testPoint)
{
    bool32 result = true;

    if (region->Type == EntityType_Stairwell) {
        

        // TODO: no SafeRatio_0
        //real32 ground = SafeRatio_0(Lerp(regionRect.Min.z, bary.y, regionRect.Max.z), (regionRect.Max.z - regionRect.Min.z));
        v3 groundPoint = GetEntityGroundPoint(movingEntity);
        real32 ground = GetEntityPositionOnStairs(region, groundPoint);
        
        real32 stepHeight = 0.1f;
        result = (AbsoluteValue(groundPoint.z - ground) > stepHeight);
    }

    return result;
}

internal bool32
EntitiesOverlap(sim_entity *movingEntity, sim_entity *testEntity, v3 epsilon=ToV3(0,0,0))
{
    bool32 result = false;
    for (uint32 movingVolumeIndex=0;
         !result && movingVolumeIndex < movingEntity->Collision->VolumeCount;
         movingVolumeIndex++)
    {
        sim_entity_collision_volume *movingVol = movingEntity->Collision->Volumes + movingVolumeIndex;
        rectangle3 movingEntityRect = RectCenterDim(movingEntity->P + movingVol->Offset, movingVol->Dim + epsilon);
                                
        for (uint32 testVolumeIndex=0;
             !result && testVolumeIndex < testEntity->Collision->VolumeCount;
             testVolumeIndex++)
        {
            sim_entity_collision_volume *testVol = testEntity->Collision->Volumes + testVolumeIndex;

            rectangle3 testEntityRect = RectCenterDim(testEntity->P + testVol->Offset, testVol->Dim);
            result = RectanglesIntersect(movingEntityRect, testEntityRect);
        }
    }

    return result;
}

internal bool32
CanOverlap(game_state * gameState, sim_entity *moving, sim_entity *region)
{
    bool32 result = false;

    if (moving != region) {
        if (region->Type == EntityType_Stairwell) {
            result = true;
        }
    }

    return result;
}

internal void
HandleOverlap(game_state * gameState, sim_entity *movingEntity, sim_entity *region, real32 dt, real32 *ground)
{
    if (region->Type == EntityType_Stairwell) {
        v3 groundPoint = GetEntityGroundPoint(movingEntity);
        *ground = GetEntityPositionOnStairs(region, groundPoint);
    }
}

internal void
MoveEntity(game_state *gameState, sim_region *simRegion, sim_entity *movingEntity, real32 dt, move_spec *moveSpec, v3 ddPosition)
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
    v3 oldEntityPosition = movingEntity->P;
    v3 entityPositionDelta = 0.5f * dt * dt * ddPosition + movingEntity->dP * dt;
    movingEntity->dP = ddPosition * dt + movingEntity->dP;

    // TODO: cap velocity
    Assert(LengthSq(movingEntity->dP) <= Square(simRegion->MaxEntityVelocity));

    v3 newEntityPosition = oldEntityPosition + entityPositionDelta;


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
        real32 tMax = 0.0f;
        real32 entityDeltaLength = Length(entityPositionDelta);

        if (entityDeltaLength > 0)
        {
            if (entityDeltaLength > distanceRemaining)
            {
                tMin = distanceRemaining / entityDeltaLength;
            }
        
            v3 wallNormalMin = {0,0};
            v3 wallNormalMax = {0,0};
            sim_entity *hitEntityMin = 0;
            sim_entity *hitEntityMax = 0;

            v3 desiredPosition = movingEntity->P + entityPositionDelta;

            if (!IsSet(movingEntity, EntityFlag_Nonspacial))
            {

                // TODO: spacial partion
                for (uint32 testEntityIndex = 0;
                     testEntityIndex < simRegion->EntityCount;
                     ++testEntityIndex)
                {
                    v3 overlapEpsilon = v3{0.001f, 0.001f, 0.001f};
                    sim_entity *testEntity = simRegion->Entities + testEntityIndex;

                    if ((IsSet(testEntity, EntityFlag_Traversable) && EntitiesOverlap(movingEntity, testEntity, overlapEpsilon)) || CanCollide(gameState, movingEntity, testEntity))
                    {
                        for (uint32 movingVolumeIndex=0;
                             movingVolumeIndex < movingEntity->Collision->VolumeCount;
                             movingVolumeIndex++)
                        {
                            sim_entity_collision_volume *movingVol = movingEntity->Collision->Volumes + movingVolumeIndex;
                                
                            for (uint32 testVolumeIndex=0;
                                 testVolumeIndex < testEntity->Collision->VolumeCount;
                                 testVolumeIndex++)
                            {
                                sim_entity_collision_volume *testVol = testEntity->Collision->Volumes + testVolumeIndex;
                                    
                                // TODO: entity have height
                                v3 minkovskiDiameter = {testVol->Dim.x + movingVol->Dim.x,
                                                        testVol->Dim.y + movingVol->Dim.y,
                                                        testVol->Dim.z + movingVol->Dim.z};

                                v3 minCorner = -0.5f * minkovskiDiameter;
                                v3 maxCorner = 0.5f * minkovskiDiameter;

                                v3 rel = (movingEntity->P + movingVol->Offset) - (testEntity->P + testVol->Offset);

                                // TODO: check <= or <
                                if ((rel.z >= minCorner.z) && (rel.z < maxCorner.z))
                                {
                                    sim_entity *testHitEntity = 0;

                                    test_wall walls[] = {
                                        {minCorner.x, rel.x, rel.y, entityPositionDelta.x, entityPositionDelta.y, minCorner.y, maxCorner.y, v3{-1,0,0}},
                                        {maxCorner.x, rel.x, rel.y, entityPositionDelta.x, entityPositionDelta.y, minCorner.y, maxCorner.y, v3{1,0,0}},
                                        {minCorner.y, rel.y, rel.x, entityPositionDelta.y, entityPositionDelta.x, minCorner.x, maxCorner.x, v3{0,-1,0}},
                                        {maxCorner.y, rel.y, rel.x, entityPositionDelta.y, entityPositionDelta.x, minCorner.x, maxCorner.x, v3{0,1,0}}
                                    };
                                    
                                    real32 tEpsilon = 0.001f;

                                    if (IsSet(testEntity, EntityFlag_Traversable))
                                    {
                                        real32 testTMax = tMax;
                                        v3 testWallNormalMax = {0,0};
                                            
                                        for (uint32 wallIndex =0;
                                             wallIndex < ArrayCount(walls);
                                             wallIndex++)
                                        {
                                            test_wall *wall = walls + wallIndex;

                                            if (wall->DeltaX != 0.0f)
                                            {
                                                real32 tResult = (wall->X - wall->RelX) / wall->DeltaX;
                                                real32 y = wall->RelY + tResult * wall->DeltaY;
                                                if ((tResult >= 0.0f) && (testTMax < tResult))
                                                {
                                                    if ((y >= wall->MinY) && (y <= wall->MaxY))
                                                    {
                                                        testTMax = Maximum(0, tResult-tEpsilon);
                                                        testWallNormalMax = wall->Normal;
                                                        testHitEntity = testEntity;
                                                    }
                                                }
                                            }
                                        }

                                        if (testHitEntity) {
                                            tMax = testTMax;
                                            wallNormalMax = testWallNormalMax;
                                            hitEntityMax = testHitEntity;
                                        }
                                    }
                                    else
                                    {
                                        real32 testTMin = tMin;
                                        v3 testWallNormalMin = {0,0};

                                        for (uint32 wallIndex =0;
                                             wallIndex < ArrayCount(walls);
                                             wallIndex++)
                                        {
                                            test_wall *wall = walls + wallIndex;

                                            if (wall->DeltaX != 0.0f)
                                            {
                                                real32 tResult = (wall->X - wall->RelX) / wall->DeltaX;
                                                real32 y = wall->RelY + tResult * wall->DeltaY;
                                                if ((tResult >= 0.0f) && (testTMin > tResult))
                                                {
                                                    if ((y >= minCorner.y) && (y <= maxCorner.y))
                                                    {
                                                        testTMin = Maximum(0.0f, tResult - tEpsilon);
                                                        testWallNormalMin = wall->Normal;
                                                        testHitEntity = testEntity;
                                                    }
                                                }
                                            }
                                        }

                                        if (testHitEntity)
                                        {
                                            v3 testP = movingEntity->P + testTMin * entityPositionDelta;
                                            if (TestSpeculativeCollision(movingEntity, testHitEntity, testP))
                                            {
                                                tMin = testTMin;
                                                wallNormalMin = testWallNormalMin;
                                                hitEntityMin = testHitEntity;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            v3 wallNormal = {0,0};
            sim_entity *hitEntity = 0;
            real32 tStop;
            if (tMin < tMax)
            {
                tStop = tMin;
                hitEntity = hitEntityMin;
                wallNormal = wallNormalMin;
            }
            else
            {
                tStop = tMax;
                hitEntity = hitEntityMax;
                wallNormal = wallNormalMax;
            }

            movingEntity->P += tStop * entityPositionDelta;
            distanceRemaining -= tStop * entityDeltaLength;
            if (hitEntity)
            {
                entityPositionDelta = desiredPosition - movingEntity->P;

                bool32 stopsAtCollision = HandleCollision(gameState, movingEntity, hitEntity);

                if (stopsAtCollision)
                {
                    entityPositionDelta = entityPositionDelta - 1 * Inner(entityPositionDelta, wallNormal) * wallNormal;
                    movingEntity->dP = movingEntity->dP - 1 * Inner(movingEntity->dP, wallNormal) * wallNormal;
                }
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

    real32 ground = 0.0f;

    // TODO: move to collission loop for percise detection
    // NOTE: handle overlapping events
    
    for (uint32 testEntityIndex = 0;
         testEntityIndex < simRegion->EntityCount;
         ++testEntityIndex)
    {
        sim_entity *testEntity = simRegion->Entities + testEntityIndex;
        if (CanOverlap(gameState, movingEntity, testEntity) && EntitiesOverlap(movingEntity, testEntity))
        {
            HandleOverlap(gameState, movingEntity, testEntity, dt, &ground);
        }
    }

    movingEntity->P.z = ground + (movingEntity->P.z - GetEntityGroundPoint(movingEntity).z);

    // if (movingEntity->P.z < ground) {
    //     movingEntity->P.z = ground;
    //     movingEntity->dP.z = 0;

    //     AddFlag(movingEntity, EntityFlag_ZSupported);
    // } else {
    //     ClearFlag(movingEntity, EntityFlag_ZSupported);
    // }

    if (movingEntity->DistanceLimit != 0)
    {
        if (distanceRemaining == 0.0f) {
            int a = 0 ;
        }
        movingEntity->DistanceLimit = distanceRemaining;
    }
    else
    {
        movingEntity->DistanceLimit = 0.0f;
    }
    
    if (movingEntity->dP.x == 0 && movingEntity->dP.y == 0)
    {
        // dont change direction if both zero
    }
    else
    {
        movingEntity->FacingDirection = ATan2(movingEntity->dP.y, movingEntity->dP.x);
    }
}
