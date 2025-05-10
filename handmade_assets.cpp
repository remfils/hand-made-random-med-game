struct load_asset_work
{
    task_with_memory *Task;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    asset_slot *Slot;
    asset_state FinalState;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadAssetWork)
{
    load_asset_work *work = (load_asset_work *)data;

    PlatformAPI.ReadDataFromFile(work->Handle, work->Offset, work->Size, work->Destination);

    CompletePreviousWritesBeforeFutureWrites;

    // TODO: handle not loading data somehow?

    if (PlatformNoFileErrors(work->Handle))
    {
        work->Slot->State = work->FinalState;
    }

    EndTaskWithMemory(work->Task);
}

inline platform_file_handle *
GetFileHandle(game_assets *assets, u32 fileIndex)
{
    Assert(fileIndex < assets->FileCount);
    platform_file_handle *result = assets->Files[fileIndex].Handle;
    return result;
}

internal void
LoadBitmap(game_assets *assets, bitmap_id id)
{
    if (id.Value)
    {
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&assets->Slots[id.Value].State, AssetState_Unloaded, AssetState_Queued);
        if (currentState == AssetState_Unloaded)
        {
            task_with_memory *task = BeginTaskWithMemory(assets->TranState);
    
            if (task)
            {
                asset *asset = assets->Assets + id.Value;
                hha_bitmap *info = &asset->HHA.Bitmap;
                loaded_bitmap *bitmap = PushStruct(&assets->Arena, loaded_bitmap);

                bitmap->AlignPercent = info->AlignPercentage;
                bitmap->Width = info->Dim[0];
                bitmap->Height = info->Dim[1];
                bitmap->WidthOverHeight = (r32)info->Dim[0] / (r32)info->Dim[1];
                bitmap->Pitch = info->Dim[0] * 4; // NOTE: convention

                // TODO: memorySize should be with size?
                u32 memorySize = bitmap->Pitch * bitmap->Height;

                bitmap->Memory = PushSize(&assets->Arena, memorySize);

                load_asset_work *work = PushStruct(&task->Arena, load_asset_work);

                work->Task = task;
                work->Slot = assets->Slots + id.Value;
                work->Slot->Bitmap = bitmap;
                
                work->Handle = GetFileHandle(assets, asset->FileIndex);
                work->Offset = asset->HHA.DataOffset;
                work->Size = memorySize;
                work->Destination = bitmap->Memory;
                
                work->FinalState = AssetState_Loaded;
    
                PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadAssetWork, work);
            }
        }
    }
}

