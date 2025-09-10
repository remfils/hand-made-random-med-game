#if !defined(HANDMADE_DEBUG_H)
#define HANDMADE_DEBUG_H

#define DEBUG_MAX_SNAPSHOT_COUNT 120

// c++ template compiler junk
#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

////////////////////////////////////////////////////////////////////////////////////////////////////
// debug data macros
////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_INIT_RECORD_ARRAY extern const u32 COMBINE(DebugRecordsCount_, __UnitIndex) = __COUNTER__; debug_record COMBINE(DebugRecords_, __UnitIndex)[COMBINE(DebugRecordsCount_, __UnitIndex)];

#define DEBUG_DECLARE_RECORD_ARRAY_(suffix) extern const u32 COMBINE(DebugRecordsCount_, suffix); debug_record COMBINE(DebugRecords_, suffix)[];
#define DEBUG_DECLARE_RECORD_ARRAY extern const u32 COMBINE(DebugRecordsCount_, __UnitIndex); debug_record COMBINE(DebugRecords_, __UnitIndex)[];
#define GET_DEBUG_ARRAY_RECORD(id) COMBINE(DebugRecords_, __UnitIndex) + id;

struct debug_record
{
    u64 Clocks_and_HitCount;

    char *FileName;
    char *Function;
    u16 Line;
};

enum debug_array_type
{
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
};

struct debug_event
{
    u64 Clock;
    u16 ThreadIndex;
    u16 CoreIndex;
    u16 DebugRecordIndex;
    u8 DebugRecordArrayIndex;
    u8 Type;
};

#define MAX_DEBUG_EVENT_COUNT 16*65536
extern volatile u64 Debug_ArrayIndex_EventIndex;
extern debug_event DebugEventStorage[2][MAX_DEBUG_EVENT_COUNT];

#define RECORD_DEBUG_EVENT(counter, type)                               \
        u64 arrayIndex_eventIndex = AtomicAdd64(&Debug_ArrayIndex_EventIndex, 1); \
        u32 eventIndex = arrayIndex_eventIndex & 0xFFFFFFFF; \
        Assert(eventIndex < MAX_DEBUG_EVENT_COUNT); \
        debug_event *event = DebugEventStorage[arrayIndex_eventIndex >> 32] + eventIndex; \
        event->Clock = __rdtsc();                                       \
        event->ThreadIndex = 0;                                         \
        event->CoreIndex = 0;                                           \
        event->DebugRecordArrayIndex = __UnitIndex; \
        event->DebugRecordIndex = counter;                              \
        event->Type = type;


DEBUG_DECLARE_RECORD_ARRAY;

////////////////////////////////////////////////////////////////////////////////////////////////////
// debug macros
////////////////////////////////////////////////////////////////////////////////////////////////////

struct debug_timed_block
{
    debug_record *Record;

    u64 StartCounter;
    u32 HitCount = 0;
    u16 Counter = 0;
    
    debug_timed_block(u16 counter, char *fileName ,char *functionName, u16 line)
    {
        this->HitCount = 1;
        this->Record = GET_DEBUG_ARRAY_RECORD(counter);
        this->Counter = counter;
        
        this->Record->FileName = fileName;
        this->Record->Function = functionName;
        this->Record->Line = line;

        this->StartCounter = __rdtsc();

        RECORD_DEBUG_EVENT(this->Counter, DebugEvent_BeginBlock);
    }

    ~debug_timed_block()
    {
        u64 delta = (__rdtsc() - this->StartCounter) | ((u64)this->HitCount << 32);
        AtomicAdd64(&this->Record->Clocks_and_HitCount, delta);

        RECORD_DEBUG_EVENT(this->Counter, DebugEvent_EndBlock);

    }
};


#define TIMED_BLOCK debug_timed_block COMBINE(debugCounterBlock, __LINE__)(__COUNTER__, (char *)__FILE__, (char *)__FUNCTION__, (u16)__LINE__);

////////////////////////////////////////////////////////////////////////////////////////////////////
// debug storage
////////////////////////////////////////////////////////////////////////////////////////////////////


struct debug_counter_data_snapshot
{
    u32 HitCount;
    u64 CycleCount; // TODO: return to u32
};

struct debug_counter_state
{
    char *FileName;
    char *Function;
    u16 Line;

    debug_counter_data_snapshot DataSnapshots[DEBUG_MAX_SNAPSHOT_COUNT];
};

struct debug_state
{
    u32 SnapshotIndex;
    u32 CounterCount;

    debug_counter_state CounterStates[512];

    debug_frame_info FrameInfos[DEBUG_MAX_SNAPSHOT_COUNT];
};

#endif