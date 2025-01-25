#if !defined(HANDMADE_ASSETS_H)
#define HANDMADE_ASSETS_H

#include "handmade_asset_type_id.h"

struct bitmap_id
{
    u32 Value;
};
struct sound_id
{
    u32 Value;
};


struct loaded_bitmap
{
    v2 AlignPercent;
    r32 WidthOverHeight;
    
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};

struct loaded_sound
{
    // NOTE: loaded sound assets has to be 8 alligned
    u32 SampleCount;
    u32 ChannelCount;
    int16 *Samples[2];
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
};

struct asset_tag
{
    u32 Id;
    r32 Value;
};

struct asset_vector
{
    r32 E[Tag_Count];
};

struct asset_type
{
    u32 FirstAssetIndex;
    u32 OnePastLastAssetIndex;
};

struct asset_bitmap_info
{
    char *FileName;
    v2 AlignPercent;
};

struct asset_sound_info
{
    char *FileName;
    u32 FirstSampleIndex;
    u32 SampleCount;
    sound_id NextIdToPlay;
};

struct asset_slot
{
    asset_state State;
    union
    {
        loaded_bitmap *Bitmap;
        loaded_sound *Sound;
    };
};

struct asset
{
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;

    union
    {
        asset_bitmap_info Bitmap;
        asset_sound_info Sound;
    };
};

struct asset_group
{
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
};

struct game_assets
{
    struct transient_state *TranState;
    
    memory_arena Arena;
    
    asset_slot *Slots;

    u32 TagCount;
    r32 TagPeriodRange[Tag_Count];
    asset_tag *Tags;
    
    u32 AssetCount;
    asset *Assets;

    asset_type AssetTypes[AssetType_Count];

    #if 0
    /*
    hero_bitmaps Hero[4];
    */
    
    // TODO: remove after asset pack file is done
    u32 DEBUGUsedAssetCount;
    u32 DEBUGUsedTagCount;

    asset_type *DEBUGCurrentAssetType;
    asset *DEBUGCurrentAsset;
    #endif
};

inline loaded_bitmap*
GetBitmap(game_assets *assets, bitmap_id id)
{
    loaded_bitmap *result = assets->Slots[id.Value].Bitmap;
    return result;
}

inline loaded_sound*
GetSound(game_assets *assets, sound_id id)
{
    Assert(id.Value <= assets->AssetCount);
    loaded_sound *result = assets->Slots[id.Value].Sound;
    return result;
}

inline asset_sound_info*
GetSoundInfo(game_assets *assets, sound_id id)
{
    Assert(id.Value <= assets->AssetCount);
    asset_sound_info *result = &assets->Assets[id.Value].Sound;
    return result;
}

inline bool32
IsValid(bitmap_id id)
{
    bool32 result = id.Value != 0;
    return result;
}

inline bool32
IsValid(sound_id id)
{
    bool32 result = id.Value != 0;
    return result;
}

internal void LoadBitmap(game_assets *assets, bitmap_id id);
inline void PrefetchBitmap(game_assets *assets, bitmap_id id) { LoadBitmap(assets, id); };

internal void LoadSound(game_assets *assets, sound_id id);
inline void PrefetchSound(game_assets *assets, sound_id id) { LoadSound(assets, id); };

#endif
