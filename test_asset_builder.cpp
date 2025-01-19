#include <stdio.h>
#include <stdlib.h>

#include "handmade_platform.h"
#include "handmade_asset_type_id.h"

FILE *out;

struct asset_bitmap_info
{
    char *FileName;
    r32 AlignPercent[2];
};

struct asset_sound_info
{
    char *FileName;
    uint32 FirstSampleIndex;
    uint32 SampleCount;
    u32 NextIdToPlay;
};

struct asset
{
    uint32 FirstTagIndex;
    uint32 OnePastLastTagIndex;
    u64 DataOffsetId;
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

#define VERY_LARGE_NUMBER 4096

uint32 BitmapCount;
uint32 SoundCount;
uint32 AssetCount;
uint32 TagCount;
asset_bitmap_info BitmapInfos[VERY_LARGE_NUMBER];
asset_sound_info SoundInfos[VERY_LARGE_NUMBER];
asset Assets[VERY_LARGE_NUMBER];
asset_tag Tags[VERY_LARGE_NUMBER];
    

asset_type AssetTypes[AssetType_Count];

u32 DEBUGUsedBitmapCount;
u32 DEBUGUsedSoundCount;
u32 DEBUGUsedAssetCount;
u32 DEBUGUsedTagCount;
asset_type *DEBUGCurrentAssetType;
asset *DEBUGCurrentAsset;



struct bitmap_asset
{
    char *filename;
    r32 alignment[2];
};

internal void 
BeginAssetType(asset_type_id id)
{
    DEBUGCurrentAssetType = AssetTypes + id;
    DEBUGCurrentAssetType->FirstAssetIndex = DEBUGUsedAssetCount;
    DEBUGCurrentAssetType->OnePastLastAssetIndex = DEBUGCurrentAssetType->FirstAssetIndex;
}

internal void
AddBitmapAsset(char *fileName, r32 alignPercentX, r32 allignPercentY)
{
    Assert(DEBUGCurrentAssetType);
    Assert(DEBUGCurrentAssetType->OnePastLastAssetIndex < assetCount);
    
    asset *asset = Assets + DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset->FirstTagIndex = DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;

    /*
      bitmap_id id = {DEBUGUsedBitmapCount++};

    asset_bitmap_info *result = BitmapInfos + id.Value;
    result->AlignPercent = alignPercent;
    result->FileName = PushString(&assets->Arena, fileName);

    return id;
     */
    // asset->SlotId = DEBUGAddBitmapInfo(Assets, fileName, ).Value;

    DEBUGCurrentAsset = asset;
}

internal asset*
AddSoundAsset(char *fileName, uint32 firstSampleIndex=0, uint32 sampleCount=0)
{
    Assert(DEBUGCurrentAssetType);
    Assert(DEBUGCurrentAssetType->OnePastLastAssetIndex < AssetCount);
    
    asset *asset = Assets + DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset->FirstTagIndex = DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;

    
    asset->SlotId = DEBUGAddSoundInfo(Assets, fileName, firstSampleIndex, sampleCount).Value;
    /*
      sound_id id = {DEBUGUsedSoundCount++};

      asset_sound_info *result = SoundInfos + id.Value;
      result->FileName = PushString(&assets->Arena, fileName);
      result->NextIdToPlay.Value = 0;
      result->FirstSampleIndex = firstSampleIndex;
      result->SampleCount = sampleCount;

      return id;
     */

    DEBUGCurrentAsset = asset;
    return asset;
}

int
main(int argCount, char **args)
{
    out = fopen("test.hha", "wb");

    if (out)
    {
        
        fclose(out);
    }
    
    #if 0
    
    BeginAssetType(assets, AssetType_Loaded);
    AddBitmapAsset(assets, "../data/new-bg-code.bmp");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_EnemyDemo);
    AddBitmapAsset(assets, "../data/test2/enemy_demo.bmp", ToV2(0.516949177f, 0.0616113730f));
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FamiliarDemo);
    AddBitmapAsset(assets, "../data/test2/familiar_demo.bmp");
    /**
     // TODO: fix this if needed?
      
     work->TopDownAlignX = 58;
     work->TopDownAlignY = 203;
    */
    EndAssetType(assets);
    
    BeginAssetType(assets, AssetType_WallDemo);
    AddBitmapAsset(assets, "../data/test2/wall_demo.bmp", ToV2(0.5f, 0.484375f));
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_SwordDemo);
    AddBitmapAsset(assets, "../data/test2/sword_demo.bmp", ToV2(0.479999989f, 0.0317460336f));
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_Grass);
    AddBitmapAsset(assets, "../data/grass001.bmp");
    AddBitmapAsset(assets, "../data/grass002.bmp");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_Ground);
    AddBitmapAsset(assets, "../data/ground001.bmp");
    AddBitmapAsset(assets, "../data/ground002.bmp");
    EndAssetType(assets);

    TranState = tranState;


    BeginAssetType(assets, AssetType_HumanBody);
    v2 heroAlign = ToV2(0.508474588f, 0.07109005f);
    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_right.bmp", heroAlign);
    AddAssetTag(assets, Tag_FacingDirection, 0.0f);

    heroAlign = ToV2(0.508474588f, 0.118483409f);
    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_up.bmp", heroAlign);
    AddAssetTag(assets, Tag_FacingDirection, 0.5f * Pi32);

    heroAlign = ToV2(0.508474588f, 0.07109005f);
    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_left.bmp", heroAlign);
    AddAssetTag(assets, Tag_FacingDirection, Pi32);

    heroAlign = ToV2(0.508474588f, 0.118483409f);
    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_down.bmp", heroAlign);
    AddAssetTag(assets, Tag_FacingDirection, -0.5f * Pi32);
    EndAssetType(assets);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // sounds
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    BeginAssetType(assets, AssetType_FootstepGravel);
    AddSoundAsset(assets, "../data/sound/footsteps__gravel.wav");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FootstepMud);
    AddSoundAsset(assets, "../data/sound/footsteps__mud02.wav");
    EndAssetType(assets);
    
    BeginAssetType(assets, AssetType_FootstepStone);
    AddSoundAsset(assets, "../data/sound/footsteps__stone01.wav");
    EndAssetType(assets);
    
    BeginAssetType(assets, AssetType_FootstepWood);
    AddSoundAsset(assets, "../data/sound/footsteps__wood01.wav");
    EndAssetType(assets);


    uint32 totalMusicFileSamples = 615916;
    asset* lastAddedSound = 0;
    uint32 musicChunk = 40000;
    BeginAssetType(assets, AssetType_PianoMusic);
    for (uint32 sampleIndex=0;
         sampleIndex < totalMusicFileSamples;
         sampleIndex += musicChunk
         )
    {
        uint32 samplesToLoad = totalMusicFileSamples - sampleIndex;
        if (samplesToLoad > musicChunk) {
            samplesToLoad = musicChunk;
        }
        asset* addedAsset = AddSoundAsset(assets, "../data/sound/energetic-piano-uptempo-loop_156bpm_F#_minor.wav", sampleIndex, samplesToLoad);
        if (lastAddedSound) {
            SoundInfos[lastAddedSound->SlotId].NextIdToPlay.Value = addedAsset->SlotId;
        }
        lastAddedSound = addedAsset;
    }
    EndAssetType(assets);
    #endif

}
