/*
  HANDMADE_INTERNAL:
  	0 - build for public release
        1 - build for developer ony

  HANDMADE_SLOW:
  	0 - no slow code is allowed
        1 - can be less performant
 */


#include <stdint.h>
// todo: remove
#include <math.h>

#include "handmade_platform.h"
#include "handmade_platform.cpp"


#include "handmade_math.h"





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

#include "handmade_intrinsics.h"
#include "handmade_tile.h"


struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;
};

struct world
{
    tile_map *TileMap;
};


struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32 *Pixels;
};


struct hero_bitmaps
{
    int32 AlignX;
    int32 AlignY;

    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct game_state
{
    int ToneHz;
    int XOffset;
    int YOffset;

    memory_arena WorldArena;

    world * World;

    tile_map_position CameraPosition;
    tile_map_position PlayerPosition;

    loaded_bitmap LoadedBitmap;
    
    uint32 HeroFacingDirection;
    hero_bitmaps HeroBitmaps[4];
};



/* void GameUpdateAndRender(game_memory *memory, game_offscreen_buffer *buffer, game_input *input); */

/* void GameGetSoundSamples(game_memory *memory, game_sound_output_buffer *soundBuffer); */
