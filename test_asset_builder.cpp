#include <stdio.h>
#include <stdlib.h>

#include "handmade_platform.h"
#include "handmade_asset_type_id.h"
#include "handmade_file_formats.h"
#include "test_asset_builder.h"


FILE *out;

internal void 
BeginAssetType(game_assets *assets, asset_type_id id)
{
    assets->DEBUGCurrentAssetType = assets->AssetTypes + id;
    assets->DEBUGCurrentAssetType->TypeId = id;
    assets->DEBUGCurrentAssetType->FirstAssetIndex = assets->DEBUGUsedAssetCount;
    assets->DEBUGCurrentAssetType->OnePastLastAssetIndex = assets->DEBUGCurrentAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(game_assets *assets, char *fileName, r32 alignPercentX=0.5f, r32 alignPercentY=0.5f)
{
    Assert(assets->DEBUGCurrentAssetType);

    u32 assetIndex = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset *asset = assets->Assets + assetIndex;
    asset->FirstTagIndex = assets->DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;
    asset->Bitmap.AlignPercent[0] = alignPercentX;
    asset->Bitmap.AlignPercent[1] = alignPercentY;
    asset->Bitmap.FileName = fileName;

    assets->DEBUGCurrentAsset = asset;
    return {assetIndex};
}

internal sound_id
AddSoundAsset(game_assets *assets, char *fileName, uint32 firstSampleIndex=0, uint32 sampleCount=0)
{
    Assert(assets->DEBUGCurrentAssetType);

    u32 assetIndex = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset *asset = assets->Assets + assetIndex;
    asset->FirstTagIndex = assets->DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;

    asset->Sound.FileName = fileName;
    asset->Sound.NextIdToPlay.Value = 0;
    asset->Sound.FirstSampleIndex = firstSampleIndex;
    asset->Sound.SampleCount = sampleCount;

    assets->DEBUGCurrentAsset = asset;
    return {assetIndex};
}


internal void
AddAssetTag(game_assets *assets, asset_tag_id tagId, real32 tagValue)
{
    Assert(assets->DEBUGCurrentAsset);
    ++assets->DEBUGCurrentAsset->OnePastLastTagIndex;
    hha_tag *tag = assets->Tags + assets->TagCount++;
    tag->Id = tagId;
    tag->Value = tagValue;
}

internal void
EndAssetType(game_assets *assets)
{
    assets->DEBUGUsedAssetCount = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex;
    assets->DEBUGCurrentAssetType = 0;
    assets->DEBUGCurrentAsset = 0;
}

int
main(int argCount, char **args)
{
    
    #if 1

    game_assets assetsV = {};
    assetsV.TagCount = 0;
    assetsV.AssetCount = 1;
    assetsV.DEBUGUsedAssetCount = 0;
    assetsV.DEBUGUsedTagCount = 0;
    assetsV.DEBUGCurrentAssetType = 0;
    assetsV.DEBUGCurrentAsset = 0;

    /*
    assetsV.Tags = 0;
    assetsV.AssetTypes = 0;
    assetsV.Assets = 0;
    */
    
    game_assets *assets = &assetsV;
    
    BeginAssetType(assets, AssetType_Loaded);
    AddBitmapAsset(assets, "../data/new-bg-code.bmp");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_EnemyDemo);
    AddBitmapAsset(assets, "../data/test2/enemy_demo.bmp", 0.516949177f, 0.0616113730f);
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
    AddBitmapAsset(assets, "../data/test2/wall_demo.bmp", 0.5f, 0.484375f);
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_SwordDemo);
    AddBitmapAsset(assets, "../data/test2/sword_demo.bmp", 0.479999989f, 0.0317460336f);
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_Grass);
    AddBitmapAsset(assets, "../data/grass001.bmp");
    AddBitmapAsset(assets, "../data/grass002.bmp");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_Ground);
    AddBitmapAsset(assets, "../data/ground001.bmp");
    AddBitmapAsset(assets, "../data/ground002.bmp");
    EndAssetType(assets);


    BeginAssetType(assets, AssetType_HumanBody);
    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_right.bmp", 0.508474588f, 0.07109005f);
    AddAssetTag(assets, Tag_FacingDirection, 0.0f);

    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_up.bmp", 0.508474588f, 0.118483409f);
    AddAssetTag(assets, Tag_FacingDirection, 0.5f * Pi32);

    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_left.bmp", 0.508474588f, 0.07109005f);
    AddAssetTag(assets, Tag_FacingDirection, Pi32);

    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_down.bmp", 0.508474588f, 0.118483409f);
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
        sound_id addedAssetId = AddSoundAsset(assets, "../data/sound/energetic-piano-uptempo-loop_156bpm_F#_minor.wav", sampleIndex, samplesToLoad);
        if (lastAddedSound && addedAssetId.Value) {
            lastAddedSound->Sound.NextIdToPlay = addedAssetId;
        }
        lastAddedSound = &assets->Assets[addedAssetId.Value];
    }
    EndAssetType(assets);
    #endif


    out = fopen("test.hha", "wb");

    if (out)
    {
        hha_header header = {};
        header.MagicValue = HHA_MAGIC_VALUE;
        header.Version = HHA_VERSION;
        header.TagCount = assets->TagCount;
        header.AssetCount = assets->AssetCount;
        header.AssetTypeEntryCount = assets->AssetCount;

        header.TagOffset = sizeof(header);
        header.AssetTypeEntryOffset = header.AssetOffset + header.AssetTypeEntryCount * sizeof(hha_tag);

        header.AssetOffset = header.AssetTypeEntryOffset + header.AssetTypeEntryCount * sizeof(hha_asset_type);

        fwrite(&header, sizeof(hha_header), 1, out);


        u32 tagArraySize = header.TagCount * sizeof(hha_tag);
        u32 assetTypeArraySize = header.TagCount * sizeof(hha_asset_type);
        u32 assetArraySize = header.TagCount * sizeof(hha_asset);

        fwrite(&assets->Tags, tagArraySize, 1, out);
        fwrite(&assets->AssetTypes, assetTypeArraySize, 1, out);
        //fwrite(assets, assetArraySize, 1, out);
        
        fclose(out);
    }
    else
    {
        printf("ERROR: can't open file");
    }
    
}
