#if 1
#include "handmade_assets__general_purpose.cpp"
#else
// NOTE: memory sentinel logic has to be removed before enabling
#include "handmade_assets__simple_memory.cpp"
#endif

enum finalize_load_asset_operation
{
    FinalizeLoadAsset_None,
    FinalizeLoadAsset_Font,
};

struct load_asset_work
{
    task_with_memory *Task;

    finalize_load_asset_operation FinalizeOperation;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    asset *Asset;
    u32 FinalState;
};

internal void
LoadAssetWorkDirectly(load_asset_work *work)
{
    PlatformAPI.ReadDataFromFile(work->Handle, work->Offset, work->Size, work->Destination);

    if (PlatformNoFileErrors(work->Handle))
    {
        switch (work->FinalizeOperation)
        {
        case FinalizeLoadAsset_None:
        {
            // NOTE: do nothing
        } break;
        case FinalizeLoadAsset_Font:
        {

            loaded_font *loadedFont = &work->Asset->Header->Font;
            hha_font *info = &work->Asset->HHA.Font;

            if (loadedFont && info) {
                for (u32 glyphIndex = 0;
                     glyphIndex < info->GlyphCount;
                     glyphIndex++
                    )
                {
                    hha_font_glyph *glyph = loadedFont->Glyphs + glyphIndex;
                    loadedFont->UnicodeMap[glyph->UnicodeCodePoint] = (u16)glyphIndex;
                }
            }
        } break;
        }
    }

    if (!PlatformNoFileErrors(work->Handle))
    {
        ZeroSize(work->Size, work->Destination);
    }

    CompletePreviousWritesBeforeFutureWrites;

    work->Asset->State = work->FinalState;
}


internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadAssetWork)
{
    load_asset_work *work = (load_asset_work *)data;

    LoadAssetWorkDirectly(work);

    EndTaskWithMemory(work->Task);
}

inline asset_file *
GetFile(game_assets *assets, u32 fileIndex)
{
    Assert(fileIndex < assets->FileCount);
    asset_file *result = assets->Files + fileIndex;
    return result;
}

inline platform_file_handle *
GetFileHandle(game_assets *assets, u32 fileIndex)
{
    platform_file_handle *result = &GetFile(assets, fileIndex)->Handle;
    return result;
}

struct asset_memory_size
{
    u32 Total;
    u32 Data;
    u32 Section;
};

internal void
LoadBitmap(game_assets *assets, bitmap_id id, b32 immidiate)
{
    if (id.Value)
    {
        asset *asset = assets->Assets + id.Value;
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&asset->State, AssetState_Unloaded, AssetState_Queued);
        if (currentState == AssetState_Unloaded)
        {
            task_with_memory *task = 0;

            if (!immidiate)
            {
                task = BeginTaskWithMemory(assets->TranState);
            }
    
            if (immidiate || task)
            {
                hha_bitmap *info = &asset->HHA.Bitmap;

                asset_memory_size memorySize = {};
                u32 w = info->Dim[0];
                u32 h = info->Dim[1];
                memorySize.Section = w * 4;
                memorySize.Data = h * memorySize.Section;
                memorySize.Total = memorySize.Data + sizeof(asset_memory_header);
                
                asset->Header = AcquireAssetMemory(assets, memorySize.Total, id.Value);

                loaded_bitmap *bitmap = &asset->Header->Bitmap;

                bitmap->AlignPercent = info->AlignPercentage;
                bitmap->Width = info->Dim[0];
                bitmap->Height = info->Dim[1];
                bitmap->WidthOverHeight = (r32)info->Dim[0] / (r32)info->Dim[1];
                bitmap->Pitch = memorySize.Section; // NOTE: convention

                bitmap->Memory = asset->Header + 1;

                load_asset_work work;

                work.Task = task;
                work.Asset = asset;
                
                work.Handle = GetFileHandle(assets, asset->FileIndex);
                work.Offset = asset->HHA.DataOffset;
                work.Size = memorySize.Data;
                work.Destination = bitmap->Memory;
                
                work.FinalState = AssetState_Loaded;
                work.FinalizeOperation = FinalizeLoadAsset_None;

                if (immidiate)
                {
                    LoadAssetWorkDirectly(&work);
                }
                else 
                {
                    load_asset_work *taskWork = PushStruct(&task->Arena, load_asset_work);
                    *taskWork = work;
                    PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadAssetWork, taskWork);
                }
            } else {
                // NOTE: if task to load not found return to unloaded
                asset->State = AssetState_Unloaded;
            }
        } else {

            if (immidiate) {
                // NOTE: this is to handle case when two or more
                // immidiate loads execute at the same time
                asset_state volatile *state = (asset_state volatile *)&asset->State;
                while(*state == AssetState_Queued) {}
            }
            
        }
    }
}

