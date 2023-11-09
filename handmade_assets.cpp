#pragma pack(push, 1)
struct bitmap_header {
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

internal loaded_bitmap
DEBUGLoadBMP(char *filename, v2 alignPercent=ToV2(0.5f, 0.5f))
{
    // NOTE: bitmap order is AABBGGRR, bottom row is first
    
    debug_read_file_result readResult = DEBUG_ReadEntireFile(filename);
    
    loaded_bitmap result = {};

    if (readResult.Content)
    {
        void *content = readResult.Content;
        
        bitmap_header *header = (bitmap_header *) content;
        
        result.Width = header->Width;
        result.Height = header->Height;
        result.WidthOverHeight = SafeRatio_1((real32)result.Width, (real32)result.Height);

        result.AlignPercent = alignPercent;
        uint8 *pixels = (uint8 *)((uint8 *)content + header->BitmapOffset);
        result.Memory = pixels;

        uint32 redMask = header->RedMask;
        uint32 greenMask = header->GreenMask;
        uint32 blueMask = header->BlueMask;
        uint32 alphaMask = ~(redMask | greenMask | blueMask);

        bit_scan_result redScan = FindLeastSignificantSetBit(redMask);
        bit_scan_result greenScan = FindLeastSignificantSetBit(greenMask);
        bit_scan_result blueScan = FindLeastSignificantSetBit(blueMask);
        bit_scan_result alphaScan = FindLeastSignificantSetBit(alphaMask);

        uint32 alphaShiftUp = 24;
        int32 alphaShiftDown = (int32)alphaScan.Index;
        
        uint32 redShiftUp = 16;
        int32 redShiftDown = (int32)redScan.Index;

        uint32 greenShiftUp = 8;
        int32 greenShiftDown = (int32)greenScan.Index;

        uint32 blueShiftUp = 0;
        int32 blueShiftDown = (int32)blueScan.Index;


        // NOTE: why this is not needed?
        uint32 *srcDest = (uint32 *)pixels;
        for (int32 y =0;
             y < header->Height;
             ++y)
        {
            for (int32 x =0;
                 x < header->Width;
                 ++x)
            {
                uint32 color = *srcDest;

                v4 texel = {
                    (real32)((color & redMask) >> redShiftDown),
                    (real32)((color & greenMask) >> greenShiftDown),
                    (real32)((color & blueMask) >> blueShiftDown),
                    (real32)((color & alphaMask) >> alphaShiftDown)
                };

                texel = SRGB255_ToLinear1(texel);
                texel.rgb *= texel.a;
                texel = Linear_1_ToSRGB255(texel);

                *srcDest++ = ((uint32)(texel.a + 0.5f) << alphaShiftUp)
                    | ((uint32)(texel.r + 0.5f) << redShiftUp)
                    | ((uint32)(texel.g + 0.5f) << greenShiftUp)
                    | ((uint32)(texel.b + 0.5f) << blueShiftUp);
            }
        }
    }
    else
    {
        // TODO: log
    }

    result.Pitch = result.Width * BITMAP_BYTES_PER_PIXEL;

    return result;
}

struct load_bitmap_work
{
    game_assets *Assets;
    bitmap_id Id;
    task_with_memory *Task;
    loaded_bitmap *Bitmap;

    asset_state FinalState;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork)
{
    load_bitmap_work *work = (load_bitmap_work *)data;

    // TODO: remove this

    asset_bitmap_info *info = work->Assets->BitmapInfos + work->Id.Value;
    *work->Bitmap = DEBUGLoadBMP(info->FileName, info->AlignPercent);

    CompletePreviousWritesBeforeFutureWrites;
    
    work->Assets->Bitmaps[work->Id.Value].Bitmap = work->Bitmap;
    work->Assets->Bitmaps[work->Id.Value].State = work->FinalState;

    EndTaskWithMemory(work->Task);
}

internal void
LoadBitmap(game_assets *assets, bitmap_id id)
{
    if (id.Value)
    {
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&assets->Bitmaps[id.Value].State, AssetState_Unloaded, AssetState_Queued);
        if (currentState == AssetState_Unloaded)
        {
            task_with_memory *task = BeginTaskWithMemory(assets->TranState);
    
            if (task)
            {
                load_bitmap_work *work = PushStruct(&task->Arena, load_bitmap_work);

                work->Assets = assets;
                work->Id = id;
                work->Bitmap = PushStruct(&assets->Arena, loaded_bitmap);
                work->Task = task;
                work->FinalState = AssetState_Loaded;
    
                PlatformAddEntry(assets->TranState->LowPriorityQueue, DoLoadBitmapWork, work);
            }
        }
    }
}

inline bitmap_id
RandomAssetFrom(game_assets *assets, asset_type_id assetType, random_series *series)
{
    bitmap_id result = {};
    asset_type *type = assets->AssetTypes + assetType;

    if (type->FirstAssetIndex && type->OnePastLastAssetIndex)
    {
        uint32 count = type->OnePastLastAssetIndex - type->FirstAssetIndex;
        uint32 choice = RandomChoice(series, count);

        asset *asset = assets->Assets + type->FirstAssetIndex + choice;
        result.Value = asset->SlotId;
    }

    return result;
}

