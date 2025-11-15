#include <stdio.h>

DEBUG_DECLARE_RECORD_ARRAY_(0);
DEBUG_DECLARE_RECORD_ARRAY_(1);
DEBUG_DECLARE_RECORD_ARRAY_(2);

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

internal debug_variable_view *
AddDebugVariableView(debug_state *debugState, debug_global_variable_reference *group, v2 p)
{
    debug_variable_view *view = PushStruct(&debugState->DebugArena, debug_variable_view);
    view->Root = group;
    view->P = p;

    view->Next = debugState->VariableViewSentinel.Next;
    view->Prev = &debugState->VariableViewSentinel;

    view->Next->Prev = view;
    view->Prev->Next = view;

    return view;
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

        debugState->VariableViewSentinel.Next = &debugState->VariableViewSentinel;
        debugState->VariableViewSentinel.Prev = &debugState->VariableViewSentinel;
        debugState->VariableViewSentinel.Root = {0};
        debugState->VariableViewSentinel.P = {0};

        debug_global_variable_context ctx_ = {debugState, &debugState->DebugArena};
        debug_global_variable_context *ctx = &ctx_;

        DEBUGInitVaraibles(ctx);

        debug_global_variable_reference *prifle = DebugAddVariable_(ctx, DebugGlobalVariableType_ProfileGraph, "Profile Graph");
        prifle->Var->Graph.Dim.x = 300.0f;
        prifle->Var->Graph.Dim.y = 100.0f;

        AddDebugVariableView(debugState, debugState->RootVariable, ToV2(
            -0.5f * (r32)width,
            0.5f * (r32)height
            ));

        if (!debugState->RenderGroup) {
            debugState->RenderGroup = AllocateRenderGroup(
                &debugState->DebugArena,
                assets,
                Megabytes(30),
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

    asset_vector mV = {};
    asset_vector weight = {};
    mV.E[Tag_FontType] = (r32)FontType_Debug;
    weight.E[Tag_FontType] = 1.0f;
    font_id fontId = BestMatchFont(debugState->RenderGroup->Assets, &mV, &weight);
    debugState->Font = GetFont(debugState->RenderGroup->Assets, fontId, debugState->RenderGroup->GenerationId);

    if (!debugState->Font) {
        LoadFont(debugState->RenderGroup->Assets, fontId, true);
        debugState->Font = GetFont(debugState->RenderGroup->Assets, fontId, debugState->RenderGroup->GenerationId);
    }
    debugState->FontInfo = GetFontInfo(debugState->RenderGroup->Assets, fontId);

    MakeOrthographic(debugState->RenderGroup, width, height, 1.0f);
    debugState->FontScale = 1.0f;
    debugState->LineY = 0.5f * (r32)height - debugState->FontInfo->Ascend * debugState->FontScale + 1.0f;
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

internal rectangle2
DebugTextOperation(
    debug_text_operation operation,
    debug_state *debugState, char *string, v2 p, v4 textColor)
{
    rectangle2 result = {};
    
    loaded_font *font = debugState->Font;
    hha_font *fontInfo = debugState->FontInfo;

    if (font && fontInfo)
    {
        
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

                r32 bitmapScale = scale*(r32)info->Dim[1];
                v3 bitmapOffset = ToV3(atX, p.y, 0);
                if (operation == DebugTextOperation_Draw) {
                    PushBitmap(debugState->RenderGroup, bitmapId, bitmapScale, bitmapOffset, textColor);
                } else if (operation == DebugTextOperation_Size) {
                    loaded_bitmap * bitmap = GetBitmap(debugState->RenderGroup->Assets, bitmapId, debugState->RenderGroup->GenerationId);
                    if (bitmap) {
                        used_bitmap_dim dim = GetBitmapDim(debugState->RenderGroup, bitmap, bitmapScale, bitmapOffset);
                        rectangle2 glyphDim = RectMinDim(dim.P.xy, dim.Size);
                        result = Union(result, glyphDim);
                    }
                } else {
                    InvalidCodePath;
                }
            }

            prevCodePoint = *at;
        }
    }
    

    return result;
}

internal void
DebugTextAtPoint(char *string, v2 p, v4 color)
{
    debug_state *debugState = DEBUGGetState();
    if (!debugState) return;

    if (debugState->RenderGroup && debugState->Font && debugState->FontInfo)
    {
        DebugTextOperation(DebugTextOperation_Draw, debugState, string, p, color);
    }
}
internal rectangle2
DebugTextSize(char *string)
{
    rectangle2 result = {};
    debug_state *debugState = DEBUGGetState();
    if (!debugState) return result;

    if (debugState->RenderGroup && debugState->Font && debugState->FontInfo)
    {
        result = DebugTextOperation(DebugTextOperation_Size, debugState, string, ToV2(0,0), ToV4(1,1,1,1));
    }
    return result;
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
            DebugTextAtPoint(string, ToV2(debugState->LeftEdge, debugState->LineY), ToV4(1,1,1,1));
            debugState->LineY -= GetVerticalLineAdvance(debugState->FontInfo) * debugState->FontScale;
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
DrawDebugContextMenu(v2 mouseP)
{
    #if 0
    debug_state *debugState = DEBUGGetState();
    if (!debugState->RenderGroup) return;


    u32 bestMenuIndex = 0;
    r32 bestMenuDistanceSq = FLT_MAX;

    r32 menuRadius = 200.0f;
    r32 angleStep = Tau32 / (r32) ArrayCount(DebugVariables);

    v4 outlineColor = {0.0f,1.0f,0.0f, 0.2f};
    v4 textColor = {1.0f,1.0f,1.0f, 1.0f};
    v4 highlightedColor = {0.0f, 0.0f, 0.0f, 1.0f};
    v4 highlightedBgColor = {0.0f,0.3f,0.0f, 0.2f};

    for (s32 debugVariableIndex=0; debugVariableIndex < ArrayCount(DebugVariables); debugVariableIndex++)
    {
        r32 angle = (r32)debugVariableIndex * angleStep;
        v2 textP = debugState->MenuP + menuRadius * Arm2(angle);

        rectangle2 textBox = DebugTextSize(DebugVariables[debugVariableIndex].Name);

        textP = textP - 0.5f * GetDim(textBox);

        r32 menuDistanceSq = LengthSq(textP - mouseP);

        if (menuDistanceSq < bestMenuDistanceSq) {
            bestMenuDistanceSq = menuDistanceSq;
            bestMenuIndex = debugVariableIndex;
        }

        v4 color = textColor;
        v4 bgColor = outlineColor;
        if (debugVariableIndex == debugState->HoverMenuIndex) {
            color = highlightedColor;
            bgColor = highlightedBgColor;
        }

        PushPieceRect(debugState->RenderGroup, Offset(textBox, textP), 0.0f, outlineColor);
        DebugTextAtPoint(DebugVariables[debugVariableIndex].Name, textP, color);
    }


    debugState->HoverMenuIndex = bestMenuIndex;

    #endif
}

inline u32
DebugGlobalVariableToString(debug_global_variable *var, char *text, char *end, u32 flags)
{
    u32 bytesLeftToWrite;
    char *at = text;

    if (var->Type == DebugGlobalVariableType_Group) {

        if (flags & DebugGlobalVariableStringFlag_Define) {
            bytesLeftToWrite = (u32)(end - at);
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "// ");
        } else {
            bytesLeftToWrite = (u32)(end - at);
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%s ", (var->Group.IsOpen ? "-" : "+"));
        }

        if (flags & DebugGlobalVariableStringFlag_Name) {
            bytesLeftToWrite = (u32)(end - at);
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%s", var->Name);
        }
    } else {
        if (flags & DebugGlobalVariableStringFlag_Define) {
            bytesLeftToWrite = (u32)(end - at);
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "#define ");
        }

        if (flags & DebugGlobalVariableStringFlag_Name) {
            bytesLeftToWrite = (u32)(end - at);
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%s", var->Name);

            if (flags & DebugGlobalVariableStringFlag_DotsNameDelimiter) {
                bytesLeftToWrite = (u32)(end - at);
                at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, ": ");
            } else {
                bytesLeftToWrite = (u32)(end - at);
                at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, " ");
            }
        }

        bytesLeftToWrite = (u32)(end - at);
    
        switch (var->Type)
        {
        case DebugGlobalVariableType_b32:
        {
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%s ", (var->Bool ? "true" : "false"));
        } break;
        case DebugGlobalVariableType_s32:
        {
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%d ", var->Int);
        } break;
        case DebugGlobalVariableType_u32:
        {
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%u ", var->Uint);
        } break;
        case DebugGlobalVariableType_r32:
        {
            at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "%f", var->Real);

            if (flags & DebugGlobalVariableStringFlag_PressFForFloat) {
                bytesLeftToWrite = (u32)(end - at);
                at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "f ");
            } else {
                bytesLeftToWrite = (u32)(end - at);
                at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, " ");
            }
        } break;

        InvalidDefaultCase;
        
        }
    }

    if (flags & DebugGlobalVariableStringFlag_LineFeed) {
        bytesLeftToWrite = (u32)(end - at);
        at += _snprintf_s(at, bytesLeftToWrite, bytesLeftToWrite, "\n");
    }

    if (flags & DebugGlobalVariableStringFlag_NullTerminated) {
        *at++ = 0;
    }

    return (u32)(at - text);
}