internal void
LoadFont(game_assets *assets, font_id id, b32 immidiate)
{
    if (id.Value)
    {
        asset *asset = assets->Assets + id.Value;
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&asset->State, AssetState_Unloaded, AssetState_Queued);
        if (currentState == AssetState_Unloaded)
        {
            task_with_memory *task = 0;

            if (!immidiate)
            {
                task = BeginTaskWithMemory(assets->TranState);
            }
    
            if (immidiate || task)
            {
                hha_font *info = &asset->HHA.Font;

                u32 glyphSize = sizeof(hha_font_glyph) * info->GlyphCount;
                u32 horizontalAdvanceTableSize = sizeof(r32) * info->GlyphCount * info->GlyphCount;
                u32 unicodeMapSize = sizeof(u16) * info->OnePastLastCodePoint;
                u32 sizeData = glyphSize + horizontalAdvanceTableSize;
                u32 sizeTotal = sizeData + sizeof(asset_memory_header) + unicodeMapSize;
                
                asset->Header = AcquireAssetMemory(assets, sizeTotal, id.Value);

                asset_file *file = GetFile(assets, asset->FileIndex);

                loaded_font *font = &asset->Header->Font;
                font->Glyphs = (hha_font_glyph *)(asset->Header + 1);
                font->HorizontalAdvance = (r32 *)((u8*)font->Glyphs + glyphSize);
                font->UnicodeMap = (u16 *)((u8*)font->HorizontalAdvance + horizontalAdvanceTableSize);
                ZeroSize(unicodeMapSize, font->UnicodeMap);

                font->BitmapIdOffset = file->FontBitmapIdOffset;

                load_asset_work work;

                work.Task = task;
                work.Asset = asset;
                
                work.Handle = &file->Handle;
                work.Offset = asset->HHA.DataOffset;
                work.Size = sizeData;
                work.Destination = font->Glyphs;

                work.FinalState = AssetState_Loaded;
                work.FinalizeOperation = FinalizeLoadAsset_Font;

                if (immidiate)
                {
                    LoadAssetWorkDirectly(&work);
                }
                else 
                {
                    load_asset_work *taskWork = PushStruct(&task->Arena, load_asset_work);
                    *taskWork = work;
                    PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadAssetWork, taskWork);
                }
            } else {
                // NOTE: if task to load not found return to unloaded
                asset->State = AssetState_Unloaded;
            }
        } else {

            if (immidiate) {
                // NOTE: this is to handle case when two or more
                // immidiate loads execute at the same time
                asset_state volatile *state = (asset_state volatile *)&asset->State;
                while(*state == AssetState_Queued) {}
            }
            
        }
    }
}

internal void
LoadSound(game_assets *assets, sound_id id)
{
    if (id.Value)
    {
        asset *asset = assets->Assets + id.Value;
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&asset->State, AssetState_Unloaded, AssetState_Queued);
        if (currentState == AssetState_Unloaded)
        {
            task_with_memory *task = BeginTaskWithMemory(assets->TranState);
    
            if (task)
            {
                hha_sound *info = &asset->HHA.Sound;

                asset_memory_size memorySize = {};
                memorySize.Section = info->ChannelCount * sizeof(s16);
                memorySize.Data = info->SampleCount * memorySize.Section;
                memorySize.Total = memorySize.Data + sizeof(asset_memory_header);
                asset->Header = (asset_memory_header *)AcquireAssetMemory(assets, memorySize.Total, id.Value);

                loaded_sound *sound = &asset->Header->Sound;

                sound->SampleCount = info->SampleCount;
                sound->ChannelCount = info->ChannelCount;

                
                u32 channelSize = memorySize.Section;
                
                void *memory = asset->Header+1;

                s16 *soundAt = (s16 *)memory;
                for (u32 i=0; i < sound->ChannelCount; i++)
                {
                    sound->Samples[i] = soundAt;
                    soundAt += channelSize;
                }
                
                load_asset_work *work = PushStruct(&task->Arena, load_asset_work);

                work->Task = task;
                work->Asset = asset;
                
                work->Handle = GetFileHandle(assets, asset->FileIndex);
                work->Offset = asset->HHA.DataOffset;
                work->Size = memorySize.Data;
                work->Destination = memory;
                
                work->FinalState = AssetState_Loaded;
                work->FinalizeOperation = FinalizeLoadAsset_None;
    
                PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadAssetWork, work);
            } else {
                // NOTE: if task to load not found return to unloaded
                AtomicCompareExchange((uint32 *)&asset->State, AssetState_Queued, AssetState_Unloaded);
            }
        }
    }
}