internal void
LoadSound(game_assets *assets, sound_id id)
{
    if (id.Value)
    {
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&assets->Slots[id.Value].State, AssetState_Unloaded, AssetState_Queued);
        if (currentState == AssetState_Unloaded)
        {
            task_with_memory *task = BeginTaskWithMemory(assets->TranState);
    
            if (task)
            {
                asset *asset = assets->Assets + id.Value;
                hha_sound *info = &asset->HHA.Sound;
                loaded_sound *sound = PushStruct(&assets->Arena, loaded_sound);

                sound->SampleCount = info->SampleCount;
                sound->ChannelCount = info->ChannelCount;

                u32 channelSize = sound->ChannelCount * sizeof(s16);
                u32 memorySize = sound->SampleCount * channelSize;

                void *memory = PushSize(&assets->Arena, memorySize);

                s16 *soundAt = (s16 *)memory;
                for (u32 i=0; i < sound->ChannelCount; i++)
                {
                    sound->Samples[i] = soundAt;
                    soundAt += channelSize;
                }
                
                load_asset_work *work = PushStruct(&task->Arena, load_asset_work);

                work->Task = task;
                work->Slot = assets->Slots + id.Value;
                work->Slot->Sound = sound;
                
                work->Handle = GetFileHandle(assets, asset->FileIndex);
                work->Offset = asset->HHA.DataOffset;
                work->Size = memorySize;
                work->Destination = memory;
                
                work->FinalState = AssetState_Loaded;
    
                PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadAssetWork, work);
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

inline uint32
GetFirstSlotId(game_assets* assets, asset_type_id assetType)
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
    bitmap_id result = {GetFirstSlotId(assets, assetType)};
    return result;
}

inline sound_id
GetFirstSound(game_assets* assets, asset_type_id assetType)
{
    sound_id result = {GetFirstSlotId(assets, assetType)};
    return result;
}

internal game_assets*
AllocateGameAssets(memory_arena *arena, memory_index assetSize, transient_state *tranState)
{
    game_assets *assets = PushStruct(arena, game_assets);

    SubArena(&assets->Arena, arena, assetSize);
    
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
        platform_file_group *fileGroup = PlatformAPI.GetAllFilesOfTypeBegin("*.hha");
        assets->FileCount = fileGroup->FileCount;
        assets->Files = PushArray(arena, assets->FileCount, asset_file);
        for (u32 i=0; i < assets->FileCount; i++)
        {
            asset_file *file = assets->Files + i;
            file->Handle = PlatformAPI.OpenNextFile(fileGroup);
            file->TagBase = assets->TagCount;
            
            ZeroStruct(file->Header);
            PlatformAPI.ReadDataFromFile(file->Handle, 0, sizeof(file->Header), &file->Header);
            u32 assetTypeArraySize = file->Header.AssetTypeCount * sizeof(hha_asset_type);
            file->AssetTypes = (hha_asset_type *)PushSize(arena, assetTypeArraySize);
            PlatformAPI.ReadDataFromFile(file->Handle, file->Header.AssetTypeOffset, assetTypeArraySize, file->AssetTypes);

            if (file->Header.MagicValue != HHA_MAGIC_VALUE)
            {
                PlatformAPI.FileError(file->Handle, "HHA file magic version not found");
            }

            if (file->Header.Version != HHA_VERSION)
            {
                PlatformAPI.FileError(file->Handle, "HHA bad version");
            }

            
            if (PlatformNoFileErrors(file->Handle))
            {
                // NOTE: first slot in every hha is null asset
                // (reserved), so do not count it when loading
                assets->TagCount += file->Header.TagCount - 1;
                assets->AssetCount += file->Header.AssetCount - 1;
            }
            else
            {
                InvalidCodePath;
            }
        }

        PlatformAPI.GetAllFilesOfTypeEnd(fileGroup);
    }

    // allocate all metadata space
    assets->Assets = PushArray(arena, assets->AssetCount, asset);
    assets->Slots = PushArray(arena, assets->AssetCount, asset_slot);
    assets->Tags = PushArray(arena, assets->TagCount, hha_tag);

    ZeroStruct(assets->Tags[0]);

    // NOTE: zero out null asset

    // NOTE: load tags
    for (u32 i=0; i < assets->FileCount; i++)
    {
        asset_file *file = assets->Files + i;
        if (PlatformNoFileErrors(file->Handle)) {
            u32 tagArraySize = (file->Header.TagCount - 1) * sizeof(hha_tag);
            PlatformAPI.ReadDataFromFile(file->Handle, file->Header.TagOffset + sizeof(hha_tag), tagArraySize, assets->Tags + file->TagBase);
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
                    u32 assetCountForType (sourceType->OnePastLastAssetIndex - sourceType->FirstAssetIndex);
                    u64 firstAssetToReadFrom = file->Header.AssetOffset + sourceType->FirstAssetIndex * sizeof(hha_asset);

                    temporary_memory temp = BeginTemporaryMemory(&tranState->TransientArena);
                    hha_asset *hhaAssetArray = PushArray(&tranState->TransientArena, assetCountForType, hha_asset);
                    PlatformAPI.ReadDataFromFile(file->Handle, firstAssetToReadFrom, assetCountForType * sizeof(hha_asset), hhaAssetArray);
                    
                    for (u32 assetIndex = 0; assetIndex < assetCountForType; assetIndex++)
                    {
                        asset *asset = assets->Assets + assetCount++;
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

