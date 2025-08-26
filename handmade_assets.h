#if !defined(HANDMADE_ASSETS_H)
#define HANDMADE_ASSETS_H


struct loaded_bitmap
{
    void *Memory;
    v2 AlignPercent;
    r32 WidthOverHeight;
    
    s32 Width;
    s32 Height;
    s32 Pitch;
};

struct loaded_sound
{
    // TODO: this could be shrunk to 12 bytes
    // NOTE: loaded sound assets has to be 8 alligned
    s16 *Samples[2];
    u32 SampleCount;
    u32 ChannelCount;
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    
    AssetState_Mask = 0xFF,
    AssetState_TypeMask = 0xF000,

    AssetState_Lock = 0x10000,
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

struct asset_group
{
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
};

struct asset_file
{
    // TODO: remove assetTypeArray to transient (thread stacks...)
    hha_header Header;
    platform_file_handle Handle;
    hha_asset_type *AssetTypes;
    u32 TagBase;
};

struct asset_memory_header
{
    asset_memory_header *Next;
    asset_memory_header *Prev;

    
    u32 AssetIndex;
    u32 TotalSize;
    
    union
    {
        loaded_bitmap Bitmap;
        loaded_sound Sound;
    };
};

struct asset
{
    u32 State;
    u32 FileIndex;
    hha_asset HHA;

    asset_memory_header *Header;
};

enum asset_memory_block_flags
{
    AssetMemory_Used = 0x1,
};

struct asset_memory_block
{
    asset_memory_block *Prev;
    asset_memory_block *Next;
    u64 Flags;
    u64 Size;
};

struct game_assets
{
    struct transient_state *TranState;

    asset_memory_block MemorySentinel;

    u32 FileCount;
    asset_file *Files;
    
    // NOTE: end of the list will be least used
    asset_memory_header LoadedAssetsSentinel;
    
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

inline b32
IsAssetLocked(asset *asset)
{
    b32 result = asset->State && AssetState_Lock;
    return result;
}

inline u32
GetAssetState(asset *asset)
{
    u32 result = asset->State & AssetState_Mask; // take the bottom bits;
    return result;
}

inline u32
GetAssetType(asset *asset)
{
    u32 result = asset->State & AssetState_TypeMask; // take the top bits;
    return result;
}

internal void MoveHeaderToFront(game_assets *assets, asset *asset);

inline loaded_bitmap*
GetBitmap(game_assets *assets, bitmap_id id, b32 mustBeLocked)
{
    asset *asset = assets->Assets + id.Value;

    loaded_bitmap *result = 0;

    if (GetAssetState(asset) >= AssetState_Loaded)
    {
        Assert(!mustBeLocked || IsAssetLocked(asset));
        CompletePreviousReadsBeforeFutureReads;
        result = &asset->Header->Bitmap;

        MoveHeaderToFront(assets, asset);
    }
    return result;
}

inline loaded_sound*
GetSound(game_assets *assets, sound_id id)
{
    asset *asset = assets->Assets + id.Value;
    loaded_sound *result = 0;

    if (GetAssetState(asset) >= AssetState_Loaded)
    {
        CompletePreviousReadsBeforeFutureReads;
        result = &asset->Header->Sound;

        MoveHeaderToFront(assets, asset);
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

internal void LoadBitmap(game_assets *assets, bitmap_id id, b32 locked);
inline void PrefetchBitmap(game_assets *assets, bitmap_id id, b32 locked) { LoadBitmap(assets, id, locked); };

internal void LoadSound(game_assets *assets, sound_id id);
inline void PrefetchSound(game_assets *assets, sound_id id) { LoadSound(assets, id); };

inline sound_id
GetNextSoundInChain(game_assets *assets, sound_id id)
{
    sound_id result = {};
    hha_sound *info = GetSoundInfo(assets, id);

    switch (info->Chain) {
    case HHASoundChain_None: {
        // NOTE: nothing
    } break;
    case HHASoundChain_Loop: {
        result = id;
    } break;
    case HHASoundChain_Advance: {
        result.Value = id.Value + 1;
    } break;
    }
    
    return result;
}

#endif

