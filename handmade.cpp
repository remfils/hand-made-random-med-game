#include "handmade.h"
#include "handmade_platform.cpp"
#include "handmade_assets.cpp"
#include "handmade_render_group.cpp"
#include "handmade_world.cpp"
#include "handmade_sim_region.cpp"
#include "handmade_sim_entity.cpp"
#include "handmade_audio.cpp"




// TODO: remove these
global_variable real32 tileSideInMeters = 1.3f;
global_variable real32 tileDepthInMeters = 3.0f;


struct add_low_entity_result
{
    low_entity *Low;
    uint32 Index;
};

internal add_low_entity_result
AddLowEntity(game_state *gameState, entity_type type, world_position pos)
{
    Assert(gameState->LowEntityCount < ArrayCount(gameState->LowEntities));

    add_low_entity_result result;

    uint32 ei = gameState->LowEntityCount++;

    low_entity *entityLow = gameState->LowEntities + ei;
    *entityLow = {};
    
    entityLow->Sim.Type = type;
    entityLow->WorldP = NullPosition();

    entityLow->Sim.Collision = gameState->NullCollision;

    ChangeEntityLocation(&gameState->WorldArena, gameState->World, ei, entityLow, pos);

    result.Low = entityLow;
    result.Index = ei;
    return result;
}

internal add_low_entity_result
AddGroundedEntity(game_state *gameState, entity_type type, world_position pos, sim_entity_collision_volume_group *collision)
{
    add_low_entity_result result = AddLowEntity(gameState, type, pos);
    
    result.Low->Sim.Collision = collision;
    
    return result;
}

inline void
InitHitpoints(low_entity *lowEntity, uint32 hitpointCount, uint32 filledAmount = HIT_POINT_SUB_COUNT)
{
    lowEntity->Sim.HitPointMax = hitpointCount;
    
    lowEntity->Sim.HitPoints[2] = lowEntity->Sim.HitPoints[1] = lowEntity->Sim.HitPoints[0];

    for (uint32 i=0; i<hitpointCount;++i)
    {
        lowEntity->Sim.HitPoints[i].FilledAmount = filledAmount;
    }
}

inline world_position
ChunkPositionFromTilePosition(world * world, int32 absTileX, int32 absTileY, int32 absTileZ, v3 additionalOffset=ToV3(0,0,0))
{
    world_position basePosition = {};

    v3 tileDim = ToV3(tileSideInMeters, tileSideInMeters, tileDepthInMeters);
    v3 offset = Hadamard(tileDim, ToV3((real32)absTileX, (real32)absTileY, (real32)absTileZ));

    world_position result = MapIntoChunkSpace(world, basePosition, offset + additionalOffset);

    return result;
}

internal add_low_entity_result
AddSword(game_state *gameState)
{
    // v3 dim = {25 / gameState->MetersToPixels, 63 / gameState->MetersToPixels, 0.5f};
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_Sword, NullPosition());
    
    lowEntityResult.Low->Sim.Collision = gameState->SwordCollision;

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Nonspacial);
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Movable);

    return lowEntityResult;
}

internal add_low_entity_result
AddMonster(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{   
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Monster, pos, gameState->MonsterCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    InitHitpoints(lowEntityResult.Low, 8, 1);

    return lowEntityResult;
}

internal add_low_entity_result
AddStairway(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Stairwell, pos, gameState->StairCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    lowEntityResult.Low->Sim.WalkableHeight = gameState->TypicalFloorHeight;
    lowEntityResult.Low->Sim.WalkableDim = lowEntityResult.Low->Sim.Collision->TotalVolume.Dim.xy;
    
    return lowEntityResult;
}

internal add_low_entity_result
AddStandartRoom(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Space, pos, gameState->StandardRoomCollision);
    
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Traversable);
    
    return lowEntityResult;
}

internal add_low_entity_result
AddFamiliar(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Familiar, pos, gameState->FamiliarCollision);

    lowEntityResult.Low->WorldP._Offset.x += 8;
   
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Movable);

    return lowEntityResult;
}

internal add_low_entity_result
AddPlayer(game_state *gameState)
{
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Hero, gameState->CameraPosition, gameState->PlayerCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);
    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Movable);

    InitHitpoints(lowEntityResult.Low, 3);

    if (gameState->CameraFollowingEntityIndex == 0) {
        gameState->CameraFollowingEntityIndex = lowEntityResult.Index;
    }

    add_low_entity_result swordEntity = AddSword(gameState);
    entity_reference swordRef;
    swordRef.Index = swordEntity.Index;
    lowEntityResult.Low->Sim.Sword = swordRef;

    return lowEntityResult;
}

internal add_low_entity_result
AddWall(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddGroundedEntity(gameState, EntityType_Wall, pos, gameState->WallCollision);

    AddFlag(&lowEntityResult.Low->Sim, EntityFlag_Collides);

    return lowEntityResult;
}

internal add_low_entity_result
AddDebugParticleEmmiter(game_state *gameState, uint32 absX, uint32 absY, uint32 absZ)
{
    world_position pos = ChunkPositionFromTilePosition(gameState->World, absX, absY, absZ);
    add_low_entity_result lowEntityResult = AddLowEntity(gameState, EntityType_ParticleEmmiter, pos);
    return lowEntityResult;
}



internal sim_entity_collision_volume_group *
MakeNullCollision(game_state *gameState)
{
    // TODO: fundamental types arena
    // gameState->WorldArena;

    sim_entity_collision_volume_group *group = PushStruct(&gameState->WorldArena, sim_entity_collision_volume_group);
    group->VolumeCount = 0;
    group->Volumes = 0;
    group->TotalVolume.Offset = ToV3(0,0, 0);
    // TODO: make negative ?
    group->TotalVolume.Dim = ToV3(0,0, 0);
    return group;
}

internal sim_entity_collision_volume_group *
MakeSimpleGroundedCollision(game_state *gameState, real32 x, real32 y, real32 z)
{
    // TODO: fundamental types arena
    // gameState->WorldArena;

    sim_entity_collision_volume_group *group = PushStruct(&gameState->WorldArena, sim_entity_collision_volume_group);
    group->VolumeCount = 1;
    group->Volumes = PushArray(&gameState->WorldArena, group->VolumeCount, sim_entity_collision_volume);
    group->TotalVolume.Offset = ToV3(0,0, 0.5f * z);
    group->TotalVolume.Dim = ToV3(x, y, z);
    group->Volumes[0] = group->TotalVolume;

    return group;
}

struct fill_ground_chunk_work
{
    render_group *RenderGroup;
    loaded_bitmap *OutputTarget;
    task_with_memory *Task;
    
};

internal task_with_memory*
BeginTaskWithMemory(transient_state *tranState)
{
    task_with_memory* foundTask = 0;
    for (uint32 taskIndex=0; taskIndex < ArrayCount(tranState->Tasks); taskIndex++)
    {
        task_with_memory *task = tranState->Tasks + taskIndex;
        if (!task->IsUsed) {
            foundTask = task;
            foundTask->IsUsed = true;
            foundTask->TempMemory = BeginTemporaryMemory(&task->Arena);
            break;
        }
    }
    return foundTask;
}

internal void
EndTaskWithMemory(task_with_memory *task)
{
     EndTemporaryMemory(task->TempMemory);
     task->IsUsed = false;
}

internal PLATFORM_WORK_QUEUE_CALLBACK(DoFillGrounChunkWork)
{
    fill_ground_chunk_work *work = (fill_ground_chunk_work *)data;

    SingleThreadedRenderGroup(work->OutputTarget, work->RenderGroup);

    EndTaskWithMemory(work->Task);
}

