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
    r32 MinValue;
    r32 MaxValue;
    u32 LaneIndex;
};

struct debug_frame
{
    u64 BeginClock;
    u64 EndClock;
    u32 RegionCount;
    debug_frame_region *Regions;
};



struct debug_state
{
//     u32 SnapshotIndex;
//     u32 CounterCount;
// 
//     debug_counter_state CounterStates[512];

    b32 Initialized;
    memory_arena CollateArena;
    temporary_memory CollateTemp;

    u32 FrameBarLaneCount;
    r32 FrameBarScale;

    u32 FrameCount;
    debug_frame *Frames;
};

#endif