internal void
DrawDebugProfileGraph(debug_state *debugState, rectangle2 bounds, v2 mouseP)
{
    u32 maxFrameCount = 10;
    if (debugState->FrameCount < maxFrameCount) {
        maxFrameCount = debugState->FrameCount;
    }

    r32 laneHeight = 0;
    u32 laneCount = debugState->FrameBarLaneCount;
    r32 barSpacing = 0.5f;

    if (maxFrameCount > 0 && laneCount > 0) {
        laneHeight = ((GetDim(bounds).y / maxFrameCount) + barSpacing) / laneCount;
    }
        
    r32 barHeight = laneHeight * laneCount;
    r32 barHeightAndSpace = barHeight + barSpacing;
    r32 chartHeight = barHeightAndSpace * maxFrameCount;
    r32 chartWidth = GetDim(bounds).x;
    r32 chartTop = bounds.Max.y;
    r32 chartLeft = bounds.Min.x;
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

                DebugTextAtPoint(buffer, mouseP, ToV4(1,1,1,1));

                #if 0
                if (WasPressed(input->MouseButtons[PlatformMouseButton_Left])) {
                    debugState->ScopeToRecord = record;
                    debugState->ForceCollcationRefresh = true;
                    RefreshCollation();
                }
                #endif
            }

            //chartWidth += barWidth + barHeightAndSpace;
        }
    }
}