internal void
FillGroundChunk(transient_state *tranState, game_state *gameState, ground_buffer *groundBuffer, world_position chunkP)
{
#if 1
    task_with_memory *task = BeginTaskWithMemory(tranState);
    
    if (task)
    {
        fill_ground_chunk_work *work = PushStruct(&task->Arena, fill_ground_chunk_work);
        
        // TODO: how to draw ground chunk res?
    
        loaded_bitmap *drawBuffer = &groundBuffer->Bitmap;
        // TODO: (uint32)GetArenaSizeRemaining(&task->Arena) is not safe
        // TODO(vlad): why allocate group is not working here?
        render_group *renderGroup = AllocateRenderGroup(&task->Arena, tranState->Assets, 0, drawBuffer->Width, drawBuffer->Height);

        real32 width = gameState->World->ChunkDimInMeters.x;
        real32 height = gameState->World->ChunkDimInMeters.y;

        // NOTE: account for pixel margin for bilinear blend
        MakeOrthographic(renderGroup, drawBuffer->Width, drawBuffer->Height, ((real32)drawBuffer->Width - 2.0f) / width);

        groundBuffer->P = chunkP;
    
        PushDefaultColorFill(renderGroup, ToV4(0,0.3f,0,1));

        v2 halfDim = 0.5f * ToV2(width, height);

        v4 color = {1,1,1,1};

        if (chunkP.ChunkZ == 0) {
            color = {1,1,1,1};
        } else {
            color = {1.0f - (real32)chunkP.ChunkZ / 10.0f,1.0f - (real32)chunkP.ChunkZ / 10.0f,1.0f - (real32)chunkP.ChunkZ / 10.0f,1};
        }

        for (int32 chunkOffsetX =  - 1;
             chunkOffsetX <= 1;
             ++chunkOffsetX)
        {
            for (int32 chunkOffsetY = -1;
                 chunkOffsetY <= 1;
                 ++chunkOffsetY)
            {
                int32 chunkX = chunkP.ChunkX + chunkOffsetX;
                int32 chunkY = chunkP.ChunkY + chunkOffsetY;
                int32 chunkZ = chunkP.ChunkZ;

                // TODO: look into wang hashing here
                random_series series = CreateRandomSeed(139*chunkX + 593*chunkY + 329*chunkZ);
    
                v2 center = ToV2((real32)chunkOffsetX * width, (real32)chunkOffsetY * height);

                for (uint32 grassIndex=0; grassIndex < 100; grassIndex++)
                {
                    bitmap_id stamp = RandomAssetFrom(tranState->Assets, AssetType_Ground, &series);
                    
                    v2 grassCenter = center + halfDim + ToV2(width * RandomUnilateral(&series), height * RandomUnilateral(&series));
        
                    PushBitmap(renderGroup, stamp, 1.0f, ToV3(grassCenter), color);
                }
            }
        }

        for (int32 chunkOffsetX =  - 1;
             chunkOffsetX <= 1;
             ++chunkOffsetX)
        {
            for (int32 chunkOffsetY = -1;
                 chunkOffsetY <= 1;
                 ++chunkOffsetY)
            {
        
                int32 chunkX = chunkP.ChunkX + chunkOffsetX;
                int32 chunkY = chunkP.ChunkY + chunkOffsetY;
                int32 chunkZ = chunkP.ChunkZ;
            
                // TODO: look into wang hashing here
                random_series series = CreateRandomSeed(139*chunkX + 593*chunkY + 329*chunkZ);
    
                v2 center = ToV2((real32)chunkOffsetX * width, (real32)chunkOffsetY * height);

                for (uint32 grassIndex=0; grassIndex < 50; grassIndex++)
                {
                    bitmap_id stamp = RandomAssetFrom(tranState->Assets, AssetType_Grass, &series);

                    v2 grassCenter = center + halfDim + ToV2(width * RandomUnilateral(&series), height * RandomUnilateral(&series));
        
                    PushBitmap(renderGroup, stamp, 0.4f, ToV3(grassCenter));
                }
            }
        }

        if (AllBitmapsAreValid(renderGroup))
        {
            work->OutputTarget = drawBuffer;
            work->RenderGroup = renderGroup;
            work->Task = task;
        
            PlatformAPI.AddEntry(tranState->LowPriorityQueue, DoFillGrounChunkWork, work);
        }
        else {
            groundBuffer->P = NullPosition();
            EndTaskWithMemory(task);
        }
    }
#endif
}

