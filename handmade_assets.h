#if !defined(HANDMADE_ASSETS_H)
#define HANDMADE_ASSETS_H

#include "handmade_asset_type_id.h"

struct bitmap_id
{
    uint32 Value;
};
struct sound_id
{
    uint32 Value;
};


struct loaded_bitmap
{
    v2 AlignPercent;
    real32 WidthOverHeight;
    
    int32 Width;
    int32 Height;
    int32 Pitch;
    void *Memory;
};

struct loaded_sound
{
    // NOTE: loaded sound assets has to be 8 alligned
    uint32 SampleCount;
    uint32 ChannelCount;
    int16 *Samples[2];
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
};

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    /* NOTE: angle in radians off of due right */
    Tag_FacingDirection,

    Tag_Count
};


struct asset_tag
{
    uint32 Id;
    real32 Value;
};

struct asset_vector
{
    real32 E[Tag_Count];
};

struct asset_type
{
    uint32 FirstAssetIndex;
    uint32 OnePastLastAssetIndex;
};

struct asset_bitmap_info
{
    char *FileName;
    v2 AlignPercent;
};

struct asset_sound_info
{
    char *FileName;
    uint32 FirstSampleIndex;
    uint32 SampleCount;
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
    uint32 FirstTagIndex;
    uint32 OnePastLastTagIndex;

    union
    {
        asset_bitmap_info Bitmap;
        asset_sound_info Sound;
    };
};

struct asset_group
{
    uint32 FirstTagIndex;
    uint32 OnePastLastTagIndex;
};

struct game_assets
{
    struct transient_state *TranState;
    
    memory_arena Arena;
    
    asset_slot *Slots;

    uint32 TagCount;
    real32 TagPeriodRange[Tag_Count];
    asset_tag *Tags;
    
    uint32 AssetCount;
    asset *Assets;

    asset_type AssetTypes[AssetType_Count];

    /*
    hero_bitmaps Hero[4];
    */
    
    // TODO: remove after asset pack file is done
    uint32 DEBUGUsedBitmapCount;
    uint32 DEBUGUsedSoundCount;
    uint32 DEBUGUsedAssetCount;
    uint32 DEBUGUsedTagCount;

    asset_type *DEBUGCurrentAssetType;
    asset *DEBUGCurrentAsset;
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