internal void
DrawDebugGlobalVariableMenu(debug_state *debugState, v2 mouseP)
{
    debugState->NextHoverRef = 0;
    debugState->NextHoverView = 0;
    debugState->NextHoverInteraction = DebugInteractionType_None;
    

    for (
        debug_variable_view *variableView = debugState->VariableViewSentinel.Next;
        variableView != &debugState->VariableViewSentinel;
        variableView = variableView->Next
        )
    {
        debug_global_variable_reference *ref = variableView->Root->Var->Group.FirstChild;

        r32 lineAdvance = GetVerticalLineAdvance(debugState->FontInfo) * debugState->FontScale + 2;
        r32 textVPosition = variableView->P.y;
        u32 depth = 0;

        while (ref)
        {
            char text[256];
            rectangle2 bounds;
            b32 isHover = debugState->HoverRef == ref;

            v4 textColor = isHover
                ? ToV4(1,1,0,1)
                : ToV4(1,1,1,1);

            switch (ref->Var->Type)
            {
            case DebugGlobalVariableType_ProfileGraph: {
                r32 chartWidth = ref->Var->Graph.Dim.x;
                r32 chartHeight = ref->Var->Graph.Dim.y;
                bounds = RectMinMax(
                    ToV2(variableView->P.x, textVPosition - chartHeight), ToV2(variableView->P.x + chartWidth, textVPosition)
                    );
                DrawDebugProfileGraph(debugState, bounds, mouseP);

                rectangle2 sizeHandleRect = RectCenterDim(
                    ToV2(bounds.Max.x, bounds.Min.y),
                    ToV2(8.0f, 8.0f)
                    );

                v4 sizeHandleColor = ToV4(1,1,1,1);

                if (isHover) {
                    if (debugState->Interaction == DebugInteractionType_ResizeProfileGraph) {
                        sizeHandleColor = ToV4(0,0,1,1);
                    } else {
                        sizeHandleColor = ToV4(1,0,0,1);
                    }
                }

                if (IsInRectangle(sizeHandleRect, mouseP))
                {
                    debugState->NextHoverInteraction = DebugInteractionType_ResizeProfileGraph;
                    debugState->NextHoverRef = ref;
                }
                else if (IsInRectangle(bounds, mouseP))
                {
                    debugState->NextHoverRef = ref;
                }
            
                PushPieceRect(debugState->RenderGroup, sizeHandleRect, 0.0f, sizeHandleColor);

                textVPosition -= chartHeight;
            } break;
            default: {

#if DEBUGUI_ShowVariableConfigOutput
                DebugGlobalVariableToString(
                    ref->Var, text, text + sizeof(text),
                    DebugGlobalVariableStringFlag_Name
                    | DebugGlobalVariableStringFlag_DotsNameDelimiter
                    | DebugGlobalVariableStringFlag_NullTerminated);
#else

                DebugGlobalVariableToString(
                    ref->Var, text, text + sizeof(text),
                    DebugGlobalVariableStringFlag_Define
                    | DebugGlobalVariableStringFlag_Name
                    | DebugGlobalVariableStringFlag_PressFForFloat
                    | DebugGlobalVariableStringFlag_LineFeed);

#endif

                v2 textP = ToV2(
                    variableView->P.x + lineAdvance * depth,
                    textVPosition - lineAdvance);
                bounds = DebugTextSize(text);
                bounds = Offset(bounds, textP);
                bounds.Min.y = bounds.Max.y - lineAdvance;

                DebugTextAtPoint(text, textP, textColor);

                textVPosition -= lineAdvance;

                if (IsInRectangle(bounds, mouseP))
                {
                    debugState->NextHoverRef = ref;
                }
            } break;
            }

            if (ref->Var->Type == DebugGlobalVariableType_Group && ref->Var->Group.IsOpen)
            {
                ref = ref->Var->Group.FirstChild;
                depth++;
            }
            else
            {
                while(ref)
                {
                    if (ref->Next)
                    {
                        ref = ref->Next;
                        break;
                    }
                    else
                    {
                        ref = ref->Parent;
                        depth--;
                    }
                }
            }
        }

        rectangle2 moveHandleRect = RectCenterDim(
            variableView->P + ToV2(-3.0f, 3.0f),
            ToV2(8.0f, 8.0f)
            );

        v4 moveHandleColor = ToV4(1,1,1,1);

        if (IsInRectangle(moveHandleRect, mouseP))
        {
            debugState->NextHoverInteraction = DebugInteractionType_MoveView;
            debugState->NextHoverView = variableView;
        }

        if (debugState->DraggingView == variableView)
        {
            moveHandleColor = ToV4(1,0,0,1);
        }

        PushPieceRect(debugState->RenderGroup, moveHandleRect, 0.0f, moveHandleColor);
    }
}