internal bitmap_id
BestMatchAsset(game_assets *assets,
               asset_type_id assetType,
               asset_vector *matchVector,
               asset_vector *weightVector
               )
{
    bitmap_id result = {};
    
    int32 bestIndex = 0;
    real32 bestDiff = Real32Maximum;

    asset_type *type = assets->AssetTypes + assetType;
    for (uint32 assetIndex=type->FirstAssetIndex;
         assetIndex < type->OnePastLastAssetIndex;
         ++assetIndex)
    {
        asset *asset = assets->Assets + assetIndex;
        real32 totalWeightedDiff = 0.0f;

        for (uint32 tagIndex = asset->FirstTagIndex;
             tagIndex < asset->OnePastLastTagIndex;
             ++tagIndex)
        {
            asset_tag *tag = assets->Tags + tagIndex;
            real32 diff = matchVector->E[tag->Id] - tag->Value;
            real32 weightedDifference = weightVector->E[tag->Id] * AbsoluteValue(diff);

            totalWeightedDiff += weightedDifference;
        }

        if (bestDiff > totalWeightedDiff)
        {
            bestDiff = totalWeightedDiff;
            result.Value = asset->SlotId;
        }
    }
    
    return result;
}

inline bitmap_id
GetFirstBitmap(game_assets* assets, asset_type_id assetType)
{
    // TODO: FIX THIS
    bitmap_id result = {};

    asset_type *type = assets->AssetTypes + assetType;

    if (type->FirstAssetIndex != type->OnePastLastAssetIndex)
    {
        asset *asset = assets->Assets + type->FirstAssetIndex;
        result.Value = asset->SlotId;
    }

    return result;
}

internal void
LoadSound(game_assets *assets, uint32 id)
{
    // TODO:
}

internal bitmap_id
DEBUGAddBitmapInfo(game_assets *assets, char *fileName, v2 alignPercent)
{
    bitmap_id id = {assets->DEBUGUsedBitmapCount++};

    asset_bitmap_info *result = assets->BitmapInfos + id.Value;
    result->AlignPercent = alignPercent;
    result->FileName = fileName;

    return id;
}

internal void 
BeginAssetType(game_assets *assets, asset_type_id id)
{
    assets->DEBUGCurrentAssetType = assets->AssetTypes + id;
    assets->DEBUGCurrentAssetType->FirstAssetIndex = assets->DEBUGUsedAssetCount;
    assets->DEBUGCurrentAssetType->OnePastLastAssetIndex = assets->DEBUGCurrentAssetType->FirstAssetIndex;
}

internal void
AddBitmapAsset(game_assets *assets, char *fileName, v2 alignPercent={0.5f, 0.5f})
{
    Assert(assets->DEBUGCurrentAssetType);
    Assert(assets->DEBUGCurrentAssetType->OnePastLastAssetIndex < assets->AssetCount);
    
    asset *asset = assets->Assets + assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset->FirstTagIndex = assets->DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;
    asset->SlotId = DEBUGAddBitmapInfo(assets, fileName, alignPercent).Value;

    assets->DEBUGCurrentAsset = asset;
}

internal void
AddAssetTag(game_assets *assets, asset_tag_id tagId, real32 tagValue)
{
    Assert(assets->DEBUGCurrentAsset);
    ++assets->DEBUGCurrentAsset->OnePastLastTagIndex;
    asset_tag *tag = assets->Tags + assets->DEBUGUsedTagCount++;
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


internal game_assets*
AllocateGameAssets(memory_arena *arena, memory_index assetSize, transient_state *tranState)
{
    game_assets *assets = PushStruct(arena, game_assets);

    SubArena(&assets->Arena, arena, assetSize);
    
    assets->DEBUGUsedBitmapCount = 1;
    assets->DEBUGUsedAssetCount = 0;
    
    assets->BitmapCount = 256 * AssetType_Count;
    assets->BitmapInfos = PushArray(arena, assets->BitmapCount, asset_bitmap_info);
    assets->Bitmaps = PushArray(arena, assets->BitmapCount, asset_slot);

    assets->SoundCount = 1;
    assets->Sounds = PushArray(arena, assets->SoundCount, asset_slot);;
    
    assets->AssetCount = assets->BitmapCount + assets->SoundCount;
    assets->Assets = PushArray(arena, assets->AssetCount, asset);

    assets->TagCount = 1024 * AssetType_Count;
    assets->Tags = PushArray(arena, assets->TagCount, asset_tag);
    
    BeginAssetType(assets, AssetType_Loaded);
    AddBitmapAsset(assets, "../data/new-bg-code.bmp");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_EnemyDemo);
    AddBitmapAsset(assets, "../data/test2/enemy_demo.bmp", ToV2(0.516949177f, 0.0616113730f));
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FamiliarDemo);
    AddBitmapAsset(assets, "../data/test2/familiar_demo.bmp");
                    /* TODO: fix this if needed?
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

    assets->TranState = tranState;


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

    return assets;
}
