#include <stdio.h>

DEBUG_DECLARE_RECORD_ARRAY_(0);
DEBUG_DECLARE_RECORD_ARRAY_(1);
DEBUG_DECLARE_RECORD_ARRAY_(2);


//global_variable render_group *DEBUGRenderGroup;
// global_variable r32 DEBUG_LeftEdge = 0.0f;
// global_variable r32 DEBUG_LineY = 0.0f;
// global_variable r32 DEBUG_FontScale = 0.0f;
// global_variable r32 GlobalWidth = 0.0f;
// global_variable r32 GlobalHeight = 0.0f;

global_variable debug_table GlobalDebugTable_;
debug_table *GlobalDebugTable = &GlobalDebugTable_;
const u32 DebugRecordsCount_2 = 0;

internal void RestartCollector(u64 invalidArrayIndex);

inline debug_state *
DEBUGGetState(game_memory *memory)
{
    debug_state *debugState = (debug_state *)memory->DebugStorage;
    Assert(debugState->Initialized);
    
    return debugState;
}

inline debug_state *
DEBUGGetState(void)
{
    debug_state *result = DEBUGGetState(DebugGlobalMemory);
    
    return result;
}

internal void
DEBUGStart(game_memory *memory, game_assets *assets, u32 width, u32 height)
{
    debug_state *debugState = (debug_state *)memory->DebugStorage;
    if (!debugState) return;

    if (!debugState->Initialized) {
        debugState->HighPriorityQueue = memory->HighPriorityQueue;
        
        InitializeArena(
            &debugState->DebugArena,
            memory->DebugStorageSize - sizeof(debug_state),
            debugState+1
            );

        if (!debugState->RenderGroup) {
            debugState->RenderGroup = AllocateRenderGroup(
                &debugState->DebugArena,
                assets,
                Megabytes(15),
                SafeTruncateToUInt16(width),
                SafeTruncateToUInt16(height),
                false
                );
        }

        SubArena(&debugState->CollateArena, &debugState->DebugArena, Megabytes(32), 4);
        debugState->CollateTemp = BeginTemporaryMemory(&debugState->CollateArena);
            
        debugState->Initialized = true;
        debugState->Paused = false;

        debugState->ScopeToRecord = 0;

        RestartCollector(GlobalDebugTable->CurrentWriteEventArrayIndex);
    }

    BeginRender(debugState->RenderGroup);

    MakeOrthographic(debugState->RenderGroup, width, height, 1.0f);
    asset_vector mV = {};
    asset_vector weight = {};
    weight.E[Tag_UnicodePoint] = 1.0f;

    font_id fontId = BestMatchFont(debugState->RenderGroup->Assets, &mV, &weight);

    hha_font *fontInfo = GetFontInfo(debugState->RenderGroup->Assets, fontId);
    
    debugState->FontScale = 1.0f;
    debugState->LineY = 0.5f * (r32)height - fontInfo->Ascend * debugState->FontScale + 1.0f;
    debugState->LeftEdge = -0.5f * (r32)width;

    debugState->GlobalWidth = (r32)width;
    debugState->GlobalHeight = (r32)height;
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
DebugTextAtPoint(char *string, v2 p)
{
    debug_state *debugState = DEBUGGetState();
    
    if (!debugState) return;

    if (debugState->RenderGroup)
    {
        // TODO: move to static variables
        asset_vector mV = {};
        asset_vector weight = {};
        mV.E[Tag_FontType] = (r32)FontType_Debug;
        weight.E[Tag_FontType] = 1.0f;
        font_id fontId = BestMatchFont(debugState->RenderGroup->Assets, &mV, &weight);
        loaded_font *font = GetFont(debugState->RenderGroup->Assets, fontId, debugState->RenderGroup->GenerationId);

        if (!font)
        {
            LoadFont(debugState->RenderGroup->Assets, fontId, true);
            font = GetFont(debugState->RenderGroup->Assets, fontId, debugState->RenderGroup->GenerationId);
        }

        if (font)
        {
            hha_font *fontInfo = GetFontInfo(debugState->RenderGroup->Assets, fontId);
            r32 atX = p.x;
            r32 scale = debugState->FontScale;

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
                    bitmap_id bitmapId = GetBitmapForGlyph(debugState->RenderGroup->Assets, fontInfo, font, codePoint);
                
                    hha_bitmap *info = GetBitmapInfo(debugState->RenderGroup->Assets, bitmapId);

                    PushBitmap(debugState->RenderGroup, bitmapId, scale*(r32)info->Dim[1], ToV3(atX, p.y, 0), ToV4(1,1,1,1));
                }

                prevCodePoint = *at;
            }
        }
    }
}