internal void
WriteHandmadeConfig(debug_state *debugState)
{
    if (!debugState->CompilerProcess.IsRunning) {
        char temp[4096];

        char *at = temp;

        debug_global_variable_reference *ref = debugState->RootVariable;

        while (ref)
        {
            if (ref->Var->IsStored) {
                u32 bytesLeftToWrite = (u32)(temp + ArrayCount(temp) - at);

                at += DebugGlobalVariableToString(
                    ref->Var, at, temp + ArrayCount(temp),
                    DebugGlobalVariableStringFlag_Define
                    | DebugGlobalVariableStringFlag_Name
                    | DebugGlobalVariableStringFlag_PressFForFloat
                    | DebugGlobalVariableStringFlag_LineFeed);   
            }

            if (ref->Var->Type == DebugGlobalVariableType_Group)
            {
                ref = ref->Var->Group.FirstChild;
            }
            else
            {
                while(ref)
                {
                    if (ref->Next)
                    {
                        ref = ref->Next;
                        break;
                    }
                    else
                    {
                        ref = ref->Parent;
                    }
                }
            }
        }

        PlatformAPI.DEBUG_WriteEntireFile("../code/handmade_config.h", (u32)(at - temp), temp);

        debugState->CompilerProcess = PlatformAPI.DEBUG_ExecuteSystemCommand("../misc/", "c:\\Windows\\System32\\cmd.exe", "/C build.bat");
    }
}

