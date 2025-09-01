#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"


#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_file_formats.h"
#include "test_asset_builder.h"



FILE *out;

internal void 
BeginAssetType(game_assets *assets, asset_type_id id)
{
    // assets->AssetTypeCount++; // NOTE: assettype has to be static
    assets->DEBUGCurrentAssetType = assets->AssetTypes + id;
    assets->DEBUGCurrentAssetType->TypeId = id;
    assets->DEBUGCurrentAssetType->FirstAssetIndex = assets->DEBUGUsedAssetCount;
    assets->DEBUGCurrentAssetType->OnePastLastAssetIndex = assets->DEBUGCurrentAssetType->FirstAssetIndex;
}

struct added_asset
{
    u32 Id;
    hha_asset *HHA;
};

internal added_asset
AddAsset(game_assets *assets)
{
    Assert(assets->DEBUGCurrentAssetType);

    added_asset result = {};

    result.Id = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex++;

    result.HHA = assets->AssetData + result.Id;
    result.HHA->FirstTagIndex = assets->TagCount;
    result.HHA->OnePastLastTagIndex = result.HHA->FirstTagIndex;

    assets->AssetCount++;
    assets->AssetIndex = result.Id;
    return result;
}

internal bitmap_id
AddBitmapAsset(game_assets *assets, char *fileName, char *name, v2 alignPercentage={0.5f, 0.5f})
{
    added_asset result = AddAsset(assets);
    source_asset *assetSrc = assets->AssetSource + result.Id;
    assetSrc->Bitmap.FileName = fileName;
    assetSrc->AssetFileType = AssetFileType_Bitmap;

    u32 maxDebugNameLen = 20;
    for (u32 idx=0; idx < maxDebugNameLen && *name; idx++) {
        result.HHA->DebugName[idx] = *name++;
    }
    
    result.HHA->Bitmap.AlignPercentage = alignPercentage;
    return {result.Id};
}

internal bitmap_id
AddLetterAsset(game_assets *assets, loaded_font *font, u32 codePoint)
{
    added_asset result = AddAsset(assets);
    source_asset *assetSrc = assets->AssetSource + result.Id;
    assetSrc->FontGlyph.Font = font;
    assetSrc->FontGlyph.CodePoint = codePoint;
    assetSrc->AssetFileType = AssetFileType_FontGlyph;

    return {result.Id};
}

internal font_id
AddFontAsset(game_assets *assets, loaded_font *font)
{
    added_asset result = AddAsset(assets);

    source_asset *assetSrc = assets->AssetSource + result.Id;
    assetSrc->Font.Font = font;
    assetSrc->AssetFileType = AssetFileType_Font;

    return {result.Id};
}

internal sound_id
AddSoundAsset(game_assets *assets, char *fileName, char *name, uint32 firstSampleIndex=0, uint32 sampleCount=0)
{
    added_asset result = AddAsset(assets);
    
    source_asset *assetSrc = assets->AssetSource + result.Id;
    assetSrc->AssetFileType = AssetFileType_Sound;
    assetSrc->Sound.FileName = fileName;
    assetSrc->Sound.FirstSampleIndex = firstSampleIndex;
    
    result.HHA->FirstTagIndex = assets->TagCount;
    u32 maxDebugNameLen = 20;
    for (u32 idx=0; idx < maxDebugNameLen && *name; idx++) {
        result.HHA->DebugName[idx] = *name++;
    }
    result.HHA->OnePastLastTagIndex = result.HHA->FirstTagIndex;
    result.HHA->Sound.Chain = HHASoundChain_None;
    result.HHA->Sound.SampleCount = sampleCount;

    return {result.Id};
}


internal void
AddAssetTag(game_assets *assets, asset_tag_id tagId, real32 tagValue)
{
    hha_asset *asset = assets->AssetData + assets->AssetIndex;
    
    asset->OnePastLastTagIndex++;
    hha_tag *tag = assets->Tags + assets->TagCount++;
    tag->Id = tagId;
    tag->Value = tagValue;
}

