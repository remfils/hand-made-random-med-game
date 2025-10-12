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

inline debug_thread*
GetDebugThread(debug_state *debugState, u32 threadId)
{
    debug_thread *result = 0;
    for (debug_thread *thread = debugState->FirstThread;
         thread;
        thread = thread->Next)
    {
        if (thread->ThreadId == threadId) {
            result = thread;
            break;
        }
    }

    if (!result) {
        result = PushStruct(&debugState->CollateArena, debug_thread);
        result->ThreadId = (u16)threadId;
        result->FirstOpenBlock = 0;
        result->LaneIndex = debugState->FrameBarLaneCount++;
        result->Next = debugState->FirstThread;
        debugState->FirstThread = result;
    }
    
    return result;
}

inline debug_frame_region*
AddRegion(debug_state *debugState, debug_frame *currentFrame)
{
    Assert(currentFrame->RegionCount < MAX_DEBUG_REGIONS_PER_FRAME);
    debug_frame_region *result = currentFrame->Regions + currentFrame->RegionCount++;
    result->MinValue = 0;
    result->MaxValue = 0;
    result->LaneIndex = 0;

    return result;
}

internal void
CollectDebugRecords(debug_state *debugState, u64 invalidArrayIndex)
{
    debugState->Frames = PushArray(&debugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT * 4, debug_frame);
    
    debugState->FrameBarLaneCount = 0;
    debugState->FrameCount = 0;
    debugState->MaxValue = 1.0f; // TODO:

    u64 eventArrayIndex = invalidArrayIndex + 1;
    debug_frame *currentFrame = 0;
    for (;;)
    {
        if (eventArrayIndex >= MAX_DEBUG_EVENT_ARRAY_COUNT) {
            eventArrayIndex = 0;
        }
        if (eventArrayIndex == invalidArrayIndex) {
            break;
        }

        for (u32 eventIndex=0;
             eventIndex < GlobalDebugTable->EventCount[eventArrayIndex];
            eventIndex++)
        {
            debug_event *event = GlobalDebugTable->Events[eventArrayIndex] + eventIndex;
            debug_record *src = GlobalDebugTable->Records[event->UnitIndex] + event->DebugRecordIndex;

            if (event->Type == DebugEvent_FrameMarker)
            {
                if (currentFrame) {
                    currentFrame->EndClock = event->Clock;
                    currentFrame->WallSecondsElapsed = event->WallClock;
                }
                
                currentFrame = debugState->Frames + debugState->FrameCount++;
                currentFrame->BeginClock = event->Clock;
                currentFrame->EndClock = 0;
                currentFrame->RegionCount = 0;
                currentFrame->WallSecondsElapsed = 0.0f;
                currentFrame->Regions = PushArray(&debugState->CollateArena, MAX_DEBUG_REGIONS_PER_FRAME, debug_frame_region);
            }
            else if (currentFrame)
            {
                u32 eventFrameIndex = debugState->FrameCount - 1;
                u64 relativeClock = event->Clock - currentFrame->BeginClock;
                debug_thread *thread = GetDebugThread(debugState, event->TC.ThreadId);

                if (event->Type == DebugEvent_BeginBlock)
                {
                    open_debug_block *block = debugState->FirstFreeBlock;
                    if (block) {
                        debugState->FirstFreeBlock = block->NextFree;
                    } else {
                        block = PushStruct(&debugState->CollateArena, open_debug_block);
                    }

                    block->StartingFrameIndex = eventFrameIndex;
                    block->OpeningEvent = event;
                    block->Parent = thread->FirstOpenBlock;
                    block->NextFree = 0;
                    thread->FirstOpenBlock = block;
                    
                }
                else if (event->Type == DebugEvent_EndBlock)
                {
                    if (thread->FirstOpenBlock)
                    {
                        if (thread->FirstOpenBlock->OpeningEvent)
                        {
                            open_debug_block *matchingBlock = thread->FirstOpenBlock;
                            debug_event *openEvent = matchingBlock->OpeningEvent;
                            if (openEvent->TC.ThreadId == event->TC.ThreadId
                                && openEvent->DebugRecordIndex == event->DebugRecordIndex
                                && openEvent->UnitIndex == event->UnitIndex)
                            {
                                if (matchingBlock->StartingFrameIndex == eventFrameIndex)
                                {
                                    if (matchingBlock->Parent == 0) {
                                        debug_frame_region *region = AddRegion(debugState, currentFrame);
                                        region->LaneIndex = thread->LaneIndex;
                                        region->MinValue = (r32)(openEvent->Clock - currentFrame->BeginClock);
                                        region->MaxValue = (r32)(event->Clock - currentFrame->BeginClock);

                                        if (region->MaxValue > debugState->MaxValue) {
                                            debugState->MaxValue = region->MaxValue;
                                        }
                                    }
                                }
                                else
                                {
                                    // TODO: function spans multiple frames
                                }
                            }
                            else
                            {
                                // TODO: 
                            }

                            matchingBlock->NextFree = debugState->FirstFreeBlock;
                            debugState->FirstFreeBlock = thread->FirstOpenBlock;
                            thread->FirstOpenBlock = matchingBlock->Parent;
                        }
                    }
                }
            }
        }
        
        eventArrayIndex++;
    }

    
    #if 0
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

    #endif
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
        // DebugTextLine("DEBUG CYCLES: \\0414\\0410\\0424\\0444\\0424!");

        if (debugState->FrameCount) {
            char buffer[256];
            _snprintf_s(buffer, 256, "last frame: %.02fms", debugState->Frames[0].WallSecondsElapsed * 1000.0f);
            DebugTextLine(buffer);
        }

        #if 0
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

        #endif

        r32 laneWidth = 8.0f;
        u32 laneCount = debugState->FrameBarLaneCount;
        r32 barWidth = laneWidth * laneCount;
        r32 barSpacing = barWidth + 0.5f;
        r32 chartHeight = 200.0f;
        r32 chartMinY = DEBUG_LineY - (chartHeight + 10.0f);
        r32 chartLeft = DEBUG_LeftEdge;
        r32 scale = 1.0f;
        if (debugState->MaxValue > 0) {
            scale = chartHeight / debugState->MaxValue;
        }
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

        u32 maxFrameCount = 10;
        if (debugState->FrameCount < maxFrameCount) {
            maxFrameCount = debugState->FrameCount;
        }

        for (u32 frameIndex=0;
             frameIndex < maxFrameCount;
             frameIndex++)
        {
            debug_frame *frame = debugState->Frames + frameIndex;

            r32 stackX = chartLeft + barSpacing * (r32) frameIndex;
            
            for (u32 regionIndex=0;
                 regionIndex < frame->RegionCount;
                 regionIndex++)
            {
                debug_frame_region *region = frame->Regions + regionIndex;

                v4 color = colors[regionIndex % ArrayCount(colors)];
                r32 thisMinY = chartMinY + scale * region->MinValue;
                r32 thisMaxY = chartMinY + scale * region->MaxValue;
            
                PushPieceRect(
                    DEBUGRenderGroup,
                    ToV3(stackX + laneWidth * region->LaneIndex + 0.5f * laneWidth,  0.5f * (thisMinY + thisMaxY), 0), // original
                    ToV2(laneWidth, thisMaxY - thisMinY), color
                    );

                chartWidth += barWidth + barSpacing;
            }
        }

        PushPieceRect(DEBUGRenderGroup, ToV3(chartLeft + 0.5f * chartWidth, chartMinY + 2.0f * chartHeight, 0), ToV2(chartWidth, 1.0f), ToV4(1, 1, 1, 1));
    }
}