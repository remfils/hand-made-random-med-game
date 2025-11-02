#if !defined(HANDMADE_DEBUG_H)
#define HANDMADE_DEBUG_H



////////////////////////////////////////////////////////////////////////////////////////////////////
// debug data macros
////////////////////////////////////////////////////////////////////////////////////////////////////

// #define DEBUG_INIT_RECORD_ARRAY extern const u32 COMBINE(DebugRecordsCount_, __UnitIndex) = __COUNTER__; debug_record COMBINE(DebugRecords_, __UnitIndex)[COMBINE(DebugRecordsCount_, __UnitIndex)];

// #define DEBUG_DECLARE_RECORD_ARRAY_(suffix) extern const u32 COMBINE(DebugRecordsCount_, suffix); debug_record COMBINE(DebugRecords_, suffix)[];
// #define DEBUG_DECLARE_RECORD_ARRAY extern const u32 COMBINE(DebugRecordsCount_, __UnitIndex); debug_record COMBINE(DebugRecords_, __UnitIndex)[];
// #define GET_DEBUG_ARRAY_RECORD(id) COMBINE(DebugRecords_, __UnitIndex) + id;

// extern volatile u64 Debug_ArrayIndex_EventIndex;

struct debug_counter_data_snapshot
{
    u32 HitCount;
    u64 CycleCount; // TODO: return to u32
};

struct debug_counter_state
{
    char *FileName;
    char *BlockName;
    u16 Line;

    debug_counter_data_snapshot DataSnapshots[DEBUG_MAX_SNAPSHOT_COUNT];
};

struct debug_frame_region
{
    debug_record *Record;
    u64 CycleCount;
    r32 MinValue;
    r32 MaxValue;
    u32 LaneIndex;
};

struct debug_frame
{
    u64 BeginClock;
    u64 EndClock;
    u32 RegionCount;
    r32 WallSecondsElapsed;
    debug_frame_region *Regions;
};

struct open_debug_block
{
    debug_event *OpeningEvent;
    debug_record *Source;
    open_debug_block *Parent;
    open_debug_block *NextFree;
    u32 StartingFrameIndex;
};

struct debug_thread
{
    u16 ThreadId;
    u32 LaneIndex;
    
    open_debug_block *FirstOpenBlock;

    debug_thread *Next;
};

struct debug_state
{
//     u32 SnapshotIndex;
//     u32 CounterCount;
// 
//     debug_counter_state CounterStates[512];

    b32 Initialized;
    b32 Paused;

    platform_work_queue *HighPriorityQueue;
    
    memory_arena DebugArena;
    
    memory_arena CollateArena;
    temporary_memory CollateTemp;

    debug_record *ScopeToRecord;
    b32 ForceCollcationRefresh;

    u32 FrameBarLaneCount;
    r32 MaxValue;

    u32 CollectionArrayIndex;
    debug_frame *CollectionFrame;

    u32 FrameCount;
    debug_frame *Frames;

    debug_thread *FirstThread;
    open_debug_block *FirstFreeBlock;


    render_group *RenderGroup;

    r32 LeftEdge = 0.0f;
    r32 LineY = 0.0f;
    r32 FontScale = 0.0f;
    r32 GlobalWidth = 0.0f;
    r32 GlobalHeight = 0.0f;

    rectangle2 ProfileRect;
};

#endif