internal void
EndAssetType(game_assets *assets)
{
    assets->DEBUGUsedAssetCount = assets->DEBUGCurrentAssetType->OnePastLastAssetIndex;
    assets->DEBUGCurrentAssetType = 0;
    assets->AssetIndex = 0;
}


struct entire_file
{
    u32 Size;
    void *Content;
};

entire_file
ReadEntireFile(char *fileName)
{
    entire_file result = {};


    FILE *in = fopen(fileName, "rb");

    if (in) {

        fseek(in, 0, SEEK_END);
        result.Size = ftell(in);
        result.Content = malloc(result.Size);

        fseek(in, 0, SEEK_SET);
        fread(result.Content, result.Size, 1, in);
        fclose(in);
    } else {
        printf("ERROR: cannot open file %s", fileName);
    }
    
    return result;
}


internal loaded_bitmap
LoadBMP(char *filename)
{
    // NOTE: bitmap order is AABBGGRR, bottom row is first
    
    entire_file readResult = ReadEntireFile(filename);
    
    loaded_bitmap result = {};

    if (readResult.Size != 0)
    {
        result.Free = readResult.Content;
        void *content = readResult.Content;
        
        bitmap_header *header = (bitmap_header *) content;
        
        result.Width = header->Width;
        result.Height = header->Height;

        uint8 *pixels = (u8 *)((u8 *)content + header->BitmapOffset);
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

    result.Pitch = result.Width * 4;

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
LoadWAV(char *filename, uint32 sectionFirstSampleIndex, uint32 sectionSampleCount)
{
    entire_file readResult = ReadEntireFile(filename);
    
    loaded_sound result = {};

    if (readResult.Size != 0)
    {
        result.Free = readResult.Content;
        
        WAVE_header *header = (WAVE_header *)readResult.Content;
        Assert(header->ChunkId == WAVE_ChunkId_RIFF);
        Assert(header->Format == WAVE_ChunkId_WAVE);

        u32 channelCount = 0;
        u32 sampleDataSize = 0;
        r32 *sampleData = 0;
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
                sampleData = (r32*)GetChunkData(iter);
                sampleDataSize = GetChunkSize(iter);
            } break;
            case WAVE_ChunkId_fact: {
                uint32 *numberOfSamplesInChannel = (uint32 *)GetChunkData(iter);
            } break;
            }
        }

        #if 1
        Assert(channelCount && sampleData);

        result.ChannelCount = channelCount;
        u32 sampleCount = sampleDataSize / (channelCount * sizeof(r32));

        // TODO: more channels
        if (result.ChannelCount == 1)
        {
            result.Samples[0] = (s16*)sampleData;
            result.Samples[1] = 0;
        }
        else if (result.ChannelCount == 2)
        {
            result.Samples[0] = (s16 *)sampleData;
            result.Samples[1] = (s16 *)sampleData + sampleCount;

            for (uint32 sampleIndex=0;
                 sampleIndex < sampleCount;
                 sampleIndex++)
            {
                // TODO: wtf?
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

internal loaded_font*
LoadFont(char *fileName, u32 codePointCount)
{
    loaded_font *font = (loaded_font *)malloc(sizeof(loaded_font));
    entire_file fileResult = ReadEntireFile(fileName);
    s32 lineGap=0;
    
    stbtt_InitFont(&font->Info, (u8 *)fileResult.Content, stbtt_GetFontOffsetForIndex((u8 *)fileResult.Content, 0));
    stbtt_GetFontVMetrics(&font->Info, 0, 0, &lineGap);

    font->Size = 78.0f;
    font->Factor = stbtt_ScaleForPixelHeight(&font->Info, font->Size);
    font->Free = fileResult.Content;

    font->CodePointCount = codePointCount;
    font->LineAdvance = (r32)lineGap * font->Factor;

    font->BitmapIds = (bitmap_id *)malloc(sizeof(bitmap_id) * codePointCount);
    font->HorizontalAdvance = (r32 *)malloc(sizeof(r32) * codePointCount * codePointCount);

    return font;
}

internal void
FreeFont(loaded_font *font)
{
    free(font->Free);
}

internal loaded_bitmap
LoadGlyph(loaded_font *font, u32 codePoint, hha_asset *assetDst)
{
    int width=0,height=0,advance=0,lsb=0,ascent=0,descent=0,lineGap=0,xoff=0,yoff=0;

    // TODO: add 1px padding for texel premultiplied aplha
    stbtt_GetCodepointHMetrics(&font->Info, codePoint, &advance, &lsb);
    u8 *bitmap = stbtt_GetCodepointBitmap(&font->Info, 0, stbtt_ScaleForPixelHeight(&font->Info, font->Size), codePoint, &width, &height, &xoff, &yoff);

    loaded_bitmap result = {};
    result.Width = width;
    result.Height = height;
    result.Pitch = result.Width * 4;

    result.Memory = malloc(result.Height * result.Pitch);
    result.Free = result.Memory;

    assetDst->Bitmap.AlignPercentage.x = 0.0f;
    assetDst->Bitmap.AlignPercentage.y = ((float) height + (float)yoff ) /  (float)height;

    u8 *source = bitmap;
    u8 *destRow = (u8 *)result.Memory + (height - 1) * result.Pitch;
    for (s32 y=0; y < height; y++)
    {
        u32 *dest = (u32 *)destRow;
        for (s32 x=0; x < width; x++)
        {
            u8 alpha = *source++;

            #if 1

            // NOTE: this removes black fringes on text - premultiply
            // alpha
            v4 texel = {255.0f, 255.0f, 255.0f, (float)alpha};
            texel = SRGB255_ToLinear1(texel);
            texel.rgb *= texel.a;
            texel = Linear_1_ToSRGB255(texel);
            
            *dest++ = (
                        ((u32)(texel.a + 0.5f) << 24)
                        | ((u32)(texel.r + 0.5f) << 16)
                        | ((u32)(texel.g + 0.5f) << 8)
                        | ((u32)(texel.b + 0.5f) << 0)
                        );
            #else
            *dest++ = (
                (alpha << 24)
                | (alpha << 16)
                | (alpha << 8)
                | (alpha << 0));

            #endif
        }
        destRow -= result.Pitch;
    }

    stbtt_FreeBitmap(bitmap, 0);

    for (u32 otherCodePointIndex =0;
         otherCodePointIndex < font->CodePointCount;
         otherCodePointIndex++)
    {
        font->HorizontalAdvance[codePoint * font->CodePointCount + otherCodePointIndex] = (r32)result.Width;
    }

    return result;
}

void
WriteAssetsFile(game_assets *assets, char *filename)
{
    out = fopen(filename, "wb");

    if (out)
    {
        hha_header header = {};
        header.MagicValue = HHA_MAGIC_VALUE;
        header.Version = HHA_VERSION;
        header.TagCount = assets->TagCount;
        header.AssetCount = assets->AssetCount;
        header.AssetTypeCount = AssetType_Count;

        header.TagOffset = sizeof(header);
        header.AssetTypeOffset = header.TagOffset + header.TagCount * sizeof(hha_tag);
        header.AssetOffset = header.AssetTypeOffset + header.AssetTypeCount * sizeof(hha_asset_type);

 
        u32 tagArraySize = header.TagCount * sizeof(hha_tag);
        u32 assetTypeArraySize = header.AssetTypeCount * sizeof(hha_asset_type);
        u32 assetArraySize = header.AssetCount * sizeof(hha_asset);

        fwrite(&header, sizeof(hha_header), 1, out);
        fwrite(assets->Tags, tagArraySize, 1, out);
        fwrite(assets->AssetTypes, assetTypeArraySize, 1, out);

        // TODO: write asset definition array
        // 
        fseek(out, assetArraySize, SEEK_CUR);
        for (u32 assetIndex=1; // NOTE: start from 1 because 0 is special asset
             assetIndex < header.AssetCount;
             ++assetIndex)
        {
            
            source_asset *assetSrc = assets->AssetSource + assetIndex;
            hha_asset *assetDst = assets->AssetData + assetIndex;

            assetDst->DataOffset = (u64)ftell(out);

            if (assetSrc->AssetFileType == AssetFileType_Sound)
            {
                loaded_sound snd = LoadWAV(assetSrc->Sound.FileName, assetSrc->Sound.FirstSampleIndex, assetDst->Sound.SampleCount);

                assetDst->Sound.SampleCount = snd.SampleCount;
                assetDst->Sound.ChannelCount = snd.ChannelCount;

                for (u32 channelIndex=0;
                     channelIndex < assetDst->Sound.ChannelCount;
                     channelIndex++)
                {
                    fwrite(snd.Samples[channelIndex], assetDst->Sound.SampleCount * sizeof(s16), 1, out);
                }

                free(snd.Free);
            }
            else if (assetSrc->AssetFileType == AssetFileType_Font)
            {
                loaded_font *font = assetSrc->Font.Font;
                u32 codePointSize = sizeof(bitmap_id) * font->CodePointCount;
                u32 horizontalAdvanceTableSize = sizeof(r32) * font->CodePointCount * font->CodePointCount;

                fwrite(font->BitmapIds, codePointSize, 1, out);
                fwrite(font->HorizontalAdvance, horizontalAdvanceTableSize, 1, out);

                //free(font->Free);
            }
            else 
            {
                loaded_bitmap bmp;

                if (assetSrc->AssetFileType == AssetFileType_Bitmap)
                {
                    bmp = LoadBMP(assetSrc->Bitmap.FileName);
                }
                else
                {
                    bmp = LoadGlyph(assetSrc->FontGlyph.Font, assetSrc->FontGlyph.CodePoint, assetDst);
                }

                assetDst->Bitmap.Dim[0] = bmp.Width;
                assetDst->Bitmap.Dim[1] = bmp.Height;

                Assert((bmp.Width * 4) == bmp.Pitch);
                fwrite(bmp.Memory, bmp.Width * bmp.Height * 4, 1, out);
                
                free(bmp.Free);
            }
        }
        fseek(out, (u32)header.AssetOffset, SEEK_SET);
        fwrite(assets->AssetData, assetArraySize, 1, out);
        
        fclose(out);
    }
    else
    {
        printf("ERROR: can't open file");
    } 
}

void InitAssets(game_assets *assets)
{
    assets->TagCount = 1;
    assets->AssetCount = 1;
    assets->DEBUGUsedAssetCount = 1;
    assets->DEBUGUsedTagCount = 0;
    assets->DEBUGCurrentAssetType = 0;
    assets->AssetIndex = 0;

    assets->AssetTypeCount = AssetType_Count;
    memset(assets->AssetTypes, 0, sizeof(assets->AssetTypes));
}

void
WriteHeroFiles()
{
    game_assets assetsV = {};
    game_assets *assets = &assetsV;
    InitAssets(assets);

    BeginAssetType(assets, AssetType_HumanBody);
    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_right.bmp", "char_w", {0.508474588f, 0.07109005f});
    AddAssetTag(assets, Tag_FacingDirection, 0.0f);

    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_up.bmp", "char_n", {0.508474588f, 0.118483409f});
    AddAssetTag(assets, Tag_FacingDirection, 0.5f * Pi32);

    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_left.bmp", "char_e", {0.508474588f, 0.07109005f});
    AddAssetTag(assets, Tag_FacingDirection, Pi32);

    AddBitmapAsset(assets, "../data/old_random_med_stuff/stand_down.bmp", "char_s", {0.508474588f, 0.118483409f});
    AddAssetTag(assets, Tag_FacingDirection, -0.5f * Pi32);
    EndAssetType(assets);

    WriteAssetsFile(assets, "test1.hha");
}

void WriteNonHeroFiles()
{
    game_assets assetsV = {};
    game_assets *assets = &assetsV;
    InitAssets(assets);

    BeginAssetType(assets, AssetType_Loaded);
    AddBitmapAsset(assets, "../data/new-bg-code.bmp", "bg");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_EnemyDemo);
    AddBitmapAsset(assets, "../data/test2/enemy_demo.bmp", "enemy_demo", {0.516949177f, 0.0616113730f});
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FamiliarDemo);
    AddBitmapAsset(assets, "../data/test2/familiar_demo.bmp", "fam_demo");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_WallDemo);
    AddBitmapAsset(assets, "../data/test2/wall_demo.bmp", "wall", {0.5f, 0.484375f});
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_SwordDemo);
    AddBitmapAsset(assets, "../data/test2/sword_demo.bmp", "sword", {0.479999989f, 0.0317460336f});
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_Grass);
    AddBitmapAsset(assets, "../data/grass001.bmp", "grass_001");
    AddBitmapAsset(assets, "../data/grass002.bmp", "grass_002");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_Ground);
    AddBitmapAsset(assets, "../data/ground001.bmp", "ground_001");
    AddBitmapAsset(assets, "../data/ground002.bmp", "ground_002");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_ParticleStar);
    AddBitmapAsset(assets, "../data/particle_star__001.bmp", "star_001");
    AddBitmapAsset(assets, "../data/particle_star__002.bmp", "star_002");
    AddBitmapAsset(assets, "../data/particle_star__003.bmp", "star_003");
    EndAssetType(assets);

    u32 codePointCount = '~' + 1;

    loaded_font *debugFont = LoadFont("D:/Projects/local__HandmadeHero/data/iosevka-regular.ttf", codePointCount);
    BeginAssetType(assets, AssetType_Font);
    AddFontAsset(assets, debugFont);
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FontGlyph);
    for (u32 letter = 'a'; letter <= '~'; letter++)
    {
        debugFont->BitmapIds[letter] = AddLetterAsset(assets, debugFont, letter);
    }

    EndAssetType(assets);
    WriteAssetsFile(assets, "test2.hha");
}

