#if !defined(HANDMADE_FILE_FORMATS_H)
#define HANDMADE_FILE_FORMATS_H

#pragma pack(push, 1)
struct hha_header
{
#define HHA_MAGIC_VALUE 0x10F2C
    u32 MagicValue = HHA_MAGIC_VALUE;

#define HHA_VERSION 0
    u32 Version;

    u32 TagCount;
    u32 AssetCount;
    u32 AssetTypeEntryCount;

    u64 TagOffset;
    u64 AssetOffset;
    u64 AssetTypeEntryOffset;
};

struct hha_tag
{
    u32 Id;
    r32 Value;
};

struct hha_bitmap
{
    u32 Dim[2];
    r32 AlignPercentage[2];
};

struct hha_sound
{
    u32 FirstSampleIndex;
    u32 SampleCount;
    u32 NextIdToPlay;
};

struct hha_asset
{
    u64 DataOffset;
    u32 FirstTagIndex;
    u32 OnePastLastAssetIndex;
    union
    {
        hha_bitmap Bitmap;
        hha_sound Sound;
    };
};

struct hha_asset_type
{
    u32 TypeId;
    u32 FirstAssetIndex;
    u32 OnePastLastAssetIndex;
};
#pragma pack(pop)

#endif
