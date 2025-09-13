#include <stdio.h>

DEBUG_DECLARE_RECORD_ARRAY_(0);
DEBUG_DECLARE_RECORD_ARRAY_(1);
DEBUG_DECLARE_RECORD_ARRAY_(2);


global_variable render_group *DEBUGRenderGroup;
global_variable r32 DEBUG_LeftEdge = 0.0f;
global_variable r32 DEBUG_LineY = 0.0f;
global_variable r32 DEBUG_FontScale = 0.0f;

global_variable debug_table GlobalDebugTable_;
debug_table *GlobalDebugTable = &GlobalDebugTable_;
const u32 DebugRecordsCount_2 = 0;

internal void
DEBUGReset(u32 width, u32 height)
{
    MakeOrthographic(DEBUGRenderGroup, width, height, 1.0f);
    asset_vector mV = {};
    asset_vector weight = {};
    weight.E[Tag_UnicodePoint] = 1.0f;

    font_id fontId = BestMatchFont(DEBUGRenderGroup->Assets, &mV, &weight);

    hha_font *fontInfo = GetFontInfo(DEBUGRenderGroup->Assets, fontId);
    
    DEBUG_FontScale = 1.0f;
    DEBUG_LineY = 0.5f * (r32)height - fontInfo->Ascend * DEBUG_FontScale + 1.0f;
    DEBUG_LeftEdge = -0.5f * (r32)width;
}

inline b32
IsHex(char c)
{
    b32 result = (c >= '0' && c <= '9')
        || (c >= 'A' && c <= 'F');
    return result;
}

inline u32
GetHex(char c)
{
    u32 result = 0;
    if (c >= '0' && c <= '9')
    {
        result = c - '0';
    }

    if (c >= 'A' && c <= 'F')
    {
        result = 0xA + (c - 'A');
    }
    return result;
}

internal void
DebugTextLine(char *string)
{
    if (DEBUGRenderGroup) {

        // TODO: move to static variables
        asset_vector mV = {};
        asset_vector weight = {};
        mV.E[Tag_FontType] = (r32)FontType_Debug;
        weight.E[Tag_FontType] = 1.0f;
        font_id fontId = BestMatchFont(DEBUGRenderGroup->Assets, &mV, &weight);
        loaded_font *font = GetFont(DEBUGRenderGroup->Assets, fontId, DEBUGRenderGroup->GenerationId);

        if (!font)
        {
            LoadFont(DEBUGRenderGroup->Assets, fontId, true);
            font = GetFont(DEBUGRenderGroup->Assets, fontId, DEBUGRenderGroup->GenerationId);
        }

        if (font)
        {
            hha_font *fontInfo = GetFontInfo(DEBUGRenderGroup->Assets, fontId);
            r32 atX = DEBUG_LeftEdge;
            r32 scale = DEBUG_FontScale;

            u32 prevCodePoint = 0;

            for (char *at = string;
                 *at;
                 ++at)
            {
                r32 charDim = scale;
                u32 codePoint = *at;

                if (
                    at[0] == '\\'
                    && IsHex(at[1])
                    && IsHex(at[2])
                    && IsHex(at[3])
                    && IsHex(at[4])
                    ) {
                    codePoint = (GetHex(at[1]) << 12)
                        | (GetHex(at[2]) << 8)
                        | (GetHex(at[3]) << 4)
                        | (GetHex(at[4]) << 0)
                        ;
                    at += 4;
                }

                r32 advanceX = scale*GetHorizontalAdvanceForPair(fontInfo, font, prevCodePoint, codePoint);
                atX += advanceX;
                //atX += scale;
                if (codePoint != ' ') {
                    bitmap_id bitmapId = GetBitmapForGlyph(DEBUGRenderGroup->Assets, fontInfo, font, codePoint);
                
                    hha_bitmap *info = GetBitmapInfo(DEBUGRenderGroup->Assets, bitmapId);

                    PushBitmap(DEBUGRenderGroup, bitmapId, scale*(r32)info->Dim[1], ToV3(atX,DEBUG_LineY,0), ToV4(1,1,1,1));
                }

                prevCodePoint = *at;
            }
            DEBUG_LineY -= GetVerticalLineAdvance(fontInfo) * scale;
        }
    }
}

