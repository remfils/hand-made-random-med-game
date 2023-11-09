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
    int32 SampleCount;
    void *Memory;
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
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

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    /* NOTE: angle in radians off of due right */
    Tag_FacingDirection,

    Tag_Count
};

struct asset_bitmap_info
{
    char *FileName;
    v2 AlignPercent;
};

struct asset_sound_info
{
    char *FileName;
};

enum asset_type_id
{
    AssetType_None,
    AssetType_Loaded,
    AssetType_EnemyDemo,
    AssetType_FamiliarDemo,
    AssetType_WallDemo,
    AssetType_SwordDemo,

    /* arrays */
    AssetType_Grass,
    AssetType_Ground,

    /* character */

    AssetType_HumanBody,
    AssetType_HumanShadow,

    AssetType_Unknown,
    AssetType_Count
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

struct asset
{
    uint32 FirstTagIndex;
    uint32 OnePastLastTagIndex;
    uint32 SlotId;
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
    
    uint32 BitmapCount;
    asset_bitmap_info *BitmapInfos;
    asset_slot *Bitmaps;
    
    uint32 SoundCount;
    asset_sound_info *SoundInfos;
    asset_slot *Sounds;

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
    uint32 DEBUGUsedAssetCount;
    uint32 DEBUGUsedTagCount;

    asset_type *DEBUGCurrentAssetType;
    asset *DEBUGCurrentAsset;
};

struct bitmap_id
{
    uint32 Value;
};
struct sound_id
{
    uint32 Value;
};

inline loaded_bitmap*
GetBitmap(game_assets *assets, bitmap_id id)
{
    loaded_bitmap *result = assets->Bitmaps[id.Value].Bitmap;
    return result;
}

internal void LoadBitmap(game_assets *assets, bitmap_id id);

internal void LoadSound(game_assets *assets, sound_id id);