inline bitmap_id
RandomAssetFrom(game_assets *assets, asset_type_id assetType, random_series *series)
{
    bitmap_id result = {};
    asset_type *type = assets->AssetTypes + assetType;

    // NOTE|TODO: is this correct?
    if (type->OnePastLastAssetIndex - type->FirstAssetIndex)
    {
        uint32 count = type->OnePastLastAssetIndex - type->FirstAssetIndex;
        uint32 choice = RandomChoice(series, count);
        result.Value = type->FirstAssetIndex + choice;
    }

    return result;
}

internal uint32
BestMatchAsset(game_assets *assets,
               asset_type_id assetType,
               asset_vector *matchVector,
               asset_vector *weightVector
               )

{
    uint32 result = 0;
    
    int32 bestIndex = 0;
    real32 bestDiff = Real32Maximum;

    asset_type *type = assets->AssetTypes + assetType;
    for (uint32 assetIndex=type->FirstAssetIndex;
         assetIndex < type->OnePastLastAssetIndex;
         ++assetIndex)
    {
        asset *asset = assets->Assets + assetIndex;
        real32 totalWeightedDiff = 0.0f;

        for (uint32 tagIndex = asset->HHA.FirstTagIndex;
             tagIndex < asset->HHA.OnePastLastTagIndex;
             ++tagIndex)
        {
            hha_tag *tag = assets->Tags + tagIndex;
            r32 a = matchVector->E[tag->Id];
            r32 b = tag->Value;
            r32 d0 = AbsoluteValue(a - b);
            r32 d1 = AbsoluteValue(a - assets->TagPeriodRange[tag->Id] * SignOf(a) - b);
            r32 diff = Minimum(d0, d1);
            r32 weightedDifference = weightVector->E[tag->Id] * AbsoluteValue(diff);

            totalWeightedDiff += weightedDifference;
        }

        if (bestDiff > totalWeightedDiff)
        {
            bestDiff = totalWeightedDiff;
            result = assetIndex;
        }
    }
    
    return result;
}

inline bitmap_id
BestMatchBitmap(game_assets *assets,
               asset_type_id assetType,
               asset_vector *matchVector,
               asset_vector *weightVector
               )
{
    bitmap_id result = {BestMatchAsset(assets, assetType, matchVector, weightVector)};
    return result;
}

inline sound_id
BestMatchSound(game_assets *assets,
               asset_type_id assetType,
               asset_vector *matchVector,
               asset_vector *weightVector
               )
{
    sound_id result = {BestMatchAsset(assets, assetType, matchVector, weightVector)};
    return result;
}

inline font_id
BestMatchFont(game_assets *assets,
               asset_vector *matchVector,
               asset_vector *weightVector)
{
    font_id result = {BestMatchAsset(assets, AssetType_Font, matchVector, weightVector)};
    return result;
}

inline uint32
GetFirstAssetId(game_assets* assets, asset_type_id assetType)
{
    uint32 result = 0;
    
    asset_type *type = assets->AssetTypes + assetType;

    if (type->FirstAssetIndex != type->OnePastLastAssetIndex)
    {
        result = type->FirstAssetIndex;
    }

    return result;
}

inline bitmap_id
GetFirstBitmap(game_assets* assets, asset_type_id assetType)
{
    bitmap_id result = {GetFirstAssetId(assets, assetType)};
    return result;
}

inline sound_id
GetFirstSound(game_assets* assets, asset_type_id assetType)
{
    sound_id result = {GetFirstAssetId(assets, assetType)};
    return result;
}

