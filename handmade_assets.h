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
    AssetState_Operating,
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
    u32 GenerationId;
    
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
};

internal void MoveHeaderToFront(game_assets *assets, asset *asset );

inline asset_memory_header*
GetAsset(game_assets *assets, u32 id)
{
    asset *asset = assets->Assets + id;

    asset_memory_header *result = 0;

    for (;;)
    {
        if (asset->State == AssetState_Loaded)
        {
            if (AtomicCompareExchange(&asset->State, AssetState_Operating, AssetState_Loaded) == AssetState_Loaded)
            {
                result = asset->Header;
                MoveHeaderToFront(assets, asset);

                #if 0
                if (asset->Header->GenerationId < generationId) {
                    asset->Header->GenerationId = generationId;
                }
                #endif

                CompletePreviousReadsBeforeFutureReads;

                asset->State = AssetState_Loaded;

                break;
            }
            
        }
        else if (asset->State != AssetState_Operating)
        {
            break;
        }
    }

    
    return result;
}

inline loaded_bitmap*
GetBitmap(game_assets *assets, bitmap_id id)
{
    asset_memory_header *header = GetAsset(assets, id.Value);
    loaded_bitmap *result = header? &header->Bitmap : 0;
    return result;
}

inline loaded_sound*
GetSound(game_assets *assets, sound_id id)
{
    asset_memory_header *header = GetAsset(assets, id.Value);
    loaded_sound *result = header? &header->Sound : 0;
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

