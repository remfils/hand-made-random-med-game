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
   - Z!
     - how to go up and down and how to render it
     - solve puzler from worl_position
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

   - Audio
     - sound effect triggers
     - ambient sound
     - music
   - Asset streaming

   PRODUCTION
   - Rendering
   - Entity system
   - World generation
     
 */

// TODO: remove
#include <math.h>

#include "handmade_platform.h"
#include "handmade_platform.cpp"

#include "handmade_intrinsics.h"
#include "handmade_math.h"




#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#define PushStruct(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, count,  type) (type *)PushSize_(arena, count * sizeof(type))
#define PushSize(arena, size) PushSize_(arena, size)

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

struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;

    int32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

#define BITMAP_BYTES_PER_PIXEL 4
struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};


struct hero_bitmaps
{
    v2 Align;
    loaded_bitmap Character;
};

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

struct ground_buffer
{
    world_position P; // NOTE: this is center of bitmap
    loaded_bitmap Bitmap;
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
    real32 MetersToPixels;
    real32 PixelsToMeters;

    // TODO: what to do with multiple players?
    uint32 CameraFollowingEntityIndex;
    world_position CameraPosition;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    loaded_bitmap GrassBitmaps[2];
    loaded_bitmap GroundBitmaps[2];
    
    loaded_bitmap LoadedBitmap;
    loaded_bitmap EnemyDemoBitmap;
    loaded_bitmap FamiliarDemoBitmap;
    loaded_bitmap WallDemoBitmap;
    loaded_bitmap SwordDemoBitmap;
    loaded_bitmap StairwayBitmap;

    loaded_bitmap HeroNormal;
    
    world_position GroundP;    
    
    hero_bitmaps HeroBitmaps[4];

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
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TransientArena;
    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;
};


inline void*
PushSize_(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size);

    void *result = arena->Base + arena->Used;
    arena->Used += size;
    return result;
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

inline void
CheckArena(memory_arena *arena)
{
    Assert(arena->TempCount == 0);
}

/* void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_input *input); */

/* void GameGetSoundSamples(game_memory *memory, game_sound_output_buffer *soundBuffer); */
