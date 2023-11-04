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
DEBUGLoadBMP(char *filename, int32 topDownAlignX, int32 topDownAlignY)
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

        result.AlignPercent = ToV2(
                                 SafeRatio_0((real32)topDownAlignX, (real32)result.Width),
                                 SafeRatio_0((real32)((result.Height - 1) - topDownAlignY), (real32)result.Height)
                                 );
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

internal loaded_bitmap
DEBUGLoadBMP(char *filename)
{
    loaded_bitmap result = DEBUGLoadBMP(filename, 0, 0);
    result.AlignPercent = ToV2(0.5f, 0.5f);
    return result;
}

struct load_bitmap_work
{
    game_assets *Assets;
    char *FileName;
    bitmap_id Id;
    task_with_memory *Task;
    loaded_bitmap *Bitmap;
    int32 TopDownAlignX;
    int32 TopDownAlignY;
    asset_state FinalState;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork)
{
    load_bitmap_work *work = (load_bitmap_work *)data;

    // TODO: remove this
    *work->Bitmap = DEBUGLoadBMP(work->FileName, work->TopDownAlignX, work->TopDownAlignY);

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

                switch (id.Value)
                {
                case AssetType_Loaded: {
                    work->FileName = "../data/new-bg-code.bmp";
                    work->TopDownAlignX = 0;
                    work->TopDownAlignY = 0;
                } break;
                case AssetType_EnemyDemo: {
                    work->FileName = "../data/test2/enemy_demo.bmp";
                    work->TopDownAlignX = 61;
                    work->TopDownAlignY = 197;
                }   break;
                case AssetType_FamiliarDemo: {
                    work->FileName = "../data/test2/familiar_demo.bmp";
                    work->TopDownAlignX = 58;
                    work->TopDownAlignY = 203;
                }   break;
                case AssetType_WallDemo: {
                    work->FileName = "../data/test2/wall_demo.bmp";
                    work->TopDownAlignX = 64 / 2;
                    work->TopDownAlignY = 64 / 2;
                }   break;
                case AssetType_SwordDemo: {
                    work->FileName = "../data/test2/sword_demo.bmp";
                    work->TopDownAlignX = 12;
                    work->TopDownAlignY = 60;
                }   break;
                case AssetType_Stairway: {
                    work->FileName = "../data/old_random_med_stuff/Stairway2.bmp";
                    work->TopDownAlignX = 0;
                    work->TopDownAlignY = 0;
                }   break;
                }

                PlatformAddEntry(assets->TranState->LowPriorityQueue, DoLoadBitmapWork, work);
            }
        }
    }
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
}

internal game_assets*
AllocateGameAssets(memory_arena *arena, memory_index assetSize, transient_state *tranState)
{
    game_assets *assets = PushStruct(arena, game_assets);

    SubArena(&assets->Arena, arena, assetSize);

    assets->BitmapCount = AssetType_Count;
    assets->Bitmaps = PushArray(arena, assets->BitmapCount, asset_slot);
    
    assets->AssetCount = AssetType_Count;
    assets->Assets = PushArray(arena, assets->BitmapCount, asset);

    assets->TagCount = 0;
    assets->Tags = 0;
    
    assets->SoundCount = 1;
    assets->Sounds = PushArray(arena, assets->SoundCount, asset_slot);;
    
    assets->TranState = tranState;

    for (uint32 assetId =0; assetId < AssetType_Count; assetId++)
    {
        asset_type *type = assets->AssetTypes + assetId;
        type->FirstAssetIndex = assetId;
        type->OnePastLastAssetIndex = assetId + 1;

        asset *asset = assets->Assets + type->FirstAssetIndex;
        asset->FirstTagIndex = 0;
        asset->OnePastLastTagIndex = 0;
        asset->SlotId = type->FirstAssetIndex;
    }

    assets->Grass[0] = DEBUGLoadBMP("../data/grass001.bmp");
    assets->Grass[1] = DEBUGLoadBMP("../data/grass002.bmp");
    assets->Ground[0] = DEBUGLoadBMP("../data/ground001.bmp");
    assets->Ground[1] = DEBUGLoadBMP("../data/ground002.bmp");
        
    assets->Hero[0].Character = DEBUGLoadBMP("../data/old_random_med_stuff/stand_right.bmp", 60, 195);
    assets->Hero[1].Character = DEBUGLoadBMP("../data/old_random_med_stuff/stand_up.bmp", 60, 185);
    assets->Hero[2].Character = DEBUGLoadBMP("../data/old_random_med_stuff/stand_left.bmp", 60, 195);
    assets->Hero[3].Character = DEBUGLoadBMP("../data/old_random_med_stuff/stand_down.bmp", 60, 185);

    return assets;
}