// TODO: remove this
internal void
UpdateCycleCounterArray(debug_state *debugState, debug_record *records, u32 recordsCount)
{
    for (u32 debugIndex=0;
         debugIndex < recordsCount;
         debugIndex++)
    {
        debug_record *src = records + debugIndex;
        debug_counter_state *dst = debugState->CounterStates + debugState->CounterCount++;

        dst->FileName = src->FileName;
        dst->BlockName = src->BlockName;
        dst->Line = src->Line;

        u64 value = AtomicExchange64(&src->Clocks_and_HitCount, 0);
        dst->DataSnapshots[debugState->SnapshotIndex].HitCount = (u32)(value >> 32);
        dst->DataSnapshots[debugState->SnapshotIndex].CycleCount = (u32)(value & 0xFFFFFFFF);
    }
}

internal void
CollectDebugRecords(debug_state *debugState, u32 eventCount, debug_event *debugEventArray)
{
    u32 totalCounterCount = 0;
    debug_counter_state *counters[MAX_UNIT_COUNT];
    debug_counter_state *currentCounterState = debugState->CounterStates;

    for (u32 unitIndex=0;
         unitIndex < MAX_UNIT_COUNT;
         unitIndex++)
    {
        counters[unitIndex] = currentCounterState;

        totalCounterCount += GlobalDebugTable->RecordCount[unitIndex];
        currentCounterState += GlobalDebugTable->RecordCount[unitIndex];
    }

    
    for (u32 debugIndex=0;
         debugIndex < totalCounterCount;
         debugIndex++)
    {
        debug_counter_state *dst = debugState->CounterStates + debugIndex;
        dst->DataSnapshots[debugState->SnapshotIndex].HitCount = 0;
        dst->DataSnapshots[debugState->SnapshotIndex].CycleCount = 0;
        debugState->CounterCount++;
    }

    for (u32 eventIndex=0;
         eventIndex < eventCount;
         eventIndex++)
    {
        debug_event *event = debugEventArray + eventIndex;
        debug_counter_state *dst = counters[event->UnitIndex] + event->DebugRecordIndex;
        debug_record *src = GlobalDebugTable->Records[event->UnitIndex] + event->DebugRecordIndex;

        if (event->Type == DebugEvent_BeginBlock)
        {
            Assert(src->BlockName);
            
            dst->FileName = src->FileName;
            dst->BlockName = src->BlockName;
            dst->Line = src->Line;

            dst->DataSnapshots[debugState->SnapshotIndex].HitCount++;
            dst->DataSnapshots[debugState->SnapshotIndex].CycleCount -= event->Clock;
        }
        else if (event->Type == DebugEvent_EndBlock)
        {
            Assert(src->BlockName);
            dst->DataSnapshots[debugState->SnapshotIndex].CycleCount += event->Clock;
        }
        
    }
}

struct debug_stat_record
{
    u32 Count;
    r64 Min;
    r64 Max;
    r64 Avg;
};

inline void
BeginStatRecord(debug_stat_record *record)
{
    record->Count = 0;
    record->Avg = 0;
    record->Min = Real32Maximum;
    record->Max = -Real32Maximum;
}

inline void
UpdateStatRecord(debug_stat_record *record, r64 value)
{
    ++record->Count;
    record->Avg += value;
    if (record->Min > value)
    {
        record->Min = value;
    }
    
    if (record->Max < value)
    {
        record->Max = value;
    }
}

inline void
EndStatRecord(debug_stat_record *record)
{
    if (record->Count == 0) {
        record->Min = 0;
        record->Max = 0;
    } else {
        record->Avg /= record->Count;
    }
}

