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


#include <stdint.h>
// TODO: remove
#include <math.h>

#include "handmade_platform.h"
#include "handmade_platform.cpp"

#include "handmade_intrinsics.h"
#include "handmade_math.h"




#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))


struct color
{
    real32 Red;
    real32 Green;
    real32 Blue;
};


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
};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32 *Pixels;
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
    PairCollisionFlag_ShouldCollide = 1 << 1,
    PairCollisionFlag_Temporary = 1 << 2,
};

struct pairwise_collision_rule
{
    bool32 ShouldCollide;
    uint32 StorageIndexA;
    uint32 StorageIndexB;

    pairwise_collision_rule * NextInHash;
};

struct game_state
{
    int XOffset;
    int YOffset;

    memory_arena WorldArena;

    real32 tSine;

    world * World;
    real32 MetersToPixels;

    // TODO: what to do with multiple players?
    uint32 CameraFollowingEntityIndex;
    world_position CameraPosition;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    
    loaded_bitmap LoadedBitmap;
    loaded_bitmap EnemyDemoBitmap;
    loaded_bitmap FamiliarDemoBitmap;
    loaded_bitmap WallDemoBitmap;
    loaded_bitmap SwordDemoBitmap;
    loaded_bitmap StairwayBitmap;
    
    hero_bitmaps HeroBitmaps[4];

    // must be a power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;
};


struct entity_visible_piece
{
    loaded_bitmap *Bitmap;
    v2 Offset;
    real32 Z;
    real32 Alpha;

    v2 Dim;
    real32 R, G, B;
};

struct entity_visible_piece_group
{
    game_state *GameState;
    uint32 PieceCount;
    entity_visible_piece Pieces[32];
};

/* void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_input *input); */

/* void GameGetSoundSamples(game_memory *memory, game_sound_output_buffer *soundBuffer); */
