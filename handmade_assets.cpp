
#if 0
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


// TODO: remove once assets are done
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

#pragma pack(push, 1)
struct WAVE_header
{
    uint32 ChunkId;
    uint32 ChunkSize;
    uint32 Format;
};

#define RIFF_CODE(a,b,c,d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | (((uint32)d) << 24))

enum
{
    WAVE_ChunkId_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkId_data = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_ChunkId_fact = RIFF_CODE('f', 'a', 'c', 't'),
    WAVE_ChunkId_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkId_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct WAVE_chunk_header
{
    uint32 Id;
    uint32 Size;
};

struct WAVE_fmt_header
{
    uint16 AudioFormat;    // (2 байта) Аудио формат, список допустипых форматов. Для PCM = 1 (то есть, Линейное квантование). Значения, отличающиеся от 1, обозначают некоторый формат сжатия.
    uint16 NumChannels;    // (2 байта) Количество каналов. Моно = 1, Стерео = 2 и т.д.
    uint32 SampleRate;     // (4 байта) Частота дискретизации. 8000 Гц, 44100 Гц и т.д.
    uint32 ByteRate;       // (4 байта) Количество байт, переданных за секунду воспроизведения.
    uint16 BlockAlign;     // (2 байта) Количество байт для одного сэмпла, включая все каналы.
    uint16 BitsPerSample;  // (2 байта) Количество бит в сэмпле. Так называемая "глубина" или точность звучания. 8 бит, 16 бит и т.д.
    // uint32 Subchunk2Id;    // (4 байта) Содержит символы "data" 0x64617461
    uint16 cbSize;
    uint16 wValidBitsPerSample;
    uint32 Subchunk2Size;  // (4 байта) Количество байт в области данных.
    uint8 SubFormat[16];// wtf?
};
#pragma pack(pop)


struct riff_iterator
{
    uint8 *At;
    uint8 *Stop;
};


// ParseChunkAt(readResult.Content, header + 1)

inline riff_iterator
ParseChunkAt(void *at, void *stop)
{
    riff_iterator result;

    result.At = (uint8 *)at;
    result.Stop = (uint8 *)stop;

    return result;
}

inline bool32
IsChunkValid(riff_iterator iter)
{
    bool32 result = iter.At < iter.Stop;
    return result;
}

inline riff_iterator
NextChunk(riff_iterator iter)
{
    WAVE_chunk_header *chunk = (WAVE_chunk_header *)iter.At;

    // uint32 size = (chunk->Size + 1) & ~1;
    uint32 size = chunk->Size;
    iter.At += sizeof(WAVE_chunk_header) + size;
    return iter;
}

inline void*
GetChunkData(riff_iterator iter)
{
    void *result = iter.At + sizeof(WAVE_chunk_header);
    return result;
}

inline uint32
GetChunkType(riff_iterator iter)
{
    WAVE_chunk_header *chunk = (WAVE_chunk_header *)iter.At;
    uint32 result = chunk->Id;
    return result;
}

inline uint32
GetChunkSize(riff_iterator iter)
{
    WAVE_chunk_header *chunk = (WAVE_chunk_header *)iter.At;
    uint32 result = chunk->Size;
    return result;
}

internal loaded_sound
DEBUGLoadWAV(char *filename, memory_arena *arena, uint32 sectionFirstSampleIndex, uint32 sectionSampleCount)
{
    debug_read_file_result readResult = DEBUG_ReadEntireFile(filename);
    
    loaded_sound result = {};

    if (readResult.Content)
    {
        WAVE_header *header = (WAVE_header *)readResult.Content;
        Assert(header->ChunkId == WAVE_ChunkId_RIFF);
        Assert(header->Format == WAVE_ChunkId_WAVE);

        uint32 channelCount = 0;
        uint32 sampleDataSize = 0;
        real32 *sampleData = 0;
        for (
             riff_iterator iter = ParseChunkAt(header + 1, (uint8 *)(header + 1) + header->ChunkSize - 4);
             IsChunkValid(iter);
             iter = NextChunk(iter)
             )
        {
            switch (GetChunkType(iter))
            {
            case WAVE_ChunkId_fmt: {
                WAVE_fmt_header *fmt = (WAVE_fmt_header *)GetChunkData(iter);
                channelCount = fmt->NumChannels;

                Assert(fmt->AudioFormat == 3);
            } break;
            case WAVE_ChunkId_data: {
                sampleData = (real32 *)GetChunkData(iter);
                sampleDataSize = GetChunkSize(iter);
            } break;
            case WAVE_ChunkId_fact: {
                uint32 *numberOfSamplesInChannel = (uint32 *)GetChunkData(iter);
                int a = 3;
                // sampleDataSize = GetChunkSize(iter);
            } break;
            }
        }

        #if 1
        Assert(channelCount && sampleData);

        result.ChannelCount = channelCount;
        u32 sampleCount = sampleDataSize / (channelCount * sizeof(real32)); // TODO: uint16 in Casey code is correct?

        if (result.ChannelCount == 1)
        {
            result.Samples[0] = (int16 *)sampleData;
            result.Samples[1] = 0;
        }
        else if (result.ChannelCount == 2)
        {
            result.Samples[0] = PushArray(arena, sampleCount, int16);
            result.Samples[1] = PushArray(arena, sampleCount, int16);;

            for (uint32 sampleIndex=0;
                 sampleIndex < sampleCount;
                 sampleIndex++)
            {
                real32 leftChannel = sampleData[2 * sampleIndex];
                real32 rightChannel = sampleData[2 * sampleIndex + 1];

                result.Samples[0][sampleIndex] = (int16)(leftChannel * 32767.0f);
                result.Samples[1][sampleIndex] = (int16)(rightChannel * 32767.0f);
            }
        }
        else
        {
            // TODO: error
        }

        #endif

        // TODO: propper load of channels
        b32 atEnd = true;
        result.ChannelCount = 1;
        
        if (sectionSampleCount) {
            Assert(sectionFirstSampleIndex + sectionSampleCount <= sampleCount);
            atEnd = ((sectionFirstSampleIndex + sectionSampleCount) == sampleCount);
            sampleCount = sectionSampleCount;
            for (uint32 chIndex=0; chIndex < result.ChannelCount; chIndex++) {
                result.Samples[chIndex] += sectionFirstSampleIndex;
            }
        }
        if (atEnd) {
            // TODO: all sounds have to be padded with their subsequent
            // sound out to 8 samples past their end.
            for (u32 i=0; i < result.ChannelCount; i++)
            {
                for (u32 sampleIndex = sampleCount; sampleIndex < sampleCount + 8; sampleIndex++)
                {
                    result.Samples[i][sampleIndex] = 0;
                }
            }
            
        }

        result.SampleCount = sampleCount;
    }

    return result;
}

#endif

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


// TODO: remove
struct load_bitmap_work
{
    game_assets *Assets;
    bitmap_id Id;
    task_with_memory *Task;
    loaded_bitmap *Bitmap;