internal game_assets*
AllocateGameAssets(memory_arena *arena, memory_index assetSize, transient_state *tranState)
{
    game_assets *assets = PushStruct(arena, game_assets);

    assets->NextGenerationId = 0;
    assets->InFlightGenerationCount = 0;

    assets->MemorySentinel.Flags = 0;
    assets->MemorySentinel.Size = 0;
    assets->MemorySentinel.Next = &assets->MemorySentinel;
    assets->MemorySentinel.Prev = &assets->MemorySentinel;

    InsertBlock(&assets->MemorySentinel, assetSize, PushSize(arena, assetSize));
    
    assets->LoadedAssetsSentinel.Next = &assets->LoadedAssetsSentinel;
    assets->LoadedAssetsSentinel.Prev = &assets->LoadedAssetsSentinel;

    // SubArena(&assets->Arena, arena, assetSize);
    
    //assets->DEBUGUsedAssetCount = 0;

    for (uint32 tagType=0;
         tagType < Tag_Count;
         ++tagType)
    {
        // NOTE: init to some large number
        assets->TagPeriodRange[tagType] = 1000000.0f;
    }
    assets->TagPeriodRange[Tag_FacingDirection] = 2.0f * Pi32;

#if 1


    // NOTE: skip zeros invalid asset
    assets->TagCount = 1;
    assets->AssetCount = 1;


    {
        platform_file_group fileGroup = PlatformAPI.GetAllFilesOfTypeBegin(PlatformFileType_Asset);
        assets->FileCount = fileGroup.FileCount;
        assets->Files = PushArray(arena, assets->FileCount, asset_file);
        for (u32 i=0; i < assets->FileCount; i++)
        {
            asset_file *file = assets->Files + i;
            file->Handle = PlatformAPI.OpenNextFile(&fileGroup);
            file->TagBase = assets->TagCount;
            file->FontBitmapIdOffset = 0;
            
            ZeroStruct(file->Header);
            PlatformAPI.ReadDataFromFile(&file->Handle, 0, sizeof(file->Header), &file->Header);
            u32 assetTypeArraySize = file->Header.AssetTypeCount * sizeof(hha_asset_type);
            file->AssetTypes = (hha_asset_type *)PushSize(arena, assetTypeArraySize);
            PlatformAPI.ReadDataFromFile(&file->Handle, file->Header.AssetTypeOffset, assetTypeArraySize, file->AssetTypes);

            if (file->Header.MagicValue != HHA_MAGIC_VALUE)
            {
                PlatformAPI.FileError(&file->Handle, "HHA file magic version not found");
            }

            if (file->Header.Version != HHA_VERSION)
            {
                PlatformAPI.FileError(&file->Handle, "HHA bad version");
            }

            
            if (PlatformNoFileErrors(&file->Handle))
            {
                // NOTE: first asset in every hha is null asset
                // (reserved), so do not count it when loading
                assets->TagCount += file->Header.TagCount - 1;
                assets->AssetCount += file->Header.AssetCount - 1;
            }
            else
            {
                InvalidCodePath;
            }
        }

        PlatformAPI.GetAllFilesOfTypeEnd(&fileGroup);
    }

    // allocate all metadata space
    assets->Assets = PushArray(arena, assets->AssetCount, asset);
    assets->Assets = PushArray(arena, assets->AssetCount, asset);
    assets->Tags = PushArray(arena, assets->TagCount, hha_tag);

    ZeroStruct(assets->Tags[0]);

    // NOTE: zero out null asset

    // NOTE: load tags
    for (u32 i=0; i < assets->FileCount; i++)
    {
        asset_file *file = assets->Files + i;
        if (PlatformNoFileErrors(&file->Handle)) {
            u32 tagArraySize = (file->Header.TagCount - 1) * sizeof(hha_tag);
            PlatformAPI.ReadDataFromFile(&file->Handle, file->Header.TagOffset + sizeof(hha_tag), tagArraySize, assets->Tags + file->TagBase);
        }
    }

    u32 assetCount = 0;
    ZeroStruct(*(assets->Assets + assetCount));
    assetCount++;

    // TODO: do this in a way to scale gracefully
    for (u32 destTypeId = AssetType_None; destTypeId < AssetType_Count; destTypeId++)
    {
        asset_type *destType = assets->AssetTypes + destTypeId;
        destType->FirstAssetIndex = assetCount;
        
        for (u32 i=0; i < assets->FileCount; i++)
        {
            asset_file *file = assets->Files + i;

            for (u32 sourceIdx = 0; sourceIdx < file->Header.AssetTypeCount; sourceIdx++)
            {
                hha_asset_type *sourceType = file->AssetTypes + sourceIdx;
                if (sourceType->TypeId == destTypeId) {
                    if (sourceType->TypeId == AssetType_FontGlyph)
                    {
                        file->FontBitmapIdOffset = assetCount - sourceType->FirstAssetIndex;
                    }

                    if (sourceType->TypeId == AssetType_Font)
                    {
                        int a = 10;
                    }
                    
                    u32 assetCountForType (sourceType->OnePastLastAssetIndex - sourceType->FirstAssetIndex);
                    u64 firstAssetToReadFrom = file->Header.AssetOffset + sourceType->FirstAssetIndex * sizeof(hha_asset);

                    temporary_memory temp = BeginTemporaryMemory(&tranState->TransientArena);
                    hha_asset *hhaAssetArray = PushArray(&tranState->TransientArena, assetCountForType, hha_asset);
                    PlatformAPI.ReadDataFromFile(&file->Handle, firstAssetToReadFrom, assetCountForType * sizeof(hha_asset), hhaAssetArray);
                    
                    for (u32 assetIndex = 0; assetIndex < assetCountForType; assetIndex++)
                    {
                        asset *asset = assets->Assets + assetCount++;
                        //asset->State = AssetState_Unloaded;
                        asset->FileIndex = i;
                        asset->HHA = hhaAssetArray[assetIndex];
                        
                        if (asset->HHA.FirstTagIndex == 0) {
                            asset->HHA.FirstTagIndex = asset->HHA.OnePastLastTagIndex = 0;
                        } else {
                            asset->HHA.FirstTagIndex += file->TagBase - 1;
                            asset->HHA.OnePastLastTagIndex += file->TagBase - 1;
                        }
                    }

                    EndTemporaryMemory(temp);
                }
            }
        }
        
        destType->OnePastLastAssetIndex = assetCount;
    }

    Assert(assetCount == assets->AssetCount);
    //Assert(tagCount == assets->TagCount);

    #endif
    
    assets->TranState = tranState;

    return assets;
}