internal void
ClearBitmap(loaded_bitmap *bmp)
{
    if (bmp->Memory) {
        int32 totalBitmapSize = bmp->Pitch * bmp->Height;
        ZeroSize(totalBitmapSize, bmp->Memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *arena, int32 width, int32 height, int32 alignX, int32 alignY, bool32 clearToZero=true)
{
    loaded_bitmap result;

    result.Width = SafeTruncateToUInt16(width);
    result.Height = SafeTruncateToUInt16(height);
    result.WidthOverHeight = SafeRatio_1((real32)result.Width, (real32)result.Height);
    result.Pitch = SafeTruncateToUInt16(width * BITMAP_BYTES_PER_PIXEL);

    result.AlignPercent = ToV2(
                             (real32)alignX / (real32)result.Width,
                             ((real32)alignY) / (real32)result.Height
                             );
    
    int32 totalBitmapSize = result.Pitch * result.Height;
    result.Memory = PushSize(arena, totalBitmapSize, 16);

    if (clearToZero) {
        ClearBitmap(&result);
    }

    return result;
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *arena, int32 width, int32 height, bool32 clearToZero=true)
{
    loaded_bitmap result = MakeEmptyBitmap(arena, width, height, 0, 0, clearToZero);

    result.WidthOverHeight = SafeRatio_1((real32)result.Width, (real32)result.Height);;
    result.AlignPercent = ToV2(0.5f, 0.5f);

    return result;
}

internal void
MakePyramidNormalMap(loaded_bitmap *bitmap, real32 roughness)
{
    real32 invWidth = 1.0f / ((real32)bitmap->Width - 1.0f);
    real32 invHeight = 1.0f / ((real32)bitmap->Height - 1.0f);

    uint8 *row = (uint8 *)bitmap->Memory;
    
    for(int32 y =0;
        y < bitmap->Height;
        y++)
    {
        uint32 *pixel = (uint32 *)row;
        
        for(int32 x =0;
            x < bitmap->Width;
            x++)
        {

            int32 yPrimeDiag = x;
            int32 yLowDiag = bitmap->Height - x;

            real32 invSqRoot = 0.707106781188f;

            v3 normal = {0,0,invSqRoot};

            if (y < yPrimeDiag) {
                if (y < yLowDiag) {
                    normal.y = -invSqRoot;
                } else if (y > yLowDiag) {
                    normal.x = -invSqRoot;
                } else {
                    normal.z = 1.0f;
                }
            } else if (y > yPrimeDiag) {
                if (y > yLowDiag) {
                    normal.y = invSqRoot;
                } else if (y < yLowDiag) {
                    normal.x = invSqRoot;
                } else {
                    normal.z = 1.0f;
                }
            } else {
                normal.z = 1.0f;
            }

            v4 color = {
                255.0f * 0.5f * (normal.x + 1.0f),
                255.0f * 0.5f * (normal.y + 1.0f),
                255.0f * 0.5f * (normal.z + 1.0f),
                255.0f * roughness
            };

            *pixel++ = (
                      ((uint32)(color.a + 0.5f) << 24)
                      | ((uint32)(color.r + 0.5f) << 16)
                      | ((uint32)(color.g + 0.5f) << 8)
                      | ((uint32)(color.b + 0.5f) << 0)
                      );
        }

        row += bitmap->Pitch;
    }
}

internal void
MakeSphereNormalMap(loaded_bitmap *bitmap, real32 roughness, real32 cx = 1.0f, real32 cy=1.0f)
{
    real32 invWidth = 1.0f / ((real32)bitmap->Width - 1.0f);
    real32 invHeight = 1.0f / ((real32)bitmap->Height - 1.0f);

    uint8 *row = (uint8 *)bitmap->Memory;
    
    for(int32 y =0;
        y < bitmap->Height;
        y++)
    {
        uint32 *pixel = (uint32 *)row;
        
        for(int32 x =0;
            x < bitmap->Width;
            x++)
        {
            v2 bitmapUV = {
                invWidth * (real32)x, invHeight * (real32)y
            };

            // TODO(vlad): check coef... cylynder doesnt work
            v3 normal = {
                cx * (2.0f * bitmapUV.x - 1.0f),
                cy * (2.0f * bitmapUV.y - 1.0f),
                0.0f
            };

            real32 rootTerm = 1.0f - Square(normal.x) - Square(normal.y);

            if (rootTerm >= 0)
            {
                normal.z = SquareRoot(rootTerm);
            }
            else
            {
                normal.x = 0.0f;
                normal.y = -1.0f;
                normal.z = 1.0f;

                normal = Normalize(normal);
            }
            
            
            v4 color = {
                255.0f * 0.5f * (normal.x + 1.0f),
                255.0f * 0.5f * (normal.y + 1.0f),
                255.0f * 0.5f * (normal.z + 1.0f),
                255.0f * roughness
            };

            *pixel++ = (
                      ((uint32)(color.a + 0.5f) << 24)
                      | ((uint32)(color.r + 0.5f) << 16)
                      | ((uint32)(color.g + 0.5f) << 8)
                      | ((uint32)(color.b + 0.5f) << 0)
                      );
        }

        row += bitmap->Pitch;
    }
}

internal void
MakeSphereDiffuseMap(loaded_bitmap *bitmap)
{
    real32 invWidth = 1.0f / ((real32)bitmap->Width - 1.0f);
    real32 invHeight = 1.0f / ((real32)bitmap->Height - 1.0f);

    uint8 *row = (uint8 *)bitmap->Memory;
    
    for(int32 y =0;
        y < bitmap->Height;
        y++)
    {
        uint32 *pixel = (uint32 *)row;
        
        for(int32 x =0;
            x < bitmap->Width;
            x++)
        {
            v2 bitmapUV = {
                invWidth * (real32)x, invHeight * (real32)y
            };

            // TODO(vlad): check coef... cylynder doesnt work
            v3 normal = {
                (2.0f * bitmapUV.x - 1.0f),
                (2.0f * bitmapUV.y - 1.0f),
                0.0f
            };

            real32 rootTerm = 1.0f - Square(normal.x) - Square(normal.y);

            real32 alpha = 0.0f;
            if (rootTerm >= 0)
            {
                alpha  = 255.0f;
            }
            
            v4 baseColor = {1,1,1,1};
            v4 color = {
                (baseColor.x),
                (baseColor.y),
                (baseColor.z),
                alpha
            };

            *pixel++ = (
                      ((uint32)(color.a + 0.5f) << 24)
                      | ((uint32)(color.r + 0.5f) << 16)
                      | ((uint32)(color.g + 0.5f) << 8)
                      | ((uint32)(color.b + 0.5f) << 0)
                      );
        }

        row += bitmap->Pitch;
    }
}

#if HANDMADE_SLOW
game_memory *DebugGlobalMemory;
#endif

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    PlatformAPI = memory->PlatformAPI;

    #if HANDMADE_SLOW
    DebugGlobalMemory = memory;
    #endif
    
    BEGIN_TIMED_BLOCK(GameUpdateAndRender);
    
    Assert(sizeof(game_state) <= memory->PermanentStorageSize);

    
    game_state *gameState = (game_state *)memory->PermanentStorage;
    
    if (!memory->IsInitialized)
    {
        uint32 groundBufferWidth = 256;
        uint32 groundBufferHeight = 256;

        // TODO: remove
        gameState->TypicalFloorHeight = 3.0f;

        gameState->ParticleEntropy = CreateRandomSeed(1234);
        
        uint32 tilesPerScreenWidth = 17;
        uint32 tilesPerScreenHeight = 9;

        real32 pixelsToMeters = 1.0f / 32.0f;
        v3 chunkDimInMeters = ToV3((real32)groundBufferWidth * pixelsToMeters, (real32)groundBufferHeight * pixelsToMeters, gameState->TypicalFloorHeight);

        
        // TODO: sub arena start partitioning memory space
        InitializeArena(&gameState->WorldArena, memory->PermanentStorageSize - sizeof(game_state), (uint8 *)memory->PermanentStorage + sizeof(game_state));
        

        AddLowEntity(gameState, EntityType_NULL, NullPosition());


        gameState->World = PushStruct(&gameState->WorldArena, world);
        InitializeWorld(gameState->World, chunkDimInMeters);

        gameState->NullCollision = MakeNullCollision(gameState);
        gameState->SwordCollision = MakeSimpleGroundedCollision(gameState, 01.f, 0.1f, 0.1f);
        gameState->StairCollision = MakeSimpleGroundedCollision(gameState, tileSideInMeters, 4.0f * tileSideInMeters, 1.2f * gameState->TypicalFloorHeight);
        gameState->PlayerCollision = MakeSimpleGroundedCollision(gameState, 0.5f, 0.4f, 0.5f);
        gameState->MonsterCollision = MakeSimpleGroundedCollision(gameState, 0.4f, 0.4f, 0.5f);
        gameState->FamiliarCollision = MakeSimpleGroundedCollision(gameState, 0.4f, 0.4f, 0.1f);
        gameState->WallCollision = MakeSimpleGroundedCollision(gameState, tileSideInMeters, tileSideInMeters, tileDepthInMeters);
        gameState->StandardRoomCollision = MakeSimpleGroundedCollision(gameState, tilesPerScreenWidth * tileSideInMeters, tilesPerScreenHeight * tileSideInMeters, 0.9f * tileDepthInMeters);
        
        gameState->XOffset = 0;
        gameState->YOffset = 0;
        
        // generate tiles
        uint32 screenBaseX = 0;
        uint32 screenX = screenBaseX;

        uint32 screenBaseY = 0;
        uint32 screenY = screenBaseY;

        uint32 screenBaseZ = 0;
        uint32 absTileZ = screenBaseZ;

        uint32 screenIndex = 10;
        
        bool isDoorLeft = false;
        bool isDoorRight = false;
        bool isDoorTop = false;
        bool isDoorBottom = false;
        bool isDoorUp = false;
        bool isDoorDown = false;
        bool32 isDoorUpAndOnLeft = false;
        
        uint32 drawZDoorCounter = 0;
        uint32 famimiliarMaxCount = 1;

        random_series generatorSeries = CreateRandomSeed(0);
        
        while (screenIndex--) {
            uint32 rand;
            if (drawZDoorCounter > 0) {
                rand = RandomChoice(&generatorSeries, 2);
            } else {
                rand = RandomChoice(&generatorSeries, 3);
            }

            // DEBUG, no up and down movement currently
            rand = RandomChoice(&generatorSeries, 3);

            rand = 3;
            
            //rand = RandomChoice(&generatorSeries, 2);

            if (rand == 0) {
                isDoorRight = true;
            }
            else if (rand == 1) {
                isDoorTop = true;
            } else {
                drawZDoorCounter = 2;
                /*
                if (absTileZ == screenBaseZ) {
                    isDoorUp = true;
                } else {
                    isDoorDown = true;
                }
                */
                // isDoorDown = true;
                isDoorUp = true;
            }
            

            AddStandartRoom(gameState, screenX * tilesPerScreenWidth + tilesPerScreenWidth/2, screenY * tilesPerScreenHeight + tilesPerScreenHeight/2, absTileZ);

            for (uint32 tileY = 0;
                 tileY < tilesPerScreenHeight;
                 ++tileY)
            {
                for (uint32 tileX = 0;
                     tileX < tilesPerScreenWidth;
                     ++tileX)
                {
                    uint32 absTileX = screenX * tilesPerScreenWidth + tileX;
                    uint32 absTileY = screenY * tilesPerScreenHeight + tileY;

                    if (famimiliarMaxCount > 0 && tileX == 4 && tileY == 4)
                    {
                        // AddFamiliar(gameState, absTileX, absTileY, absTileZ);
                        famimiliarMaxCount--;
                    }

                    bool isWallTile = false;

                    if (tileX == 0) {
                        if (tileY == (tilesPerScreenHeight / 2) && isDoorLeft) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }
                    if (tileX == (tilesPerScreenWidth -1)) {
                        if (tileY == (tilesPerScreenHeight / 2) && isDoorRight) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }
                    if (tileY == 0) {
                        if (tileX == (tilesPerScreenWidth / 2) && isDoorBottom) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }
                    if (tileY == (tilesPerScreenHeight -1)) {
                        if (tileX == (tilesPerScreenWidth / 2) && isDoorTop) {
                            isWallTile = false;
                        } else {
                            isWallTile = true;
                        }
                    }

                    if (isDoorUpAndOnLeft) {
                        if (tileY == 4 && (tileX == tilesPerScreenWidth / 2 - 2)) {
                            AddStairway(gameState, absTileX, absTileY, absTileZ);
                        }
                    } else {
                        if (tileY == 4 && (tileX == tilesPerScreenWidth / 2 + 2)) {
                            AddStairway(gameState, absTileX, absTileY, absTileZ);
                        }
                    }

                    /*
                    if (drawZDoorCounter != 0 && isDoorDown) {
                        if (tileY == (tilesPerScreenHeight / 2) && (tileX + 1 == tilesPerScreenWidth / 2)) {
                            AddStairway(gameState, absTileX, absTileY, absTileZ);
                        }
                    }
                    */
                    
                    if (isWallTile)
                    {
                        AddWall(gameState, absTileX, absTileY, absTileZ);
                    }
                }   
            }

            /*
            if (rand == 2) {
                if (isDoorUp) {
                    ++absTileZ;
                }
                else if (isDoorDown) {
                    --absTileZ;
                }
            }
            */


            absTileZ += 1;
            
            /*
            if (drawZDoorCounter > 0) {
                --drawZDoorCounter;
                
                if (drawZDoorCounter == 0) {
                    isDoorUp = false;
                    isDoorDown = false;
                } else {
                    if (isDoorDown) {
                        absTileZ -= 1.0f;
                        isDoorDown = false;
                        isDoorUp = false; // DEBUG
                    } else if (isDoorUp) {
                        absTileZ += 1.0f;
                        isDoorDown = false; // DEBUG
                        isDoorUp = false;
                    }
                }
            }
            */
            
            isDoorLeft = isDoorRight;
            isDoorBottom = isDoorTop;
            isDoorRight = false;
            isDoorTop = false;

            isDoorUpAndOnLeft = !isDoorUpAndOnLeft;

            /*
            if (rand == 0) {
                screenX += 1;
            } else if (rand == 1) {
                screenY += 1;
            }
            */
        }
        
#if HANDMADE_INERNAL
        char *fileName = __FILE__;
        debug_read_file_result fileResult = memory->DEBUG_PlatformReadEntireFile(thread, fileName);
        if (fileResult.Content)
        {
            memory->DEBUG_PlatformWriteEntireFile(thread, "test.txt", fileResult.ContentSize, fileResult.Content);
            memory->DEBUG_PlatformFreeFileMemory(thread, fileResult.Content);
        }
#endif

        world_position newCameraP = ChunkPositionFromTilePosition(gameState->World, screenBaseX*17 + 17 / 2, screenBaseY*9 + 9 / 2, screenBaseZ);
        gameState->CameraPosition = newCameraP;


        real32 screenWidth = (real32)buffer->Width;
        real32 screenHeight = (real32)buffer->Height;
        real32 maxZScale = 0.5f;
        real32 groundOverscan = 1.5f;

        // uint32 groundBufferWidth = RoundReal32ToInt32(groundOverscan * screenWidth);
        // uint32 groundBufferHeight = RoundReal32ToInt32(groundOverscan * screenHeight);
        
        gameState->GroundP = gameState->CameraPosition;
        
        AddMonster(gameState, newCameraP.ChunkX+4, newCameraP.ChunkY+4, newCameraP.ChunkZ);

        AddDebugParticleEmmiter(gameState, newCameraP.ChunkX+6, newCameraP.ChunkY+4, newCameraP.ChunkZ);

        memory->IsInitialized = true;
    }

    transient_state *tranState = (transient_state *)memory->TransientStorage;
    
    Assert(sizeof(transient_state) <= memory->TransientStorageSize);
    if (!tranState->IsInitialized) {
        InitializeArena(&tranState->TransientArena, memory->TransientStorageSize-sizeof(transient_state), (uint8 *)memory->TransientStorage + sizeof(transient_state));

        InitializeAudioState(&gameState->AudioState, &gameState->WorldArena);

        tranState->LowPriorityQueue = memory->LowPriorityQueue;
        tranState->HighPriorityQueue = memory->HighPriorityQueue;
        
        for (uint32 taskIndex=0; taskIndex < ArrayCount(tranState->Tasks); taskIndex++)
        {
            task_with_memory *task = tranState->Tasks + taskIndex;
            task->IsUsed = false;
            SubArena(&task->Arena, &tranState->TransientArena, Megabytes(3));
        }

        //LoadAssets(&tranState->Assets, tranState, thread, memory->DEBUG_PlatformReadEntireFile);

        tranState->Assets = AllocateGameAssets(&tranState->TransientArena, Kilobytes(512), tranState);

        // TODO: sound plays really bad for some reason. With stutters...
        /*
        playing_sound *snd = PlaySound(&gameState->AudioState, GetFirstSound(tranState->Assets, AssetType_PianoMusic));
        ChangeVolume(&gameState->AudioState, snd, 10, v2{0.0f, 0.0f});
        ChangePitch(snd, 1.2f);
        */

        // NOTE: make square for now
        int32 diffuseWidth = 128;
        int32 diffuseHeight = 128;
        gameState->TestDiffuse = MakeEmptyBitmap(&gameState->WorldArena, diffuseWidth, diffuseHeight, diffuseWidth / 2, diffuseHeight / 2, false);
        MakeSphereDiffuseMap(&gameState->TestDiffuse);
        //RenderRectangle(&gameState->TestDiffuse, 0,0, (real32)gameState->TestDiffuse.Width, (real32)gameState->TestDiffuse.Height, V4(0.2f, 0.2f, 0.2f, 1.0f));
        gameState->TestNormal = MakeEmptyBitmap(&gameState->WorldArena, gameState->TestDiffuse.Width, gameState->TestDiffuse.Height, diffuseWidth / 2, diffuseHeight / 2, false);
        MakeSphereNormalMap(&gameState->TestNormal, 0.0f);

        // MakePyramidNormalMap(&gameState->TestNormal, 0.0f);
        

        uint32 groundBufferWidth = 256;
        uint32 groundBufferHeight = 256;
        // tranState->GroundBufferCount = 128;
        tranState->GroundBufferCount = 128;
        tranState->GroundBuffers = PushArray(&tranState->TransientArena, tranState->GroundBufferCount, ground_buffer);
        
        for (uint32 groundBufferIndex=0;
             groundBufferIndex < tranState->GroundBufferCount;
             groundBufferIndex++)
        {
            ground_buffer *buf = tranState->GroundBuffers + groundBufferIndex;
            buf->Bitmap = MakeEmptyBitmap(&tranState->TransientArena, groundBufferWidth, groundBufferHeight, false);
            buf->P = NullPosition();
        }
        
        tranState->IsInitialized = true;

        // TODO: test fill
        // FillGroundChunk(tranState, gameState, tranState->GroundBuffers, &gameState->CameraPosition);

        tranState->EnvMapWidth = 512;
        tranState->EnvMapHeight = 256;
        for (uint32 i=0; i<ArrayCount(tranState->EnvMaps); i++) {
            render_environment_map *map = tranState->EnvMaps + i;

            uint32 width = tranState->EnvMapWidth;
            uint32 height = tranState->EnvMapHeight;
            for (uint32 j=0; j < ArrayCount(map->LevelsOfDetails); j++) {
                map->LevelsOfDetails[j] = MakeEmptyBitmap(&tranState->TransientArena, width, height, false);
                width >>= 1;
                height >>= 1;
            }
        }

        tranState->EnvMaps[0].PositionZ = -2.0f;
        tranState->EnvMaps[1].PositionZ = 0.5f;
        tranState->EnvMaps[2].PositionZ = 2.0f;
    }

    if (input->ExecutableReloaded)
    {
        uint32 groundBufferWidth = 256;
        uint32 groundBufferHeight = 256;
        for (uint32 groundBufferIndex=0;
             groundBufferIndex < tranState->GroundBufferCount;
             groundBufferIndex++)
        {
            ground_buffer *buf = tranState->GroundBuffers + groundBufferIndex;
            buf->Bitmap = MakeEmptyBitmap(&tranState->TransientArena, groundBufferWidth, groundBufferHeight, true);
            buf->P = NullPosition();
        }
        
        input->ExecutableReloaded = false;
    }
    
    world *world = gameState->World;
    for (int controllerIndex=0; controllerIndex<ArrayCount(input->Controllers); controllerIndex++)
    {
        game_controller_input *inputController = &input->Controllers[controllerIndex];

        if (!inputController->IsConnected)
        {
            continue;
        }

        controlled_hero *controlledHero = gameState->ControlledHeroes + controllerIndex;
        controlledHero->ddPRequest = {0,0};
        controlledHero->dSwordRequest = {0,0};

        if (controlledHero->StoreIndex == 0)
        {
            if (inputController->Start.EndedDown) {
                controlledHero->StoreIndex = AddPlayer(gameState).Index;
            }
        }
        else
        {
            if (inputController->IsAnalog)
            {
                controlledHero->ddPRequest = 4.0f * v2{inputController->StickAverageX, inputController->StickAverageY};

                // TODO: restore and test analog input

                // gameState->XOffset -= (int)
                // gameState->YOffset += (int)(4.0f * inputController->StickAverageY);
            
                // todo: analog movement is out of date. Should be updated to new pos
                // gameState->PlayerPosition.TileX += (int)(10.0f * inputController->StickAverageX);
                // gameState->PlayerY -= (int)(10.0f * inputController->StickAverageY);
            }
            else
            {

                if (inputController->MoveUp.EndedDown)
                {
                    controlledHero->ddPRequest.y = 1.0f;
                }
                if (inputController->MoveDown.EndedDown)
                {
                    controlledHero->ddPRequest.y = -1.0f;
                }
                if (inputController->MoveLeft.EndedDown)
                {
                    controlledHero->ddPRequest.x = -1.0f;
                }
                if (inputController->MoveRight.EndedDown)
                {
                    controlledHero->ddPRequest.x = 1.0f;
                }


            }

#if 1
            
            if (inputController->ActionUp.EndedDown)
            {
                controlledHero->dSwordRequest.y = 1;
            }
            if (inputController->ActionDown.EndedDown)
            {
                controlledHero->dSwordRequest.y = -1;
            }
            if (inputController->ActionLeft.EndedDown)
            {
                controlledHero->dSwordRequest.x = -1;
            }
            if (inputController->ActionRight.EndedDown)
            {
                controlledHero->dSwordRequest.x = 1;
            }
#else
            #endif
        }
    }

    loaded_bitmap drawBuffer_ = {};
    loaded_bitmap *drawBuffer = &drawBuffer_;
    drawBuffer->Memory = buffer->Memory;
    drawBuffer->Width = SafeTruncateToUInt16(buffer->Width);
    drawBuffer->Height = SafeTruncateToUInt16(buffer->Height);
    drawBuffer->Pitch = SafeTruncateToUInt16(buffer->Pitch);

    temporary_memory renderMemory = BeginTemporaryMemory(&tranState->TransientArena);
    // TODO: how much push buffer should be
    render_group *renderGroup = AllocateRenderGroup(&tranState->TransientArena, tranState->Assets, Megabytes(4), drawBuffer->Width, drawBuffer->Height);
    //real32 metersToPixels = ;
    real32 focalLength = 0.6f;
    real32 distanceAboveTarget = 15.0f;
    MakePerspective(renderGroup, drawBuffer->Width, drawBuffer->Height, focalLength, distanceAboveTarget);

    
    PushDefaultRenderClear(renderGroup);


    v2 screenCenter = 0.5f * ToV2((real32) drawBuffer->Width, (real32)drawBuffer->Height);

    real32 pixelsToMeters = 1.0f / 32.0f;

    rectangle2 screenBounds = GetCameraRectangleAtTarget(renderGroup);
    rectangle3 cameraBoundsInMeters = RectCenterHalfDim(ToV3(0,0,0), ToV3(screenBounds.Max, 1));

    v4 chunkColor = {1.0f,1.0f,0.0f, 1.0f};

    for (uint32 groundBufferIndex=0;
         groundBufferIndex < tranState->GroundBufferCount;
         groundBufferIndex++)
    {
        ground_buffer *buf = tranState->GroundBuffers + groundBufferIndex;
        if (IsValid(buf->P)) {
            loaded_bitmap *bmp = &buf->Bitmap;
            v3 groundDelta = Subtract(gameState->World, &buf->P, &gameState->CameraPosition);

            if (groundDelta.z > -1 && groundDelta.z < 1) {
#if 0
                PushPieceRectOutline(renderGroup, groundDelta, gameState->World->ChunkDimInMeters.xy, chunkColor);
#endif

#if 1
                PushBitmap(renderGroup, bmp, gameState->World->ChunkDimInMeters.y, groundDelta);
#endif
                
            }

        }
    }
    
    {
        world_position minChunkP = MapIntoChunkSpace(gameState->World, gameState->CameraPosition, GetMinCorner(cameraBoundsInMeters));
        world_position maxChunkP = MapIntoChunkSpace(gameState->World, gameState->CameraPosition, GetMaxCorner(cameraBoundsInMeters));

        for (int32 chunkZ=minChunkP.ChunkZ;
             chunkZ <= maxChunkP.ChunkZ;
             chunkZ++)
        {
            for (int32 chunkY=minChunkP.ChunkY;
                 chunkY <= maxChunkP.ChunkY;
                 chunkY++)
            {
                for (int32 chunkX=minChunkP.ChunkX;
                     chunkX <= maxChunkP.ChunkX;
                     chunkX++)
                {
                    world_position chunkCenter = CenteredTilePoint(chunkX, chunkY, chunkZ);
                    v3 relP = Subtract(gameState->World, &chunkCenter, &gameState->CameraPosition);

                    real32 furthestBufferLengthSq = 0.0f;

                    ground_buffer *furtherstBuffer = 0;
                    for (uint32 groundBufferIndex=0; groundBufferIndex<tranState->GroundBufferCount; groundBufferIndex++)
                    {
                        
                        ground_buffer *groundBuffer = tranState->GroundBuffers + groundBufferIndex;
                        if (AreInSameChunk(&groundBuffer->P, &chunkCenter))
                        {
                            furtherstBuffer = 0;
                            break;
                        }
                        else if (IsValid(groundBuffer->P))
                        {
                            v3 bufferP = Subtract(gameState->World, &groundBuffer->P, &gameState->CameraPosition);
                            real32 distanceToCamera = LengthSq(bufferP.xy);

                            if (furthestBufferLengthSq < distanceToCamera) {
                                furthestBufferLengthSq = distanceToCamera;
                                furtherstBuffer = groundBuffer;
                            }
                        }
                        else
                        {
                            furthestBufferLengthSq = Real32Maximum;
                            furtherstBuffer = groundBuffer;
                            
                        }
                    }

                    if (furtherstBuffer)
                    {
                        FillGroundChunk(tranState, gameState, furtherstBuffer, chunkCenter);
                    }
                }
            }
        }
    }

    // TODO: fix meters
    v3 simBoundsExpansion = ToV3(8.0f, 8.0f, 2.0f * gameState->TypicalFloorHeight);
    rectangle3 simBounds = AddRadiusTo(cameraBoundsInMeters, simBoundsExpansion);

    temporary_memory simMemory = BeginTemporaryMemory(&tranState->TransientArena);
    world_position simCenterP = gameState->CameraPosition;
    sim_region *simRegion = BeginSim(&tranState->TransientArena, gameState, gameState->World, gameState->CameraPosition, simBounds, input->DtForFrame);

    v3 cameraPosition = Subtract(gameState->World, &gameState->CameraPosition, &simCenterP);

    // render_basis *cameraBasis = PushStruct(&tranState->TransientArena, render_basis);
    // renderGroup->DefaultBasis = cameraBasis;
    // cameraBasis->P = ToV3(0,0,0);
    PushPieceRectOutline(renderGroup, ToV3(0,0,0), GetDim(screenBounds), ToV4(0.2f, 0.0f, 0.2f, 1.0f));
    PushPieceRectOutline(renderGroup, ToV3(0,0,0), GetDim(simRegion->Bounds).xy, ToV4(0.4f, 0.0f, 0.4f, 1.0f));
    PushPieceRectOutline(renderGroup, ToV3(0,0,0), GetDim(simRegion->UpdatableBounds).xy, ToV4(1.0f, 0.0f, 1.0f, 1.0f));
    
    sim_entity *simEntity = simRegion->Entities;
    for (uint32 entityIndex = 0;
         entityIndex < simRegion->EntityCount;
         ++entityIndex, ++simEntity)
    {
        if (simEntity->Updatable)
        {
            asset_vector matchVector = {};
            matchVector.E[Tag_FacingDirection] = (real32)simEntity->FacingDirection;
            asset_vector weightVector = {};
            weightVector.E[Tag_FacingDirection] = 1.0f;

            low_entity *lowEntity = gameState->LowEntities + simEntity->StorageIndex;

            // TODO: draw entities on one Z-plane

            move_spec moveSpec = DefaultMoveSpec();
            v3 ddp = {};

            //render_basis *basis = PushStruct(&tranState->TransientArena, render_basis);
            //renderGroup->DefaultBasis = basis;

            v3 cameraRelativeGroundP = GetEntityGroundPoint(simEntity) - cameraPosition;

            real32 fadeTopEndZ = 0.9f * gameState->TypicalFloorHeight;
            real32 fadeTopStartZ = 0.2f * gameState->TypicalFloorHeight;
            real32 fadeBottomStartZ = -1.2f * gameState->TypicalFloorHeight;
            real32 fadeBottomEndZ = - 2.0f *gameState->TypicalFloorHeight;

            renderGroup->GlobalAlpha = 1.0f;
            if (cameraRelativeGroundP.z > fadeTopStartZ)
            {
                renderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(fadeTopStartZ, cameraRelativeGroundP.z, fadeTopEndZ);
            }
            else if (cameraRelativeGroundP.z < fadeBottomStartZ)
            {
                renderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(fadeBottomStartZ, cameraRelativeGroundP.z, fadeBottomEndZ);
            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////
            // pre physics entity work
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            switch (simEntity->Type) {
            case EntityType_Wall:
            {
            } break;
            case EntityType_Sword:
            {
                moveSpec.UnitMaxAccelVector = false;
                moveSpec.Speed = 0;
                moveSpec.Drag = 0;

                if (simEntity->DistanceLimit == 0.0f) {
                    MakeEntityNonSpacial(simEntity);
                    ClearCollisionRulesFor(gameState, simEntity->StorageIndex);
                }
            } break;
            case EntityType_Hero:
            {
                for (uint32 idx=0; idx<ArrayCount(gameState->ControlledHeroes); idx++)
                {
                    controlled_hero *conHero = gameState->ControlledHeroes+idx;
                    if (conHero->StoreIndex == simEntity->StorageIndex)
                    {
                        moveSpec.UnitMaxAccelVector = true;
                        moveSpec.Speed = 14.0f;
                        moveSpec.Drag = 5.0f;

                        ddp = ToV3(conHero->ddPRequest);

                        if (conHero->dSwordRequest.x != 0 || conHero->dSwordRequest.y != 0)
                        {
                            sim_entity *swordEntity = simEntity->Sword.Ptr;
                            if (swordEntity && IsSet(swordEntity, EntityFlag_Nonspacial))
                            {
                                AddCollisionRule(gameState, simEntity->StorageIndex, swordEntity->StorageIndex, false);
                                MakeEntitySpacial(swordEntity, simEntity->P, 7.0f * ToV3(conHero->dSwordRequest));
                                swordEntity->DistanceLimit = 2.0f;

                                PlaySound(&gameState->AudioState, GetFirstSound(tranState->Assets, AssetType_FootstepMud));
                            }
                        }
                    }
                }
            } break;
            case EntityType_Monster:
            {
            } break;
            case EntityType_Familiar:
            {
                sim_entity *closestHero = 0;
                real32 closestHeroRadSq = Square(90.0f);

                // TODO(casey): make spacial queries
                for (uint32 entityCount = 0;
                     entityCount < simRegion->EntityCount;
                     ++entityCount)
                {
                    sim_entity *testEntity = simRegion->Entities + entityCount;
                    if (testEntity->Type == EntityType_Hero)
                    {
                        real32 testD = LengthSq(testEntity->P - simEntity->P);
                        if (testD < closestHeroRadSq)
                        {
                            closestHero = testEntity;
                            closestHeroRadSq = testD;
                        }
                    }
                }
    
                if (closestHero && closestHeroRadSq > 4.0f)
                {
                    real32 acceleration = 0.5f;
                    real32 oneOverLength = acceleration / SquareRoot(closestHeroRadSq);
        
                    ddp = oneOverLength * (closestHero->P - simEntity->P);
                }

                moveSpec.UnitMaxAccelVector = true;
                moveSpec.Speed = 100.0f;
                moveSpec.Drag = 9.0f;

                simEntity->TBobing += input->DtForFrame;
                if (simEntity->TBobing > 2.0f * Pi32)
                {
                    simEntity->TBobing -= 2.0f * Pi32;
                }
            } break;
            case EntityType_Stairwell:
            {
            } break;
            case EntityType_Space:
            {
            } break;
            }

            if (!IsSet(simEntity, EntityFlag_Nonspacial) && IsSet(simEntity, EntityFlag_Movable))
            {
                MoveEntity(gameState, simRegion, simEntity, input->DtForFrame, &moveSpec, ddp);
            }
            
            ////////////////////////////////////////////////////////////////////////////////////////////////////
            // post physics entity work
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            
            renderGroup->Transform.OffsetP = GetEntityGroundPoint(simEntity);

            switch (simEntity->Type) {
            case EntityType_ParticleEmmiter:
            {
                for (u32 particleSpawnIndex = 0; particleSpawnIndex < 1; particleSpawnIndex++)
                {
                    particle *part = gameState->Particles + gameState->NextParticle++;
                    
                    if (gameState->NextParticle >= ArrayCount(gameState->Particles)) {
                        gameState->NextParticle = 0;
                    }

                    part->P = ToV3(RandomBetween(&gameState->ParticleEntropy, -(r32)0.1f, (r32)0.1f),
                                   RandomBetween(&gameState->ParticleEntropy, -(r32)0.1f, (r32)0.1f),
                                   0);
                    part->dP = ToV3(RandomBetween(&gameState->ParticleEntropy, -(r32)0.1f, (r32)0.1f),
                                    5.0f,
                                    0.0f);
                    part->ddP = ToV3(0.0f, -9.4f, 0.0f);
                    part->Color = ToV4(1,1,1,1);
                    part->dColor = ToV4(0, 0, 0, -0.3f);
                    part->Height = RandomBetween(&gameState->ParticleEntropy, (r32)0.1f, (r32)0.2f);
                    part->BitmapId = RandomAssetFrom(tranState->Assets, AssetType_ParticleStar, &gameState->ParticleEntropy);
                }
                
                ZeroStruct(gameState->ParticleCells);

                r32 cellDim = 0.1f;
                r32 invCellDim = 1.0f / cellDim;
                v3 gridOrigin = ToV3(-0.5f * 16 * cellDim, 0.0f, 0.0f);

                for (u32 particleIndex = 0;
                     particleIndex < ArrayCount(gameState->Particles);
                     particleIndex++)
                {
                    particle *part = gameState->Particles + particleIndex;

                    v3 p = (part->P - gridOrigin) * invCellDim;

                    s32 cellX = TruncateReal32ToInt32(p.x);
                    s32 cellY = TruncateReal32ToInt32(p.y);

                    if (cellX < 0) cellX = 0;
                    if (cellX > 15) cellX = 15;
                    if (cellY < 0) cellY = 0;
                    if (cellY > 15) cellY = 15;

                    particle_cell *cell = &gameState->ParticleCells[cellY][cellX];
                    cell->Density += part->Color.a;
                    cell->VelocityTimeDensity += cell->Density * part->dP;
                }


                for (u32 cellX=0; cellX<16; cellX++)
                {
                    for (u32 cellY=0; cellY<16; cellY++)
                    {
                        particle_cell *cell = &gameState->ParticleCells[cellY][cellX];
                        
                        v3 offset = cellDim * ToV3((r32)cellX, (r32)cellY, 0.0f) + gridOrigin;
                        r32 value = Clamp01MapToRange(0.0f, cell->Density, 16.0f);
                        
                        //PushPieceRect(renderGroup, offset, ToV2(cellDim, cellDim), ToV4(value, value, value, 1.0f));
                    }
                }

                // NOTE: particle system test
                for (u32 particleIndex = 0;
                     particleIndex < ArrayCount(gameState->Particles);
                     particleIndex++)
                {
                    particle *part = gameState->Particles + particleIndex;

                    v3 p = (part->P - gridOrigin) * invCellDim;

                    s32 cellX = TruncateReal32ToInt32(p.x);
                    s32 cellY = TruncateReal32ToInt32(p.y);

                    if (cellX < 1) cellX = 1;
                    if (cellX > 14) cellX = 14;
                    if (cellY < 1) cellY = 1;
                    if (cellY > 14) cellY = 14;

                    particle_cell *cellCenter = &gameState->ParticleCells[cellY][cellX];
                    particle_cell *cellUp = &gameState->ParticleCells[cellY-1][cellX];
                    particle_cell *cellDown = &gameState->ParticleCells[cellY+1][cellX];
                    particle_cell *cellLeft = &gameState->ParticleCells[cellY][cellX-1];
                    particle_cell *cellRight = &gameState->ParticleCells[cellY][cellX+1];

                    v3 dispersion = {};
                    r32 dispersionCoef = 1.0f;

                    dispersion += dispersionCoef * (cellCenter->Density - cellLeft->Density)* ToV3(-1.0f, 0, 0);
                    dispersion += dispersionCoef * (cellCenter->Density - cellRight->Density)* ToV3(1.0f, 0, 0);
                    dispersion += dispersionCoef * (cellCenter->Density - cellUp->Density)* ToV3(0, -1.0f, 0);
                    dispersion += dispersionCoef * (cellCenter->Density - cellDown->Density)* ToV3(0.0f, 1.0f, 0);

                    v3 ddP = part->ddP + dispersion;

                    // NOTE: simulate particle forward in time

                    part->P += 0.5f * Square(input->DtForFrame) * ddP + input->DtForFrame * part->dP;
                    part->dP += input->DtForFrame * ddP;
                    part->Color += part->dColor * input->DtForFrame;

                    if (part->P.y < 0.0f) {
                        part->P.y = 0.0f;
                        part->dP.y = -0.4f*part->dP.y;
                    }

                    if (part->dP.y < 0.0f && part->P.y > 1.0f) {
                        part->ddP.x += input->DtForFrame * part->dP.x;
                    }

                    /*

                      if (part->P.y < 1.0f) {
                      part->ddP.x = part->ddP.x * 0.9f;
                      }
                    */

                    part->P.y = AbsoluteValue(part->P.y);

                    v4 pColor = part->Color;

                    pColor.a = Clamp01(pColor.a);

                    if (pColor.a > 0.9f) {
                        pColor.a = 0.9f * Clamp01MapToRange(1.0f, pColor.a, 0.9f);
                    }

                    // NOTE: render particle
                    PushBitmap(renderGroup, part->BitmapId, part->Height, part->P, part->Color);
                }
            } break;
            case EntityType_Wall:
            {
                v4 wallColor = {1,1,1,1};
                // real32 halfDepth = simEntity->Collision->TotalVolume.Dim.z * 0.5f;
                // if (simEntity->P.z < halfDepth && simEntity->P.z > -0.5f*halfDepth) {
                //     wallColor.a = 1.0f;
                // }
                
                PushBitmap(renderGroup, GetFirstBitmap(renderGroup->Assets, AssetType_WallDemo), 1.4f, ToV3(0,0, 0), wallColor);
            } break;
            case EntityType_Sword:
            {
                PushBitmap(renderGroup, GetFirstBitmap(renderGroup->Assets, AssetType_SwordDemo), 0.5f, ToV3(0,0,0));
            } break;
            case EntityType_Hero:
            {
                // TODO|NOTE: is this correct?
                hero_bitmap_ids heroBitmapIds;// = &tranState->Assets->Hero[simEntity->FacingDirection];
                heroBitmapIds.Character = BestMatchBitmap(renderGroup->Assets, AssetType_HumanBody, &matchVector, &weightVector);

                PushBitmap(renderGroup, heroBitmapIds.Character, 1.6f, ToV3(0, 0, 0));

                sim_entity_collision_volume *volume = &simEntity->Collision->TotalVolume;
                PushPieceRectOutline(renderGroup, volume->Offset, volume->Dim.xy, ToV4(0, 0.3f, 0.3f, 1.0f));

                PushHitpoints(renderGroup, simEntity);
            } break;
            case EntityType_Monster:
            {
                PushBitmap(renderGroup, GetFirstBitmap(renderGroup->Assets, AssetType_EnemyDemo), 2.0f, ToV3(0, 0, 0));

                PushHitpoints(renderGroup, simEntity);
            } break;
            case EntityType_Familiar:
            {
                PushBitmap(renderGroup, GetFirstBitmap(renderGroup->Assets, AssetType_FamiliarDemo), 2.0f, ToV3(0, 0.2f * Sin(13 * simEntity->TBobing), 0));
            } break;
            case EntityType_Stairwell:
            {
                real32 wallAlpha = 1.0f;

                real32 stairStepWidth = simEntity->WalkableDim.y / 10.0f;
                real32 stairStepDepth = gameState->TypicalFloorHeight / 10.0f;
                v2 stairSize = ToV2(simEntity->WalkableDim.x, stairStepWidth);

                PushPieceRect(renderGroup, ToV3(0, 0, 0), simEntity->WalkableDim, ToV4(0.2f,0.2f,0,1), true);

                for (uint32 stairIndex=0; stairIndex<10; stairIndex++)
                {
                    PushPieceRect(renderGroup, ToV3(0, stairIndex*stairStepWidth - 0.5f * simEntity->WalkableDim.y, stairIndex * stairStepDepth), stairSize, ToV4(1,1,0,1));
                }

                // NOTE(vlad): this is hack to make top door in z-space
                /*
                render_basis *secondBasis = PushStruct(&tranState->TransientArena, render_basis);
                secondBasis->P = GetEntityGroundPoint(simEntity);
                secondBasis->P.z += simEntity->WalkableHeight;
                renderGroup->DefaultBasis = secondBasis;
                PushPieceRect(renderGroup, ToV3(0, 0, 0), simEntity->WalkableDim, ToV4(0.5f,0.5f,0, 1.0f));
                */
            } break;
            case EntityType_Space:
            {
                for (uint32 volumeIndex=0; volumeIndex < simEntity->Collision->VolumeCount; volumeIndex++)
                {
                    sim_entity_collision_volume *volume = simEntity->Collision->Volumes + volumeIndex;
                    PushPieceRectOutline(renderGroup, volume->Offset - ToV3(0, 0, 0.5f * volume->Dim.z), volume->Dim.xy, ToV4(0.0f, 1.0f,1.0f, 1.0f));
                }
            } break;
            }


            /**
             * TODO: figure out why meters to pixesl here....

            real32 zFugde = 1.0f + 0.1f * entityBaseP.z;

            real32 entityX = screenCenter.x + entityBaseP.x * gameState->MetersToPixels * zFugde;
            real32 entityY = screenCenter.y - entityBaseP.y * gameState->MetersToPixels * zFugde;
            real32 entityZ = -entityBaseP.z * gameState->MetersToPixels;
            */
            

            if (false) // draw entity bounds
            {
                /*
                  NOTE: does NOT support any z logic

                
                color c;

                switch (simEntity->Type) {
                case EntityType_Wall: {
                    if (simEntity->P.z == 0) {
                        c.Red = 0.3f;
                        c.Green = 0.3f;
                        c.Blue = 0.3f;
                    } else if (simEntity->P.z == 1) {
                        c.Red = 0.5f;
                        c.Green = 0.5f;
                        c.Blue = 0.5f;
                    } else if (simEntity->P.z == -1) {
                        c.Red = 0.8f;
                        c.Green = 0.8f;
                        c.Blue = 0.8f;
                    } else {
                        c.Red = 0.1f;
                        c.Green = 0.1f;
                        c.Blue = 0.1f;
                    }
                } break;
                case EntityType_Stairwell: {
                    c.Red = 0.0f;
                    c.Green = (real32)simEntity->P.z / 5;
                    c.Blue = 0.0f;
                } break;
                case EntityType_Hero: {
                    c.Red = 0.0f;
                    c.Green = 0.5f;
                    c.Blue = 0.5f;
                } break;
                default: {
                    c.Red = 1.0f;
                    c.Green = 0.0f;
                    c.Blue = 0.0f;
                } break;
                }

                c.Alpha = 1.0f;

                // real32 halfW = simEntity->Dim.x * MetersToPixels * 0.5f;
                // real32 halfH = simEntity->Dim.y * MetersToPixels * 0.5f;


                // NOTE: wtf rects?
                real32 halfW = 1;
                real32 halfH = 2;

                real32 realMinX = entityX - halfW;
                real32 realMinY = entityY - halfH;
                real32 realMaxX = entityX + halfW;
                real32 realMaxY = entityY + halfH;

                if (simEntity->Type == EntityType_Hero) {
                    realMinX = realMinX - simEntity->P.z * 10.0f;
                    realMaxX = realMaxX + simEntity->P.z * 10.0f;
                }
                
                RenderRectangle(drawBuffer, realMinX, realMinY, realMaxX, realMaxY, c);
                */
            }
        }   
    }

    if (input->MouseButtons[0].EndedDown)
    {
        real32 width = 10;
        int testSpace = 50;
        v4 color = {1.0f, 0.0f, 1.0f, 1.0f};

        // TODO: mouse is in screen pixel space
        
        PushScreenSquareDot(renderGroup, V2i(input->MouseX + testSpace, input->MouseY + testSpace), width, color);
        PushScreenSquareDot(renderGroup, V2i(input->MouseX - testSpace, input->MouseY - testSpace), width, color);
        PushScreenSquareDot(renderGroup, V2i(input->MouseX + testSpace, input->MouseY - testSpace), width, color);
        PushScreenSquareDot(renderGroup, V2i(input->MouseX - testSpace, input->MouseY + testSpace), width, color);
    }
    
    if (input->MouseButtons[1].EndedDown)
    {
        real32 width = 10;
        int testSpace = 50;
        v4 color = {1.0f, 0.0f, 1.0f, 1.0f};

        // TODO: mouse is in screen pixel space
        
        PushScreenSquareDot(renderGroup, V2i(input->MouseX + testSpace, input->MouseY), width, color);
        PushScreenSquareDot(renderGroup, V2i(input->MouseX - testSpace, input->MouseY), width, color);
        PushScreenSquareDot(renderGroup, V2i(input->MouseX, input->MouseY - testSpace), width, color);
        PushScreenSquareDot(renderGroup, V2i(input->MouseX, input->MouseY + testSpace), width, color);
    }

    gameState->CurrentTime += input->DtForFrame;

    // show what is beeing sampled
    //PushPiece(renderGroup, &tranState->EnvMaps[0].LevelsOfDetails[0], V2(0,0), 0, V2(0,0));

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // render temp
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    v4 envColors[] = {
        {1.0f, 0,0, 1},
        {0, 1.0f, 0, 1},
        {0, 0, 1.0f, 1}
    };
    
    v4 blackColor = {0, 0, 0, 1};

    v2 checkerDim = {30.0f, 30.0f};

    for (uint32 mapIndex=0;
         mapIndex < ArrayCount(tranState->EnvMaps);
         mapIndex++)
    {
        render_environment_map *map = tranState->EnvMaps + mapIndex;
        loaded_bitmap *level = map->LevelsOfDetails + 0;
        v4 envColor = envColors[mapIndex];

        bool32 rowCheckerOn = false;
        for (uint32 y=0;
             y < 15;
             y++)
        {
            bool32 checkOn = rowCheckerOn;
            for (uint32 x=0;
                 x < 30;
                 x++)
            {
                v2 min = {x * checkerDim.x, y * checkerDim.y};
                v2 max = min + checkerDim;

                //RenderRectangle(level, min.x, min.y, max.x, max.y, checkOn ? blackColor : envColor);
                checkOn = !checkOn;
            }
            rowCheckerOn = !rowCheckerOn;
        }
    }

#if 0

    real32 angle = gameState->CurrentTime;
    real32 disp = 50.0f * Cos(3.0f * angle);
    real32 disp2 = 50.0f * Sin(3.0f * angle);

    /*
    disp = 0;
    disp2 = 0;
    */

    v2 origin = screenCenter - V2i(50, 50);

    v2 xAxis = (real32)gameState->TestDiffuse.Width * ToV2(Cos(angle), Sin(angle));
    v2 yAxis = Perp(xAxis);// V2i(0, gameState->TestDiffuse.Height);

    xAxis = (real32)gameState->TestDiffuse.Width * ToV2(1.0f, 0.0);
    yAxis = Perp(xAxis);

    // v2 xAxis = V2(200.0f, 0);
    // v2 yAxis = V2(0, 120.0f);
    // v4 color = { (Sin(4.0f * angle) + 1.0f) / 2.0f, 0.0f, 1, (Cos(4.0f * angle) + 1.0f) / 2.0f};
    v4 color = {1.0f, 1.0f, 1.0f, 1.0f};

    v2 mapP = ToV2(20.0f, 20.0f);
    
    /*
    xAxis = (real32)gameState->TestDiffuse.Width * V2(Cos(angle), Sin(angle));
    yAxis = Perp(xAxis);// V2i(0, gameState->TestDiffuse.Height);
    */
    
    PushCoordinateSystem(renderGroup, origin - 0.01f * disp * xAxis - 0.01f * disp2 * yAxis, xAxis, yAxis, color,
                         &(gameState->TestDiffuse), &gameState->TestNormal,
                         tranState->EnvMaps+2, tranState->EnvMaps+1, tranState->EnvMaps+0);

    PushCoordinateSystem(renderGroup, origin + 0.02f * disp2 * xAxis + 0.02f * disp * yAxis, xAxis, yAxis, color,
                         &(gameState->TestDiffuse), &gameState->TestNormal,
                         tranState->EnvMaps+2, tranState->EnvMaps+1, tranState->EnvMaps+0);

    for (int32 i=0; i < ArrayCount(tranState->EnvMaps); i++) {
        render_environment_map *map = tranState->EnvMaps + i;
        loaded_bitmap *level = map->LevelsOfDetails + 0;
        xAxis = 0.5 * V2i(level->Width, 0);
        yAxis = 0.5 * V2i(0, level->Height);
        
        PushCoordinateSystem(renderGroup, mapP, xAxis, yAxis, color, level, 0,0,0,0);

        mapP += yAxis + ToV2(0.0f, 6.0f);
    }

    #endif
    
    /*
    PushCoordinateSystem(renderGroup, origin - 0.01f * disp * xAxis - 0.01f * disp2 * yAxis, xAxis, yAxis, color,
                         &(gameState->TestNormal), 0,
                         0, 0, 0);
    */

    // PushSaturationFilter(renderGroup, (1.0f + Cos(4.0f * angle) * 0.5f));
    // PushSaturationFilter(renderGroup, 0.5f + Cos(2.0f * angle) * 0.5f);

    TiledRenderGroup(tranState->HighPriorityQueue, drawBuffer, renderGroup);


    EndSim(simRegion, gameState);

    EndTemporaryMemory(simMemory);
    EndTemporaryMemory(renderMemory);

    EvictAssetsAsNecessary(tranState->Assets);
    
    CheckArena(&gameState->WorldArena);
    CheckArena(&tranState->TransientArena);

    END_TIMED_BLOCK(GameUpdateAndRender);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *gameState = (game_state *)memory->PermanentStorage;
    transient_state *tranState = (transient_state *)memory->TransientStorage;
    OutputPlayingSounds(&gameState->AudioState, soundBuffer, tranState->Assets, &tranState->TransientArena);
}