internal void
DebugTextLine(char *string)
{
    debug_state *debugState = DEBUGGetState();
    if (!debugState) return;

    if (debugState->RenderGroup)
    {
        asset_vector mV = {};
        asset_vector weight = {};
        mV.E[Tag_FontType] = (r32)FontType_Debug;
        weight.E[Tag_FontType] = 1.0f;
        font_id fontId = BestMatchFont(debugState->RenderGroup->Assets, &mV, &weight);
        loaded_font *font = GetFont(debugState->RenderGroup->Assets, fontId, debugState->RenderGroup->GenerationId);

        if (!font)
        {
            LoadFont(debugState->RenderGroup->Assets, fontId, true);
            font = GetFont(debugState->RenderGroup->Assets, fontId, debugState->RenderGroup->GenerationId);
        }

        if (font)
        {
            hha_font *fontInfo = GetFontInfo(debugState->RenderGroup->Assets, fontId);
            DebugTextAtPoint(string, ToV2(debugState->LeftEdge, debugState->LineY));
            debugState->LineY -= GetVerticalLineAdvance(fontInfo) * debugState->FontScale;
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

inline debug_record *
GetRecordFrom(open_debug_block *block)
{
    debug_record *result = block ? block->Source : 0;
    return result;
}

internal void
CollectDebugRecords(u64 invalidArrayIndex)
{
    debug_state *debugState = DEBUGGetState();
    
    if (!debugState) return;
    
    debugState->MaxValue = 0;
    
    for (;;debugState->CollectionArrayIndex++)
    {
        if (debugState->CollectionArrayIndex >= MAX_DEBUG_EVENT_ARRAY_COUNT) {
            debugState->CollectionArrayIndex = 0;
        }

        u64 eventArrayIndex = debugState->CollectionArrayIndex;
        
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
                if (debugState->CollectionFrame) {
                    debugState->CollectionFrame->EndClock = event->Clock;
                    debugState->FrameCount++;
                    debugState->CollectionFrame->WallSecondsElapsed = event->WallClock;
                }
                
                debugState->CollectionFrame = debugState->Frames + debugState->FrameCount;
                debugState->CollectionFrame->BeginClock = event->Clock;
                debugState->CollectionFrame->EndClock = 0;
                debugState->CollectionFrame->RegionCount = 0;
                debugState->CollectionFrame->WallSecondsElapsed = 0.0f;
                debugState->CollectionFrame->Regions = PushArray(&debugState->CollateArena, MAX_DEBUG_REGIONS_PER_FRAME, debug_frame_region);
            }
            else if (debugState->CollectionFrame)
            {
                u32 eventFrameIndex = debugState->FrameCount - 1;
                u64 relativeClock = event->Clock - debugState->CollectionFrame->BeginClock;
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
                    block->Source = src;
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
                                    if (GetRecordFrom(matchingBlock->Parent) == debugState->ScopeToRecord) {
                                        debug_frame_region *region = AddRegion(debugState, debugState->CollectionFrame);
                                        region->LaneIndex = thread->LaneIndex;
                                        region->MinValue = (r32)(openEvent->Clock - debugState->CollectionFrame->BeginClock);
                                        region->MaxValue = (r32)(event->Clock - debugState->CollectionFrame->BeginClock);

                                        region->CycleCount = (event->Clock - openEvent->Clock);
                                        region->Record = src;

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
RestartCollector(u64 invalidArrayIndex)
{
    debug_state *debugState = DEBUGGetState();
    if (!debugState) return;
    
    EndTemporaryMemory(debugState->CollateTemp);
    debugState->CollateTemp = BeginTemporaryMemory(&debugState->CollateArena);

    debugState->FirstThread = 0;
    debugState->FirstFreeBlock = 0;

    debugState->Frames = PushArray(&debugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT * 4, debug_frame);
    
    debugState->FrameBarLaneCount = 0;
    debugState->FrameCount = 0;
    debugState->MaxValue = 1.0f;

    debugState->CollectionFrame = 0;

    debugState->CollectionArrayIndex = (u32)invalidArrayIndex+1;
    
}

internal void
RefreshCollation()
{
    debug_state *debugState = DEBUGGetState();
    if (!debugState) return;
    
    if (debugState->FrameCount > MAX_DEBUG_EVENT_ARRAY_COUNT) {
        RestartCollector(GlobalDebugTable->CurrentWriteEventArrayIndex);
    }
    CollectDebugRecords(GlobalDebugTable->CurrentWriteEventArrayIndex);   
}

internal void
OverlayDebugCycleCounters(game_memory *memory, game_input *input, loaded_bitmap *drawBuffer)
{
    debug_state *debugState = (debug_state *)memory->DebugStorage;

    if (!debugState->RenderGroup) return;

    if (WasPressed(input->MouseButtons[PlatformMouseButton_Right])) {
        debugState->Paused = !debugState->Paused;
    }

    if (debugState)
    {
        v2 mouseP = ToV2(input->MouseX, input->MouseY);
        
        // DebugTextLine("DEBUG CYCLES: \\0414\\0410\\0424\\0444\\0424!");

        if (debugState->FrameCount) {
            char buffer[256];
            _snprintf_s(buffer, 256, "last frame: %.02fms", debugState->Frames[debugState->FrameCount-1].WallSecondsElapsed * 1000.0f);
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

                    PushPieceRect(debugState->RenderGroup, ToV3(chartLeft + (r32)snapIndex, barMinY + 0.5f * barHeight, 0), ToV2(1.0f, barHeight), ToV4(valueNormalized, 1, 0, 1));
                }

                char buffer[256];
                _snprintf_s(buffer, 256, "%s: %u hits: %u", counterState->BlockName, (u32)cycleCount.Avg, (u32)hitCount.Max);
                DebugTextLine(buffer);
            }
        }

        #endif

        debugState->ProfileRect = RectMinMax(ToV2(-650.0f, -200.0f), ToV2(-100.0f, 200.0f));

        u32 maxFrameCount = 10;
        if (debugState->FrameCount < maxFrameCount) {
            maxFrameCount = debugState->FrameCount;
        }

        r32 laneHeight = 0;
        u32 laneCount = debugState->FrameBarLaneCount;
        r32 barSpacing = 0.5f;

        if (maxFrameCount > 0 && laneCount > 0) {
            laneHeight = ((GetDim(debugState->ProfileRect).y / maxFrameCount) + barSpacing) / laneCount;
        }
        
        r32 barHeight = laneHeight * laneCount;
        r32 barHeightAndSpace = barHeight + barSpacing;
        r32 chartHeight = barHeightAndSpace * maxFrameCount;
        r32 chartWidth = GetDim(debugState->ProfileRect).x;
        r32 chartTop = debugState->ProfileRect.Max.y;
        r32 chartLeft = debugState->ProfileRect.Min.x;
        r32 scale = 1.0f;
        if (debugState->MaxValue > 0) {
            scale = chartWidth / debugState->MaxValue;
        }

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

        for (u32 frameIndex=debugState->FrameCount - maxFrameCount;
             frameIndex < debugState->FrameCount;
             frameIndex++)
        {
            debug_frame *frame = debugState->Frames + frameIndex;

            r32 stackX = chartLeft;
            r32 stackY = chartTop - barHeightAndSpace * (r32) (frameIndex - debugState->FrameCount + maxFrameCount);
            
            for (u32 regionIndex=0;
                 regionIndex < frame->RegionCount;
                 regionIndex++)
            {
                debug_frame_region *region = frame->Regions + regionIndex;

                v4 color = colors[regionIndex % ArrayCount(colors)];
                r32 thisMinX = stackX + scale * region->MinValue;
                r32 thisMaxX = stackX + scale * region->MaxValue;

                rectangle2 regionRect = RectMinMax(
                    ToV2(thisMinX, stackY - laneHeight * region->LaneIndex - laneHeight),
                    ToV2(thisMaxX, stackY - laneHeight * region->LaneIndex)
                    );
            
                PushPieceRect(debugState->RenderGroup, regionRect, 0.0f, color);

                if (IsInRectangle(regionRect, mouseP))
                {
                    
                    debug_record *record = region->Record;
                    char buffer[256];
                    _snprintf_s(buffer, 256,
                                "%s: %10I64ucy [%s(%d)]",
                                record->BlockName,
                                region->CycleCount,
                                record->FileName,
                                record->Line);

                    DebugTextAtPoint(buffer, mouseP);

                    if (WasPressed(input->MouseButtons[PlatformMouseButton_Left])) {
                        debugState->ScopeToRecord = record;
                        debugState->ForceCollcationRefresh = true;
                        RefreshCollation();
                    }
                }

                //chartWidth += barWidth + barHeightAndSpace;
            }
        }

        //PushPieceRect(debugState->RenderGroup, ToV3(chartLeft + 0.5f * chartWidth, chartMinY + 2.0f * chartHeight, 0), ToV2(chartWidth, 1.0f), ToV4(1, 1, 1, 1));
    }


    TiledRenderGroup(debugState->HighPriorityQueue, drawBuffer, debugState->RenderGroup);
    EndRender(debugState->RenderGroup);
}