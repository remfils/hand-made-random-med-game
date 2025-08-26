#if !defined(HANDMADE_FILE_FORMATS_H)
#define HANDMADE_FILE_FORMATS_H

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    /* NOTE: angle in radians off of due right */
    Tag_FacingDirection,
    Tag_UnicodePoint,

    Tag_Count
};

enum asset_type_id
{

    /****************************************************************************************************
     * BITMAPS
     ****************************************************************************************************/
    
    AssetType_None,
    AssetType_Loaded,
    AssetType_EnemyDemo,
    AssetType_FamiliarDemo,
    AssetType_WallDemo,
    AssetType_SwordDemo,
    AssetType_ParticleStar,

    /* arrays */
    AssetType_Grass,
    AssetType_Ground,

    /* character */

    AssetType_HumanBody,
    AssetType_HumanShadow,
    
    AssetType_Font,

    /****************************************************************************************************
     * SOUNDS
     ****************************************************************************************************/
    AssetType_FootstepGravel,
    AssetType_FootstepMud,
    AssetType_FootstepStone,
    AssetType_FootstepWood,
    AssetType_PianoMusic,

    /****************************************************************************************************
     * Other
     ****************************************************************************************************/
    
    AssetType_Unknown,
    AssetType_Count
};

#pragma pack(push, 1)

struct bitmap_id
{
    u32 Value;
};
struct sound_id
{
    u32 Value;
};

struct hha_header
{
#define HHA_MAGIC_VALUE 0x10F2C
    u32 MagicValue = HHA_MAGIC_VALUE;

#define HHA_VERSION 0
    u32 Version;

    u32 TagCount;
    u32 AssetCount;
    u32 AssetTypeCount;

    u64 TagOffset;
    u64 AssetOffset;
    u64 AssetTypeOffset;
};

struct hha_tag
{
    u32 Id;
    r32 Value;
};

enum hha_sound_chain
{
    HHASoundChain_None,
    HHASoundChain_Loop,
    HHASoundChain_Advance,
};

struct hha_bitmap
{
    u32 Dim[2];
    v2 AlignPercentage;
};

struct hha_sound
{
    u32 SampleCount;
    hha_sound_chain Chain;
    u32 ChannelCount;
};

struct hha_asset
{
    u64 DataOffset;
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
    char DebugName[20];
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
