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
    r32 AlignPercent[2];
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

struct asset_vector
{
    r32 E[Tag_Count];
};

struct asset_bitmap_info
{
    char *FileName;
    r32 AlignPercent[2];
};

struct asset_sound_info
{
    char *FileName;
    u32 FirstSampleIndex;
    u32 SampleCount;
    sound_id NextIdToPlay;
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

#define VERY_LARGE_NUMBER 4000
struct game_assets
{
    u32 TagCount;
    hha_tag Tags[VERY_LARGE_NUMBER];
    
    u32 AssetTypeCount;
    hha_asset_type AssetTypes[AssetType_Count];

    u32 AssetCount;
    asset Assets[VERY_LARGE_NUMBER];

    // TODO: remove after asset pack file is done
    u32 DEBUGUsedAssetCount;
    u32 DEBUGUsedTagCount;

    hha_asset_type *DEBUGCurrentAssetType;
    asset *DEBUGCurrentAsset;
};
