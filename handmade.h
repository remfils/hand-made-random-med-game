#if !defined(HANDMADE_H)
#define HANDMADE_H



/*
  HANDMADE_INTERNAL:
  	0 - build for public release
        1 - build for developer ony

  HANDMADE_SLOW:
  	0 - no slow code is allowed
        1 - can be less performant
 */



/* 

   TODO: 

   ARCHITECTURE EXPLORATION

   - Rendering
     - coordinates
       - screen
       - world
       - texture
     - particles
     - lighting
     - final optimisations
   
   - Audio
     - sound effect triggers
     - ambient sound
     - music
   - Asset streaming

   - Collision detection
     - detect entry/exit
     - shape defenition
   - Implement multiple sim region
     - per entity clocking
     - sim region merging for multiple players

   - Debug code
     - logging
     - diagrams
     - a little gui, but only a little

   - Z!

   - Rudimentary world generation (just what is needed to be done)
     - placement of bg things
     - connectivity
     - non-overlaping
     - map display
   - Meta game / save game
     - how to enter save slot
     - persistent unlocks
     - do we allow saved games? probably yes, just for pausing
     - continuous save for crash recovery
   - AI
     - rudamentary monster behavior
     - pathfinding
     - AI "storage"
   
   - Animation, should lead into rendering
     - skeletal animation
     - particle systems

   PRODUCTION
   - Rendering
   - Entity system
   - World generation
     
 */

// TODO: remove
#include <math.h>

#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_file_formats.h"
#include "handmade_assets.h"
#include "handmade_render_group.h"
#include "handmade_audio.h"
#include "random.h"



#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#define PushStruct(arena, type, ...) (type *)PushSize_(arena, sizeof(type), ##__VA_ARGS__)
#define PushArray(arena, count,  type, ...) (type *)PushSize_(arena, count * sizeof(type), ##__VA_ARGS__)
#define PushSize(arena, size, ...) PushSize_(arena, size, ##__VA_ARGS__)

/* void Debug_PlatformFreeFileMemory(void *memory); */
/* debug_read_file_result Debug_PlatformReadEntireFile(char *filename); */
/* bool32 Debug_PlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);  */





inline game_controller_input *GetController(game_input *input, int controllerIndex)
{
    Assert(controllerIndex < ArrayCount(input->Controllers));
    return &input->Controllers[controllerIndex];
};


#include "handmade_world.h"
#include "handmade_sim_region.h"


/*
struct high_entity
{
    v2 Position;
    uint32 AbsTileZ;
    v2 dPosition;
    uint32 FacingDirection;

    real32 TBobing;

    uint32 LowEntityIndex;
};
*/

struct low_entity
{
    world_position WorldP;
    sim_entity Sim;
};

enum entity_residence
{
    EntityResidence_Nonexistant,
    EntityResidence_Dormant,
    EntityResidence_Low,
    EntityResidence_High,
};

/*
struct entity
{
    uint32 LowIndex;
    low_entity *Low;
    high_entity *High;
};
*/

struct low_entity_chunk_reference
{
    world_chunk *TileChunk;
    uint32 EntityIndexChunk;
};


struct controlled_hero
{
    uint32 StoreIndex;
    v2 ddPRequest;
    v2 dSwordRequest;
};

enum pairwise_collision_rule_flag
{
    PairCollisionFlag_CanCollide = 1 << 1,
    PairCollisionFlag_Temporary = 1 << 2,
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32 StorageIndexA;
    uint32 StorageIndexB;

    pairwise_collision_rule * NextInHash;
};

struct hero_bitmap_ids
{
    bitmap_id Character;
};

struct ground_buffer
{
    world_position P; // NOTE: this is center of bitmap
    loaded_bitmap Bitmap;
};

struct particle_cell
{
    r32 Density;
    v3 VelocityTimeDensity;
};

struct particle
{
    v3 P;
    v3 dP;
    v3 ddP;
    v4 dColor;
    v4 Color;
    r32 Height;
    bitmap_id BitmapId;
};

struct game_state
{
    int XOffset;
    int YOffset;

    memory_arena WorldArena;
    memory_arena TransientArena;

    real32 tSine;

    real32 CurrentTime;

    real32 TypicalFloorHeight;

    world * World;

