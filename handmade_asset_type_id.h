#if !defined(HANDMADE_ASSET_TYPE_ID_H)
#define HANDMADE_ASSET_TYPE_ID_H

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    /* NOTE: angle in radians off of due right */
    Tag_FacingDirection,

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

#endif
