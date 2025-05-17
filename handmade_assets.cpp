struct load_asset_work
{
    task_with_memory *Task;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    asset *Asset;
    u32 FinalState;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadAssetWork)
{
    load_asset_work *work = (load_asset_work *)data;

    PlatformAPI.ReadDataFromFile(work->Handle, work->Offset, work->Size, work->Destination);

    CompletePreviousWritesBeforeFutureWrites;

    // TODO: handle not loading data somehow?

    if (!PlatformNoFileErrors(work->Handle))
    {
        
    }
    work->Asset->State = work->FinalState;

    EndTaskWithMemory(work->Task);
}

inline platform_file_handle *
GetFileHandle(game_assets *assets, u32 fileIndex)
{
    Assert(fileIndex < assets->FileCount);
    platform_file_handle *result = assets->Files[fileIndex].Handle;
    return result;
}

inline void*
AcquireAssetMemory(game_assets *assets, memory_index size)
{
    void *result = PlatformAPI.AllocateMemory(size);
    if (result) {
        assets->TotalMemoryUsed += size;
    }
    return result;
}

inline void
ReleaseAssetMemory(game_assets *assets, memory_index size, void *memory)
{
    if (memory) {
        PlatformAPI.FreeMemory(memory);
        assets->TotalMemoryUsed -= size;
    }
}

struct asset_memory_size
{
    u32 Total;
    u32 Data;
    u32 Section;
};

inline void
InsertAssetHeaderAtFront(game_assets *assets, asset_memory_header *header)
{
    asset_memory_header *sentinel = &assets->LoadedAssetsSentinel;

    header->Next = sentinel->Next;
    header->Prev = sentinel;

    header->Next->Prev = header;
    header->Prev->Next = header;
}

internal void
AddAssetHeaderToList(game_assets *assets, u32 assetIndex, asset_memory_size size)
{
    asset_memory_header *header = assets->Assets[assetIndex].Header;

    header->AssetIndex = assetIndex;
    header->TotalSize = size.Total;

    InsertAssetHeaderAtFront(assets, header);
}

internal void
RemoveAssetHeaderFromList(asset_memory_header *header)
{
    header->Prev->Next = header->Next;
    header->Next->Prev = header->Prev;

    header->Prev = header->Next = 0;
}

internal void
LoadBitmap(game_assets *assets, bitmap_id id, b32 locked)
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
                hha_bitmap *info = &asset->HHA.Bitmap;

                asset_memory_size memorySize = {};
                u32 w = SafeTruncateToUInt16(info->Dim[0]);
                u32 h = SafeTruncateToUInt16(info->Dim[1]);
                memorySize.Section = SafeTruncateToUInt16(w * 4);
                memorySize.Data = h * memorySize.Section;
                memorySize.Total = memorySize.Data + sizeof(asset_memory_header);
                
                asset->Header = (asset_memory_header *)AcquireAssetMemory(assets, memorySize.Total);

                loaded_bitmap *bitmap = &asset->Header->Bitmap;

                bitmap->AlignPercent = info->AlignPercentage;
                bitmap->Width = SafeTruncateToUInt16(info->Dim[0]);
                bitmap->Height = SafeTruncateToUInt16(info->Dim[1]);
                bitmap->WidthOverHeight = (r32)info->Dim[0] / (r32)info->Dim[1];
                bitmap->Pitch = SafeTruncateToUInt16(memorySize.Section); // NOTE: convention

                bitmap->Memory = asset->Header + 1;

                load_asset_work *work = PushStruct(&task->Arena, load_asset_work);

                work->Task = task;
                work->Asset = asset;
                
                work->Handle = GetFileHandle(assets, asset->FileIndex);
                work->Offset = asset->HHA.DataOffset;
                work->Size = memorySize.Data;
                work->Destination = bitmap->Memory;
                
                work->FinalState = AssetState_Loaded | (locked ? AssetState_Lock : 0);

                if (!locked) {
                    AddAssetHeaderToList(assets, id.Value, memorySize );
                }
    
                PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadAssetWork, work);
            } else {
                // NOTE: if task to load not found return to unloaded
                AtomicCompareExchange((uint32 *)&asset->State, AssetState_Queued, AssetState_Unloaded);
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
                asset->Header = (asset_memory_header *)AcquireAssetMemory(assets, memorySize.Total);

                loaded_sound *sound = &asset->Header->Sound;

                sound->SampleCount = info->SampleCount;
                sound->ChannelCount = info->ChannelCount;

                
                u32 channelSize = memorySize.Section;
                
                void *memory = asset->Header+1;

                AddAssetHeaderToList(assets, id.Value, memorySize);

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

    assets->TotalMemoryUsed = 0;
    assets->TargetMemoryUsed = assetSize; // TODO:

    assets->LoadedAssetsSentinel.Next = &assets->LoadedAssetsSentinel;
    assets->LoadedAssetsSentinel.Prev = &assets->LoadedAssetsSentinel;

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

        PlatformAPI.GetAllFilesOfTypeEnd(fileGroup);
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

inline void
MoveHeaderToFront(game_assets *assets, asset *asset)
{
    if (!IsAssetLocked(asset)) {
        asset_memory_header *header = asset->Header;

        RemoveAssetHeaderFromList(header);
        InsertAssetHeaderAtFront(assets, header);
    }
}

inline void
EvictAsset(game_assets *assets, asset *asset)
{
    u32 assetIndex = asset->Header->AssetIndex;
    Assert(GetAssetState(asset) == AssetState_Loaded);

    RemoveAssetHeaderFromList(asset->Header);

    ReleaseAssetMemory(assets, asset->Header->TotalSize, asset->Header);
    
    asset->State = AssetState_Unloaded;
    asset->Header = 0;
}

internal void
EvictAssetsAsNecessary(game_assets *assets)
{
    while (assets->TotalMemoryUsed > assets->TargetMemoryUsed)
    {
        asset_memory_header *header = assets->LoadedAssetsSentinel.Prev;
        if (header != &assets->LoadedAssetsSentinel) {
            asset *asset = assets->Assets + header->AssetIndex;
            if (GetAssetState(asset) >= AssetState_Loaded) {
                EvictAsset(assets, asset);
            }
        } else {
            InvalidCodePath;
            break;
        }
    }
    
}