internal void
BeginInteractionWithGlobalVariableState(debug_state *debugState, game_input *input, v2 mouseP, b32 alternativeUi)
{
    if (debugState->HoverInteraction)
    {
        debugState->Interaction = debugState->HoverInteraction;
    }
    else
    {
        if (alternativeUi)
        {
            debugState->Interaction = DebugInteractionType_Clone;
        }
        else
        {
            if (debugState->HoverRef) {
                switch (debugState->HoverRef->Var->Type)
                {
                case DebugGlobalVariableType_r32: {
                    debugState->Interaction = DebugInteractionType_Drag;
                } break;
                case DebugGlobalVariableType_b32: {
                    debugState->Interaction = DebugInteractionType_Toggle;
                } break;
                case DebugGlobalVariableType_Group: {
                    debugState->Interaction = DebugInteractionType_Toggle;
                } break;
                }
            } else {
                debugState->Interaction = DebugInteractionType_Empty;
            }
        }
    }

    if (debugState->Interaction) {
        debugState->InteractingRef = debugState->HoverRef;
    }
}

internal void
EndInteractionWithGlobalVariableState(debug_state *debugState, game_input *input, v2 mouseP)
{
    if (debugState->Interaction != DebugInteractionType_Empty) {
        debug_global_variable_reference *ref = debugState->InteractingRef;
        
        if (ref)
        {
            b32 doRecompile = false;

            switch (debugState->Interaction)
            {
            case DebugInteractionType_Drag: {
                doRecompile = true;
            } break;
            case DebugInteractionType_Toggle: {
                switch (ref->Var->Type)
                {
                case DebugGlobalVariableType_b32: {
                    ref->Var->Bool = !ref->Var->Bool;
                    doRecompile = true;
                } break;
                case DebugGlobalVariableType_Group: {
                    ref->Var->Group.IsOpen = !ref->Var->Group.IsOpen;
                } break;
                }
            } break;
            }

            //doRecompile = false; // DEBUG

            if (doRecompile) {
                WriteHandmadeConfig(debugState);
            }
        }
    }
    

    debugState->InteractingRef = 0;
    debugState->DraggingView = 0;
    debugState->Interaction = DebugInteractionType_None;
}