internal void
OverlayDebugCycleCounters(game_memory *memory)
{
    debug_state *debugState = (debug_state *)memory->DebugStorage;

    if (!DEBUGRenderGroup) return;

    if (debugState)
    {
        DebugTextLine("DEBUG CYCLES: \\0414\\0410\\0424\\0444\\0424!");
        for (u32 debugIndex=0;
             debugIndex < debugState->CounterCount;
             debugIndex++)
        {
            debug_counter_state *counterState = debugState->CounterStates + debugIndex;

            debug_stat_record hitCount, cycleCount;
            BeginStatRecord(&hitCount);
            BeginStatRecord(&cycleCount);

            for (u32 snapIndex=0; snapIndex < DEBUG_MAX_SNAPSHOT_COUNT; snapIndex++)
            {
                UpdateStatRecord(&hitCount, (r32)counterState->DataSnapshots[snapIndex].HitCount);
                UpdateStatRecord(&cycleCount, (r32)counterState->DataSnapshots[snapIndex].CycleCount);
            }
            
            EndStatRecord(&hitCount);
            EndStatRecord(&cycleCount);

            if (cycleCount.Max > 0.0f)
            {
                r32 chartLeft = 0.0f;
                r32 chartHeight = 20.0f;
                r32 scale = 1.0f / (r32)cycleCount.Max;
                for (u32 snapIndex=0; snapIndex < DEBUG_MAX_SNAPSHOT_COUNT; snapIndex++)
                {
                    r32 barMinY = DEBUG_LineY;
                    r32 valueNormalized = scale * (r32)counterState->DataSnapshots[snapIndex].CycleCount;
                    r32 barHeight = valueNormalized * chartHeight;

                    PushPieceRect(DEBUGRenderGroup, ToV3(chartLeft + (r32)snapIndex, barMinY + 0.5f * barHeight, 0), ToV2(1.0f, barHeight), ToV4(valueNormalized, 1, 0, 1));
                }

                char buffer[256];
                _snprintf_s(buffer, 256, "%s: %u hits: %u", counterState->BlockName, (u32)cycleCount.Avg, (u32)hitCount.Max);
                DebugTextLine(buffer);
            }
        }

        r32 chartHeight = 100.0f;
        r32 chartMinY = DEBUG_LineY - (chartHeight + 10.0f);
        r32 chartLeft = DEBUG_LeftEdge;
        r32 scale = 1.0f / (1.0f / 60.0f);
        r32 barWidth = 8.0f;
        r32 barSpacing = 5.0f;
        r32 chartWidth = 0;

        v4 colors[] = {
            ToV4(1, 0, 0, 1),
            ToV4(0, 1, 0, 1),
            ToV4(0, 0, 1, 1),
            ToV4(1, 1, 0, 1),
            ToV4(0, 1, 1, 1),
            ToV4(1, 0, 1, 1),
            ToV4(0.5f, 1, 0, 1),
            ToV4(0, 0.5f, 1, 1),
            ToV4(1, 0, 0.5f, 1),
        };

        #if 0

        for (u32 snapIndex=0;
             snapIndex < DEBUG_MAX_SNAPSHOT_COUNT;
             snapIndex++)
        {
            debug_frame_info *info = debugState->FrameInfos + snapIndex;
            r32 prevTimestamp = 0.0f;
            r32 stackY = chartMinY;
            for (u32 timestampIndex=0;
                 timestampIndex < info->TimestampCount;
                 timestampIndex++)
            {
                debug_frame_timestamp *stamp = info->Timestamps + timestampIndex;
                r32 secondsElapsed = stamp->Time - prevTimestamp;
                
                r32 valueNormalized = secondsElapsed * scale;
                r32 barHeight = valueNormalized * chartHeight;
                v4 color = colors[timestampIndex % ArrayCount(colors)];
                PushPieceRect(DEBUGRenderGroup, ToV3(chartLeft + (barWidth + barSpacing) * (r32)snapIndex,  stackY + 0.5f * barHeight, 0), ToV2(barWidth, barHeight), color);

                chartWidth += barWidth + barSpacing;

                stackY += barHeight;
                prevTimestamp = stamp->Time;
            }
        }

        #endif

        PushPieceRect(DEBUGRenderGroup, ToV3(chartLeft + 0.5f * chartWidth, chartMinY + 2.0f * chartHeight, 0), ToV2(chartWidth, 1.0f), ToV4(1, 1, 1, 1));
    }
}