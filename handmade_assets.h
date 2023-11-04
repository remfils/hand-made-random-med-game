struct loaded_bitmap
{
    v2 AlignPercent;
    real32 WidthOverHeight;
    
    int32 Width;
    int32 Height;
    int32 Pitch;
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
    struct loaded_bitmap *Bitmap;
};

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,

    Tag_Count
};

struct asset_bitmap_info
{
    v2 AlignPercent;
    real32 WidthOverHeight;
    int32 Width;
    int32 Height;

};

enum asset_type_id
{
    AssetType_None,
    AssetType_Backdrop,
    AssetType_Grass,
    AssetType_Ground,
    AssetType_Loaded,
    AssetType_EnemyDemo,
    AssetType_FamiliarDemo,
    AssetType_WallDemo,
    AssetType_SwordDemo,
    AssetType_Stairway,


    AssetType_Unknown,
    AssetType_Count
};

struct asset_tag
{
    uint32 Id;
    real32 Value;
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

struct hero_bitmaps
{
    loaded_bitmap Character;
};

struct game_assets
{
    struct transient_state *TranState;
    
    memory_arena Arena;

    uint32 BitmapCount;
    asset_slot *Bitmaps;
    
    uint32 SoundCount;
    asset_slot *Sounds;

    uint32 TagCount;
    asset_tag *Tags;
    
    uint32 AssetCount;
    asset *Assets;

    asset_type AssetTypes[AssetType_Count];
    
    hero_bitmaps Hero[4];
    
    loaded_bitmap Grass[2];
    loaded_bitmap Ground[2];
};

struct bitmap_id
{
    uint32 Value;
};
struct audio_id
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

internal void LoadSound(game_assets *assets, audio_id id);
