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
    uint32 FirstSampleIndex;
    uint32 SampleCount;
    sound_id NextIdToPlay;
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

    /* arrays */
    AssetType_Grass,
    AssetType_Ground,

    /* character */

    AssetType_HumanBody,
    AssetType_HumanShadow,

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
    uint32 DEBUGUsedSoundCount;
    uint32 DEBUGUsedAssetCount;
    uint32 DEBUGUsedTagCount;

    asset_type *DEBUGCurrentAssetType;
    asset *DEBUGCurrentAsset;
};

inline loaded_bitmap*
GetBitmap(game_assets *assets, bitmap_id id)
{
    loaded_bitmap *result = assets->Bitmaps[id.Value].Bitmap;
    return result;
}

inline loaded_sound*
GetSound(game_assets *assets, sound_id id)
{
    Assert(id.Value <= assets->SoundCount);
    loaded_sound *result = assets->Sounds[id.Value].Sound;
    return result;
}

inline asset_sound_info*
GetSoundInfo(game_assets *assets, sound_id id)
{
    Assert(id.Value <= assets->SoundCount);
    asset_sound_info *result = assets->SoundInfos + id.Value;
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