    asset_state FinalState;
};
/*
internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork)
{
    load_bitmap_work *work = (load_bitmap_work *)data;

    // TODO: remove this

    

    // TODO: do loading
    //*work->Bitmap = DEBUGLoadBMP(info->FileName, info->AlignPercent);
    
    
    
    
    CompletePreviousWritesBeforeFutureWrites;
    
    work->Assets->Slots[work->Id.Value].Bitmap = work->Bitmap;
    work->Assets->Slots[work->Id.Value].State = work->FinalState;

    EndTaskWithMemory(work->Task);
}
*/

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
                hha_asset *asset = assets->Assets + id.Value;
                hha_bitmap *info = &asset->Bitmap;
                loaded_bitmap *bitmap = PushStruct(&assets->Arena, loaded_bitmap);

                bitmap->AlignPercent = info->AlignPercentage;
                bitmap->Width = info->Dim[0];
                bitmap->Height = info->Dim[1];
                bitmap->WidthOverHeight = (r32)info->Dim[0] / (r32)info->Dim[1];
                bitmap->Pitch = info->Dim[0] * 4; // NOTE: convention

                u32 memorySize = bitmap->Pitch * bitmap->Height;
                bitmap->Memory = PushSize(&task->Arena, memorySize);

                load_asset_work *work = PushStruct(&task->Arena, load_asset_work);

                work->Task = task;
                work->Slot->Bitmap = bitmap;
                
                work->Handle = ;
                work->Offset = asset->DataOffset;
                work->Size = memorySize;
                work->Destination = bitmap->Memory;
                
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

    if (type->FirstAssetIndex && type->OnePastLastAssetIndex)
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
        hha_asset *asset = assets->Assets + assetIndex;
        real32 totalWeightedDiff = 0.0f;

