
inline v2
GetSimSpaceP(sim_region *simRegion, low_entity *stored)
{
    world_diff diff = Subtract(simRegion->World, &stored->Position, &simRegion->Origin);
    v2 result = diff.dXY;
    return result;
}


internal sim_entity*
AddEntity(sim_region *simRegion)
{
    sim_entity *simEntity = 0;
    if (simRegion->EntityCount < simRegion->MaxEntityCount)
    {
        simEntity = simRegion->Entities + simRegion->EntityCount;
        simRegion->EntityCount++;

        // TODO: clear entity
        simEntity = {};
    }   
    else
    {
        InvalidCodePath;
    }

    return simEntity;
}

internal sim_entity*
AddEntity(sim_region *region, low_entity *source, v2 *simP)
{
    sim_entity *dest = AddEntity(region);
    if (dest)
    {
        // TODO: convert this thing

        if (simP)
        {
            dest->P = *simP;
        }
        else
        {
            dest->P = GetSimSpaceP(region, source);
        }
    }
    
    return dest;
}



internal sim_region*
BeginSim(memory_arena *simArena, game_state *gameState, world_position regionCenter, rectangle2 regionBounds)
{
    sim_region *simRegion = PushSize(simArena, sim_region);

    simRegion->MaxEntityCount = 4096; // TODO: how many?
    simRegion->EntityCount = 0;
    simRegion->Entities = PushArray(simArena, simRegion->MaxEntityCount, sim_entity);

    simRegion->World = gameState->World;
    simRegion->Origin = regionCenter;
    simRegion->Bounds = regionBounds;

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
                        v2 simSpaceP = GetSimSpaceP(simRegion, low);

                        if (IsInRectangle(simRegion->Bounds, simSpaceP))
                        {
                            AddEntity(simRegion, low, &simSpaceP);
                        }
                    }
                }
            }
        }
    }
}

// TODO: low entities in world
internal void
EndSim(sim_region *region, game_state *gameState)
{
    sim_entity * simEntity = region->Entities;
    for (uint32 entityIndex=0;
         entityIndex < region->EntityCount;
         ++entityIndex, ++simEntity)
    {
        low_entity * stored = gameState->LowEntities + simEntity->StorageIndex;

        world_position newPosition = MapIntoChunkSpace(region->World, region->Origin, simEntity->P);

        // TODO: save state to store
        ChangeEntityLocation(&gameState->WorldArena, region->World, simEntity->StorageIndex, stored, stored->Position, &newPosition);
    }


    entity cameraFollowingEntity = ForceHighEntity(gameState, gameState->CameraFollowingEntityIndex, 0);
    newCameraPosition.ChunkZ = cameraFollowingEntity.Low->Position.ChunkZ;


#if 0
    if (cameraFollowingEntity.High->Position.Y > (5 * world->TileSideInMeters))
    {
        newCameraPosition.ChunkY += 9;
    }
    if (cameraFollowingEntity.High->Position.Y < -(5 * world->TileSideInMeters))
    {
        newCameraPosition.ChunkY -= 9;
    }
            
    if (cameraFollowingEntity.High->Position.X > (9 * world->TileSideInMeters))
    {
        newCameraPosition.ChunkX += 17;
    }
    if (cameraFollowingEntity.High->Position.X < -(9 * world->TileSideInMeters))
    {
        newCameraPosition.ChunkX -= 17;
    }
#else
    newCameraPosition = cameraFollowingEntity.Low->Position;
#endif
}