inline u32
BeginGeneration(game_assets *assets)
{
    BeginAssetLock(assets);
    Assert(assets->InFlightGenerationCount < ArrayCount(assets->InFlightGenerations));
    u32 result = assets->NextGenerationId++;

    assets->InFlightGenerations[assets->InFlightGenerationCount++] = result;
    EndAssetLock(assets);

    return result;
}

inline void
EndGeneration(game_assets *assets, u32 generationId)
{
    BeginAssetLock(assets);

    for (u32 index = 0;
         index < assets->InFlightGenerationCount;
         index++)
    {
        if (assets->InFlightGenerations[index] == generationId) {
            assets->InFlightGenerations[index] = assets->InFlightGenerations[--assets->InFlightGenerationCount];
            break;
        }
    }

    EndAssetLock(assets);
}

inline u32
GetGlyphIndexForCodePoint(hha_font *info, loaded_font *font, u32 requestedCodePoint)
{
    u32 result = 0;
    if (requestedCodePoint < info->OnePastLastCodePoint) {
        result = (u32)font->UnicodeMap[requestedCodePoint];
    }
    
    return result;
}

internal bitmap_id
GetBitmapForGlyph(game_assets *assets, hha_font *info, loaded_font *font, u32 requestedCodePoint)
{
    u32 glyphIndex = GetGlyphIndexForCodePoint(info, font, requestedCodePoint);
    Assert(glyphIndex < info->GlyphCount);

    bitmap_id result = font->Glyphs[glyphIndex].Id;
    result.Value += font->BitmapIdOffset;
    return result;
}

internal r32
GetHorizontalAdvanceForPair(hha_font *info, loaded_font *font, u32 prev, u32 current)
{
    r32 result = 0.0f;
    u32 prevGlyphIndex = GetGlyphIndexForCodePoint(info, font, prev);
    u32 currentGlyphIndex = GetGlyphIndexForCodePoint(info, font, current);
    result = font->HorizontalAdvance[prevGlyphIndex * info->GlyphCount + currentGlyphIndex];

    return result;
}

internal r32
GetVerticalLineAdvance(hha_font *info)
{
    r32 result = info->LineAdvance;
    return result;
}