internal void
HandleDebugGlobalVariableInteractions(debug_state *debugState, game_input *input, v2 mouseP, b32 alternativeUi)
{
    v2 mousedp = mouseP - debugState->PrevMouseP;

    if (debugState->Interaction) {
        if (debugState->InteractingRef)
        {
            // mouse drag

            debug_global_variable *var = debugState->InteractingRef->Var;

            switch (debugState->Interaction)
            {
            case DebugInteractionType_ResizeProfileGraph: {
                var->Graph.Dim.x += mousedp.x;
                var->Graph.Dim.y -= mousedp.y;
                var->Graph.Dim.x = Maximum(var->Graph.Dim.x, 10.0f);
                var->Graph.Dim.y = Maximum(var->Graph.Dim.y, 10.0f);
            } break;
            case DebugInteractionType_Drag: {
                switch (var->Type) {
                case DebugGlobalVariableType_r32: {
                    var->Real += 0.1f * mousedp.x;
                } break;
                }
            } break;
            case DebugInteractionType_Clone: {
                if (!debugState->DraggingView) {
                    debug_global_variable_reference *group = DebugNewVariableGroup("CustomGroup", 0, debugState);
                    group->Var->Group.IsOpen = true;
                    DebugAddVariableReference(debugState, group, debugState->InteractingRef->Var);
                    debugState->DraggingView = AddDebugVariableView(debugState, group, mouseP);
                }

                debugState->DraggingView->P = mouseP;
                
            } break;
            }
        }
        else
        {
            switch (debugState->Interaction)
            {
            case DebugInteractionType_MoveView: {
                debugState->DraggingView->P = mouseP;
            } break;
            }
        }

        // click interactions
        for (
            u32 transitionIndex = input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
            transitionIndex > 1;
            transitionIndex--
            )
        {
            EndInteractionWithGlobalVariableState(debugState, input, mouseP);
            BeginInteractionWithGlobalVariableState(debugState, input, mouseP, alternativeUi);
        }

        if (!input->MouseButtons[PlatformMouseButton_Left].EndedDown) {
            EndInteractionWithGlobalVariableState(debugState, input, mouseP);
        }
    } else {
        debugState->HoverRef = debugState->NextHoverRef;
        debugState->HoverInteraction = debugState->NextHoverInteraction;
        debugState->DraggingView = debugState->NextHoverView;

        for (
            u32 transitionIndex = input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
            transitionIndex > 1;
            transitionIndex--
            )
        {
            BeginInteractionWithGlobalVariableState(debugState, input, mouseP, alternativeUi);
            EndInteractionWithGlobalVariableState(debugState, input, mouseP);
        }

        if (input->MouseButtons[PlatformMouseButton_Left].EndedDown) {
            BeginInteractionWithGlobalVariableState(debugState, input, mouseP, alternativeUi);
        }
    }

    debugState->PrevMouseP = mouseP;
}

internal void
OverlayDebugCycleCounters(game_memory *memory, game_input *input, loaded_bitmap *drawBuffer)
{
    debug_state *debugState = (debug_state *)memory->DebugStorage;
    if (!debugState->RenderGroup) return;

    if (debugState)
    {
        v2 mouseP = ToV2(input->MouseX, input->MouseY);
        
        // DebugTextLine("DEBUG CYCLES: \\0414\\0410\\0424\\0444\\0424!");

        if (debugState->FrameCount) {
            char buffer[256];
            _snprintf_s(buffer, 256, "last frame: %.02fms", debugState->Frames[debugState->FrameCount-1].WallSecondsElapsed * 1000.0f);
            DebugTextLine(buffer);
        }

        if (debugState->CompilerProcess.IsRunning) {
            debug_process_state state = PlatformAPI.DEBUG_GetProcessState(debugState->CompilerProcess);
            if (state.Ok) {
                if (state.Finished) {
                    debugState->CompilerProcess.IsRunning = false;
                } else {
                    DebugTextLine("COMPILING...");
                }
            } else {
                DebugTextLine("Error when compiling");
                debugState->CompilerProcess.IsRunning = false;
            }
            
        }

        //WriteHandmadeConfig(debugState); // DEBUG

        b32 useAltUi = input->MouseButtons[PlatformMouseButton_Right].EndedDown;
        if (useAltUi) {
            int a = 1;
        }

        DrawDebugGlobalVariableMenu(debugState, mouseP);
        HandleDebugGlobalVariableInteractions(debugState, input, mouseP, useAltUi);

        #if 0

        if (input->MouseButtons[PlatformMouseButton_Right].EndedDown) {
            if (input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0) {
                debugState->MenuP = mouseP;
            }
            DrawDebugContextMenu(mouseP);
        } else {
            if (input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0) {

                #if 0
                if (debugState->HoverMenuIndex < ArrayCount(DebugVariables)) {
                    DebugVariables[debugState->HoverMenuIndex].Value = !DebugVariables[debugState->HoverMenuIndex].Value;
                }
                #endif
                WriteHandmadeConfig(debugState);
                DrawDebugContextMenu(mouseP);
            }
        }

        #endif

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

        //PushPieceRect(debugState->RenderGroup, ToV3(chartLeft + 0.5f * chartWidth, chartMinY + 2.0f * chartHeight, 0), ToV2(chartWidth, 1.0f), ToV4(1, 1, 1, 1));
    }


    TiledRenderGroup(debugState->HighPriorityQueue, drawBuffer, debugState->RenderGroup);
    EndRender(debugState->RenderGroup);
}