        for (uint32 tagIndex = asset->FirstTagIndex;
             tagIndex < asset->OnePastLastTagIndex;
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

struct load_sound_work
{
    game_assets *Assets;
    sound_id Id;
    task_with_memory *Task;
    loaded_sound *Sound;

    asset_state FinalState;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoLoadSoundWork)
{
    load_sound_work *work = (load_sound_work *)data;

    // TODO: remove this

    hha_asset *asset = work->Assets->Assets + work->Id.Value;
    hha_sound *info = &asset->Sound;
    // TODO: proper sound load
    //*work->Sound = DEBUGLoadWAV(info->FileName, &work->Assets->Arena, info->FirstSampleIndex, info->SampleCount);

    loaded_sound *sound = work->Sound;

    sound->SampleCount = info->SampleCount;
    sound->ChannelCount = info->ChannelCount;
    
    u64 sampleDataOffset = asset->DataOffset;
    for (u32 i=0; i< info->ChannelCount; i++) {
        sound->Samples[i] = (s16*)(work->Assets->HHAContent + sampleDataOffset);
        sampleDataOffset += info->SampleCount * sizeof(s16);
    }

    CompletePreviousWritesBeforeFutureWrites;
    
    work->Assets->Slots[work->Id.Value].Sound = work->Sound;
    work->Assets->Slots[work->Id.Value].State = work->FinalState;

    EndTaskWithMemory(work->Task);
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
                load_sound_work *work = PushStruct(&task->Arena, load_sound_work);

                work->Assets = assets;
                work->Id = id;
                work->Sound = PushStruct(&assets->Arena, loaded_sound);

                work->Task = task;
                work->FinalState = AssetState_Loaded;
    
                PlatformAPI.AddEntry(assets->TranState->LowPriorityQueue, DoLoadSoundWork, work);
            }
        }
    }
}

