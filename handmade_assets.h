#if !defined(HANDMADE_ASSETS_H)
#define HANDMADE_ASSETS_H

#include "handmade_asset_type_id.h"

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
    void *Memory;
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

/*
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
*/

struct asset_slot
{
    asset_state State;
    union
    {
        loaded_bitmap *Bitmap;
        loaded_sound *Sound;
    };
};

struct asset_group
{
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
};

struct asset_file
{
    // TODO: remove assetTypeArray to transient (thread stacks...)
    hha_header Header;
    platform_file_handle *Handle;
    hha_asset_type *AssetTypes;
    u32 TagBase;
};

struct asset
{
    u32 FileIndex;
    hha_asset HHA;
};

struct game_assets
{
    struct transient_state *TranState;

    u32 FileCount;
    asset_file *Files;
    
    memory_arena Arena;
    
    asset_slot *Slots;

    u32 TagCount;
    r32 TagPeriodRange[Tag_Count];
    hha_tag *Tags;
    
    u32 AssetCount;
    asset *Assets;


    u8 *HHAContent;

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
    asset_slot *slot = assets->Slots + id.Value;

    loaded_bitmap *result = 0;

    if (slot->State >= AssetState_Loaded)
    {
        CompletePreviousReadsBeforeFutureReads;
        result = slot->Bitmap;
    }
    return result;
}

inline loaded_sound*
GetSound(game_assets *assets, sound_id id)
{
    asset_slot *slot = assets->Slots + id.Value;
    loaded_sound *result = 0;

    if (slot->State >= AssetState_Loaded)
    {
        CompletePreviousReadsBeforeFutureReads;
        result = slot->Sound;
    }
    return result;
}

inline hha_sound*
GetSoundInfo(game_assets *assets, sound_id id)
{
    Assert(id.Value <= assets->AssetCount);
    hha_sound *result = &assets->Assets[id.Value].HHA.Sound;
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
