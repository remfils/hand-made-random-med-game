#if !defined(HANDMADE_DEBUG_PLATFORM_H)
#define HANDMADE_DEBUG_PLATFORM_H


// c++ template compiler junk
#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

#define DEBUG_MAX_SNAPSHOT_COUNT 120
#define MAX_DEBUG_EVENT_COUNT 16*65536
#define MAX_DEBUG_RECORD_COUNT 65536

#define DEBUG_INIT_RECORD_ARRAY extern const u32 COMBINE(DebugRecordsCount_, __UnitIndex) = __COUNTER__;
#define DEBUG_DECLARE_RECORD_ARRAY_(suffix) extern const u32 COMBINE(DebugRecordsCount_, suffix);

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

struct debug_table
{
    // extern debug_event DebugEventStorage[2][MAX_DEBUG_EVENT_COUNT];
    volatile u64 ArrayIndex_EventIndex;
    u64 CurrentWriteEventArrayIndex;
    debug_record Records[3][MAX_DEBUG_RECORD_COUNT];
    // NOTE: double rotating buffer logic implemented
    debug_event Events[2][MAX_DEBUG_EVENT_COUNT];
};

extern debug_table GlobalDebugTable;

inline void
RecordDebugEvent(u16 counter, debug_array_type type)
{
    u64 arrayIndex_eventIndex = AtomicAdd64(&GlobalDebugTable.ArrayIndex_EventIndex, 1); 
    u32 eventIndex = arrayIndex_eventIndex & 0xFFFFFFFF; 
    Assert(eventIndex < MAX_DEBUG_EVENT_COUNT); 
    debug_event *event = GlobalDebugTable.Events[arrayIndex_eventIndex >> 32] + eventIndex; 
    event->Clock = __rdtsc();                                       
    event->ThreadIndex = (u16)MyGetCurrentThreadId();                   
    event->CoreIndex = 0;
    event->DebugRecordArrayIndex = __UnitIndex;
    event->DebugRecordIndex = counter;
    event->Type = (u8)type;
}

struct debug_timed_block
{
    u16 Counter = 0;
    
    debug_timed_block(u16 counter, char *fileName ,char *functionName, u16 line)
    {
        this->Counter = counter;

        debug_record *record = GlobalDebugTable.Records[__UnitIndex] + counter;
        record->FileName = fileName;
        record->Function = functionName;
        record->Line = line;

        RecordDebugEvent(this->Counter, DebugEvent_BeginBlock);
    }

    ~debug_timed_block()
    {
        RecordDebugEvent(this->Counter, DebugEvent_EndBlock);
     }
};

#define TIMED_BLOCK debug_timed_block COMBINE(debugCounterBlock, __LINE__)(__COUNTER__, (char *)__FILE__, (char *)__FUNCTION__, (u16)__LINE__);

#endif