#if 0
internal void 
BeginAssetType(game_assets *assets, asset_type_id id)
{
    assets->DEBUGCurrentAssetType = assets->AssetTypes + id;
    assets->DEBUGCurrentAssetType->FirstAssetIndex = assets->DEBUGUsedAssetCount;
    assets->DEBUGCurrentAssetType->OnePastLastAssetIndex = assets->DEBUGCurrentAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(game_assets *assets, char *fileName, v2 alignPercent={0.5f, 0.5f})
{
    Assert(assets->DEBUGCurrentAssetType);
    Assert(assets->DEBUGCurrentAssetType->OnePastLastAssetIndex < assets->AssetCount);

    u32 assetIndex = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset *asset = assets->Assets + assetIndex;
    asset->FirstTagIndex = assets->DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;
    asset->Bitmap.AlignPercent = alignPercent;
    asset->Bitmap.FileName = PushString(&assets->Arena, fileName);

    assets->DEBUGCurrentAsset = asset;
    return {assetIndex};
}

internal sound_id
AddSoundAsset(game_assets *assets, char *fileName, uint32 firstSampleIndex=0, uint32 sampleCount=0)
{
    Assert(assets->DEBUGCurrentAssetType);
    Assert(assets->DEBUGCurrentAssetType->OnePastLastAssetIndex < assets->AssetCount);

    u32 assetIndex = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset *asset = assets->Assets + assetIndex;
    asset->FirstTagIndex = assets->DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;

    asset->Sound.FileName = PushString(&assets->Arena, fileName);
    asset->Sound.NextIdToPlay.Value = 0;
    asset->Sound.FirstSampleIndex = firstSampleIndex;
    asset->Sound.SampleCount = sampleCount;

    assets->DEBUGCurrentAsset = asset;
    return {assetIndex};
}


internal void
AddAssetTag(game_assets *assets, hha_tag_id tagId, real32 tagValue)
{
    Assert(assets->DEBUGCurrentAsset);
    ++assets->DEBUGCurrentAsset->OnePastLastTagIndex;
    hha_tag *tag = assets->Tags + assets->DEBUGUsedTagCount++;
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

#endif

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

#if 0


    assets->TagCount = 0;
    assets->AssetCount = 0;


    {
        platform_file_group fileGroup = PlatformAPI.GetAllFilesOfTypeBegin("hha");
        assets->FileCount = fileGroup.FileCount;
        assets->Files = PushArray(arena, assets->FileCount, asset_file);
        for (u32 i=0; i < assets->FileCount; i++)
        {
            asset_file *file = assets->Files + i;
            file->Handle = PlatformAPI.OpenFile(fileGroup, i);
            
            ZeroStruct(file->Header);
            PlatformAPI.ReadDataFromFile(file->Handle, 0, sizeof(file->Header), &file->Header);
            u32 assetTypeArraySize = file->Header.AssetTypeCount * sizeof(hha_asset_type);
            file->AssetTypes = PushArray(arena, file->Header.AssetTypeCount, hha_asset_type);
            PlatformAPI.ReadDataFromFile(file->Handle, file->Header.AssetTypeOffset, assetTypeArraySize, &file->AssetTypes);


            if (file->Header.MagicValue == HHA_MAGIC_VALUE)
            {
                PlatformAPI.FileError(file->Handle, "HHA file magic version not found");
            }

            if (file->Header.Version != HHA_VERSION)
            {
                PlatformAPI.FileError(file->Handle, "HHA bad version");
            }

            
            if (PlatformNoFileErrors(file->Handle))
            {
                assets->TagCount += file->Header.TagCount;
                assets->AssetCount += file->Header.AssetCount;

                
            }
            else
            {
                InvalidCodePath;
            }
        }

        PlatformAPI.GetAllFilesOfTypeEnd(fileGroup);
    }

    // allocate all metadata space
    assets->Assets = PushArray(arena, assets->AssetCount, hha_asset);
    assets->Slots = PushArray(arena, assets->AssetCount, asset_slot);
    assets->Tags = PushArray(arena, assets->TagCount, hha_tag);

    // NOTE: load tags
    for (u32 i=0; i < assets->FileCount; i++)
    {
        asset_file *file = assets->Files + i;
        if (PlatformNoFileErrors(file->Handle)) {
            u32 tagArraySize = file->Header.TagCount * sizeof(hha_tag);
            PlatformAPI.ReadDataFromFile(file->Handle, file->Header.TagOffset, tagArraySize, assets->Tags + file->TagBase);
        }
    }

    u32 assetCount = 0;
    u32 tagCount = 0;

    // TODO: do this in a way to scale gracefully
    for (u32 destTypeId = AssetType_None; destTypeId < AssetType_Count; destTypeId++)
    {
        asset_type *destType = assets->AssetTypes + destTypeId;
        destType->FirstAssetIndex = assetCount;
        
        for (u32 i=0; i < assets->FileCount; i++)
        {
            asset_file *file = assets->Files + i;

            file->TagBase = assets->TagCount;

            for (u32 sourceIdx = 0; sourceIdx < file->Header.AssetTypeCount; sourceIdx++)
            {
                hha_asset_type *sourceType = file->AssetTypes + sourceIdx;
                if (sourceType->TypeId == destTypeId) {
                    u32 assetCountForType (sourceType->OnePastLastAssetIndex - sourceType->FirstAssetIndex);
                    u64 firstAssetToReadFrom = file->Header.AssetOffset + sourceType->FirstAssetIndex * sizeof(hha_asset);
                    PlatformAPI.ReadDataFromFile(file->Handle, firstAssetToReadFrom, assetCountForType * sizeof(hha_asset), assets->Assets + assetCount);
                    
                    for (u32 assetIndex = assetCount; assetIndex < assetCount+assetCountForType; assetIndex++)
                    {
                        hha_asset *asset = assets->Assets + assetIndex;
                        asset->FirstTagIndex += file->TagBase;
                        asset->OnePastLastTagIndex += file->TagBase;
                    }
                    assetCount += assetCountForType;
                }
            }
        }
        
        destType->OnePastLastAssetIndex = assetCount;
    }
    

    Assert(assetCount == assets->AssetCount);
    Assert(tagCount == assets->TagCount);

    #endif

    
    #if 1
    debug_read_file_result fileResult = PlatformAPI.DEBUG_ReadEntireFile("test.hha");

    if (fileResult.ContentSize != 0)
    {
        hha_header *assetHeader = (hha_header *)fileResult.Content;

        
        assets->Tags = (hha_tag *)((u8 *)fileResult.Content + assetHeader->TagOffset);
        assets->Assets = (hha_asset *)((u8 *)fileResult.Content + assetHeader->AssetOffset);
        assets->Slots = PushArray(arena, assetHeader->AssetCount, asset_slot);

        assets->TagCount = assetHeader->TagCount;
        assets->AssetCount = assetHeader->AssetCount;

        hha_asset_type *HHAAssetTypes = (hha_asset_type *)((u8 *)fileResult.Content + assetHeader->AssetTypeOffset);
        for (u32 assetTypeIndex=0; assetTypeIndex < AssetType_Count; assetTypeIndex++)
        {
            hha_asset_type *source = HHAAssetTypes + assetTypeIndex;

            if (source->TypeId < assetHeader->AssetTypeCount) {
                // TODO: support merging
                asset_type *dest = assets->AssetTypes + assetTypeIndex;
                dest->FirstAssetIndex = source->FirstAssetIndex;
                dest->OnePastLastAssetIndex = source->OnePastLastAssetIndex;
            }
        }
        assets->HHAContent = (u8 *)fileResult.Content;
    }

    #endif

    assets->TranState = tranState;

    #if 0
    
    BeginAssetType(assets, AssetType_Loaded);
    AddBitmapAsset(assets, "../data/new-bg-code.bmp");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_EnemyDemo);
    AddBitmapAsset(assets, "../data/test2/enemy_demo.bmp", ToV2(0.516949177f, 0.0616113730f));
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FamiliarDemo);
    AddBitmapAsset(assets, "../data/test2/familiar_demo.bmp");
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
        sound_id addedAssetId = AddSoundAsset(assets, "../data/sound/energetic-piano-uptempo-loop_156bpm_F#_minor.wav", sampleIndex, samplesToLoad);
        if (lastAddedSound && addedAssetId.Value) {
            lastAddedSound->Sound.NextIdToPlay = addedAssetId;
        }
        lastAddedSound = &assets->Assets[addedAssetId.Value];
    }
    EndAssetType(assets);

    #endif


    return assets;
}

