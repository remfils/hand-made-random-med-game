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
DEBUGLoadWAV(char *filename, memory_arena *arena)
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
        result.SampleCount = sampleDataSize / (channelCount * sizeof(real32)); // TODO: uint16 is correct?

        if (result.ChannelCount == 1)
        {
            result.Samples[0] = (int16 *)sampleData;
            result.Samples[1] = 0;
        }
        else if (result.ChannelCount == 2)
        {
            result.Samples[0] = PushArray(arena, result.SampleCount, int16);
            result.Samples[1] = PushArray(arena, result.SampleCount, int16);;

            for (uint32 sampleIndex=0;
                 sampleIndex < result.SampleCount;
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
        result.ChannelCount = 1;
        
    }

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

        for (uint32 tagIndex = asset->FirstTagIndex;
             tagIndex < asset->OnePastLastTagIndex;
             ++tagIndex)
        {
            asset_tag *tag = assets->Tags + tagIndex;
            real32 a = matchVector->E[tag->Id];
            real32 b = tag->Value;
            real32 d0 = AbsoluteValue(a - b);
            real32 d1 = AbsoluteValue(a - assets->TagPeriodRange[tag->Id] * SignOf(a) - b);
            real32 diff = Minimum(d0, d1);
            real32 weightedDifference = weightVector->E[tag->Id] * AbsoluteValue(diff);

            totalWeightedDiff += weightedDifference;
        }

        if (bestDiff > totalWeightedDiff)
        {
            bestDiff = totalWeightedDiff;
            result = asset->SlotId;
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
        asset *asset = assets->Assets + type->FirstAssetIndex;
        result = asset->SlotId;
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

    asset_sound_info *info = work->Assets->SoundInfos + work->Id.Value;
    *work->Sound = DEBUGLoadWAV(info->FileName, &work->Assets->Arena);

    CompletePreviousWritesBeforeFutureWrites;
    
    work->Assets->Sounds[work->Id.Value].Sound = work->Sound;
    work->Assets->Sounds[work->Id.Value].State = work->FinalState;

    EndTaskWithMemory(work->Task);
}

internal void
LoadSound(game_assets *assets, sound_id id)
{
    if (id.Value)
    {
        asset_state currentState = (asset_state)AtomicCompareExchange((uint32 *)&assets->Sounds[id.Value].State, AssetState_Unloaded, AssetState_Queued);
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
    
                PlatformAddEntry(assets->TranState->LowPriorityQueue, DoLoadSoundWork, work);
            }
        }
    }
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

internal sound_id
DEBUGAddSoundInfo(game_assets *assets, char *fileName)
{
    sound_id id = {assets->DEBUGUsedSoundCount++};

    asset_sound_info *result = assets->SoundInfos + id.Value;
    result->FileName = fileName;

    return id;
}

internal void
AddSoundAsset(game_assets *assets, char *fileName)
{
    Assert(assets->DEBUGCurrentAssetType);
    Assert(assets->DEBUGCurrentAssetType->OnePastLastAssetIndex < assets->AssetCount);
    
    asset *asset = assets->Assets + assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;
    asset->FirstTagIndex = assets->DEBUGUsedTagCount;
    asset->OnePastLastTagIndex = asset->FirstTagIndex;
    asset->SlotId = DEBUGAddSoundInfo(assets, fileName).Value;

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

    for (uint32 tagType=0;
         tagType < Tag_Count;
         ++tagType)
    {
        // NOTE: init to some large number
        assets->TagPeriodRange[tagType] = 1000000.0f;
    }
    assets->TagPeriodRange[Tag_FacingDirection] = 2.0f * Pi32;
    
    assets->BitmapCount = 256 * AssetType_Count;
    assets->BitmapInfos = PushArray(arena, assets->BitmapCount, asset_bitmap_info);
    assets->Bitmaps = PushArray(arena, assets->BitmapCount, asset_slot);

    assets->SoundCount = 256 * AssetType_Count;
    assets->SoundInfos = PushArray(arena, assets->SoundCount, asset_sound_info);
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
    
    BeginAssetType(assets, AssetType_PianoMusic);
    AddSoundAsset(assets, "../data/sound/energetic-piano-uptempo-loop_156bpm_F#_minor.wav");
    EndAssetType(assets);


    return assets;
}