    // TODO: what to do with multiple players?
    uint32 CameraFollowingEntityIndex;
    world_position CameraPosition;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    uint32 LowEntityCount;
    low_entity LowEntities[100000];
    
    world_position GroundP;    
    
    // must be a power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *PlayerCollision;
    sim_entity_collision_volume_group *MonsterCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *StandardRoomCollision;

    loaded_bitmap TestDiffuse;
    loaded_bitmap TestNormal;

    audio_state AudioState;

    u32 NextParticle;
    particle Particles[64];
    particle_cell ParticleCells[16][16];
    random_series ParticleEntropy;

    
};

struct transient_state
{
    game_assets *Assets;

    bool32 IsInitialized;
    memory_arena TransientArena;
    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;
    
    task_with_memory Tasks[4];

    uint32 EnvMapWidth;
    uint32 EnvMapHeight;
    render_environment_map EnvMaps[3];

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;
    uint64_t Padding;

};

inline memory_index
GetMemoryBitAlignmentOffset(memory_arena *arena, memory_index alignment)
{
    memory_index ptr = (memory_index)(arena->Base + arena->Used);
    memory_index alignOffset = 0;
    memory_index alignMask = alignment - 1;
    if (ptr & alignMask)
    {
        alignOffset = alignment - (ptr & alignMask);
    }

    return alignOffset;
}


inline void*
PushSize_(memory_arena *arena, memory_index size, memory_index alignment = 4)
{
    memory_index alignOffset = GetMemoryBitAlignmentOffset(arena, alignment);

    size += alignOffset;
    
    Assert((arena->Used + size) <= arena->Size);
    
    void *result = (void *) (arena->Base + arena->Used + alignOffset);
    arena->Used += size;
    return result;
}

inline char*
PushString(memory_arena *arena, char *source)
{
    u32 size = 1;
    // calc char length
    for (char *c = source; *c; ++c) { size++; }
    
    char *dest = (char *)PushSize_(arena, size);
    for (u32 i = 0; i < size; i++) {
        dest[i] = source[i];
    }

    return dest;
}

#define ZeroStruct(instance) ZeroSize(sizeof(instance), &(instance))
inline void
ZeroSize(memory_index size, void *ptr)
{
    // TODO(casey): performance
    uint8 *byte = (uint8*)ptr;
    while (size--)
    {
        *byte++ = 0;
    }
}


inline void
Copy(memory_index size, void*source, void *dest)
{
    u8 *s = (u8 *)source;
    u8 *d = (u8 *)dest;

    while (size--) { *d++ = *s++; }
}



inline temporary_memory
BeginTemporaryMemory(memory_arena *arena)
{
    temporary_memory result;

    result.Arena = arena;
    result.Used = arena->Used;
    result.Arena->TempCount++;

    return result;
}

inline void
EndTemporaryMemory(temporary_memory tempMem)
{
    memory_arena *arena = tempMem.Arena;
    Assert(arena->Used >= tempMem.Used)
    arena->Used = tempMem.Used;
    Assert(arena->TempCount > 0);
    arena->TempCount--;
}

internal void
InitializeArena(memory_arena *arena, memory_index size, void *base)
{
    arena->Size = size;
    arena->Base = (uint8 *)base;
    arena->Used = 0;
    arena->TempCount = 0;
}

inline memory_index
GetArenaSizeRemaining(memory_arena *arena, memory_index alignment=4)
{
    memory_index result = arena->Size - (arena->Used + GetMemoryBitAlignmentOffset(arena, alignment));
    return result;
}

inline void
CheckArena(memory_arena *arena)
{
    Assert(arena->TempCount == 0);
}

inline void
SubArena(memory_arena *result, memory_arena *arena, memory_index size, memory_index alignment = 4)
{
    result->Size = size;
    result->Base = (uint8 *)PushSize_(arena, size, alignment);
    result->Used = 0;
    result->TempCount = 0;
}

/* void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_input *input); */

/* void GameGetSoundSamples(game_memory *memory, game_sound_output_buffer *soundBuffer); */
internal task_with_memory* BeginTaskWithMemory(transient_state *tranState);
internal void EndTaskWithMemory(task_with_memory *task);
#endif


global_variable platform_api PlatformAPI;
