/*
  HANDMADE_INTERNAL:
  	0 - build for public release
        1 - build for developer ony

  HANDMADE_SLOW:
  	0 - no slow code is allowed
        1 - can be less performant
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

struct high_entity
{
    v2 Position;
    uint32 AbsTileZ;
    v2 dPosition;
    uint32 FacingDirection;

    real32 TBobing;

    uint32 LowEntityIndex;
};

enum entity_type
{
    EntityType_NULL,
    EntityType_Hero,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monster,
    EntityType_Sword
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    // TODO: collapse into one variable
    uint8 Flags;
    uint32 FilledAmount;

};

struct low_entity
{
    entity_type Type;

    world_position Position;
    real32 Width;
    real32 Height;

    bool32 Collides;
    int32 dAbsTileZ;

    uint32 HighEntityIndex;

    // TODO: should hit points be entities
    uint32 HitPointMax;
    hit_point HitPoints[16];

    uint32 SwordLowIndex;
    real32 DistanceRemaining;
};

enum entity_residence
{
    EntityResidence_Nonexistant,
    EntityResidence_Dormant,
    EntityResidence_Low,
    EntityResidence_High,
};

struct entity
{
    uint32 LowIndex;
    low_entity *Low;
    high_entity *High;
};

struct low_entity_chunk_reference
{
    world_chunk *TileChunk;
    uint32 EntityIndexChunk;
};

struct game_state
{
    int XOffset;
    int YOffset;

    memory_arena WorldArena;

    world * World;
    real32 MetersToPixels;

    // TODO: what to do with multiple players?
    uint32 CameraFollowingEntityIndex;
    world_position CameraPosition;

    uint32 PlayerIndexControllerMap[ArrayCount(((game_input *)0)->Controllers)];

    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    uint32 HighEntityCount;
    high_entity HighEntities_[256];
    
    loaded_bitmap LoadedBitmap;
    loaded_bitmap EnemyDemoBitmap;
    loaded_bitmap FamiliarDemoBitmap;
    loaded_bitmap WallDemoBitmap;
    loaded_bitmap SwordDemoBitmap;
    
    hero_bitmaps HeroBitmaps[4];


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
