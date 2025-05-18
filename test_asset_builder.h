#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression; // wtf
    uint32 ImageSize; // wtf
    uint32 XPixelsPerMeter; // wtf
    uint32 YPixelsPerMeter; // wtf
    uint32 ColorsInColorTable; // wtf
    uint32 ImportantColorCount; // wtf
    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
    uint32 AlphaMask;
};
#pragma pack(pop)

struct loaded_bitmap
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    void *Memory;
    void *Free;
};

struct loaded_sound
{
    // NOTE: loaded sound assets has to be 8 alligned
    s16 *Samples[2];
    u32 SampleCount;
    u32 ChannelCount;
    void *Free;
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

struct asset_sound_info
{
    
};

enum asset_file_type
{
    AssetFileType_None,
    AssetFileType_Sound,
    AssetFileType_Bitmap
};

struct source_asset
{
    asset_file_type AssetFileType;
    char *FileName;
    u32 FirstSampleIndex;
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
    source_asset AssetSource[VERY_LARGE_NUMBER];

    hha_asset AssetData[VERY_LARGE_NUMBER];

    // TODO: remove after asset pack file is done
    u32 DEBUGUsedAssetCount;
    u32 DEBUGUsedTagCount;

    hha_asset_type *DEBUGCurrentAssetType;
    u32 AssetIndex;
};