void WriteSoundFiles()
{
    game_assets assetsV = {};
    game_assets *assets = &assetsV;
    InitAssets(assets);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // sounds
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    BeginAssetType(assets, AssetType_FootstepGravel);
    AddSoundAsset(assets, "../data/sound/footsteps__gravel.wav", "foot_grav");
    EndAssetType(assets);

    BeginAssetType(assets, AssetType_FootstepMud);
    AddSoundAsset(assets, "../data/sound/footsteps__mud02.wav", "foot_mud");
    EndAssetType(assets);
    
    BeginAssetType(assets, AssetType_FootstepStone);
    AddSoundAsset(assets, "../data/sound/footsteps__stone01.wav", "foot_stone");
    EndAssetType(assets);
    
    BeginAssetType(assets, AssetType_FootstepWood);
    AddSoundAsset(assets, "../data/sound/footsteps__wood01.wav", "foot_wood");
    EndAssetType(assets);


    uint32 totalMusicFileSamples = 615916;
    uint32 musicChunk = 40000;
    sound_id lastAddedSound = {};
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
        
        lastAddedSound = AddSoundAsset(assets, "../data/sound/energetic-piano-uptempo-loop_156bpm_F#_minor.wav", "piano", sampleIndex, samplesToLoad);

        if (sampleIndex + musicChunk < totalMusicFileSamples) {
            assets->AssetData[lastAddedSound.Value].Sound.Chain = HHASoundChain_Advance;
        }
    }
    EndAssetType(assets);

    WriteAssetsFile(assets, "test3.hha");
}

int
main(int argCount, char **args)
{
    WriteHeroFiles();
    WriteNonHeroFiles();
    WriteSoundFiles();
}
