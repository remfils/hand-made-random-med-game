#if !defined(HANDMADE_DEBUG_PLATFORM_H)
#define HANDMADE_DEBUG_PLATFORM_H


// c++ template compiler junk
#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

#define DEBUG_MAX_SNAPSHOT_COUNT 120
#define MAX_DEBUG_EVENT_COUNT 16*65536
#define MAX_DEBUG_RECORD_COUNT 65536
#define MAX_UNIT_COUNT 3
#define MAX_DEBUG_FRAME_COUNT 64

#define DEBUG_INIT_RECORD_ARRAY extern const u32 COMBINE(DebugRecordsCount_, __UnitIndex) = __COUNTER__;
#define DEBUG_DECLARE_RECORD_ARRAY_(suffix) extern const u32 COMBINE(DebugRecordsCount_, suffix);
#define DEBUG_DECLARE_RECORD_ARRAY DEBUG_DECLARE_RECORD_ARRAY_(__UnitIndex);

struct debug_record
{
    char *FileName;
    char *BlockName;
    u16 Line;
};

enum debug_array_type
{
    DebugEvent_FrameMarker,
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
};

struct debug_event
{
    u64 Clock;
    u16 ThreadIndex;
    u16 CoreIndex;
    u16 DebugRecordIndex;
    u8 UnitIndex;
    u8 Type;
};

struct debug_table
{
    volatile u64 ArrayIndex_EventIndex;
    u64 CurrentWriteEventArrayIndex;
    // NOTE: double rotating buffer logic implemented
    u32 EventCount[MAX_DEBUG_FRAME_COUNT];
    debug_event Events[MAX_DEBUG_FRAME_COUNT][MAX_DEBUG_EVENT_COUNT];

    u32 RecordCount[MAX_UNIT_COUNT];
    debug_record Records[MAX_UNIT_COUNT][MAX_DEBUG_RECORD_COUNT];
};

extern debug_table *GlobalDebugTable;

inline void
RecordDebugEvent(u16 counter, debug_array_type type)
{
    u64 arrayIndex_eventIndex = AtomicAdd64(&GlobalDebugTable->ArrayIndex_EventIndex, 1); 
    u32 eventIndex = arrayIndex_eventIndex & 0xFFFFFFFF; 
    Assert(eventIndex < MAX_DEBUG_EVENT_COUNT); 
    debug_event *event = GlobalDebugTable->Events[arrayIndex_eventIndex >> 32] + eventIndex; 
    event->Clock = __rdtsc();                                       
    event->ThreadIndex = (u16)MyGetCurrentThreadId();                   
    event->CoreIndex = 0;
    event->UnitIndex = __UnitIndex;
    event->DebugRecordIndex = counter;
    event->Type = (u8)type;
}

struct debug_timed_block
{
    u16 Counter;
    b32 Finished;
    
    debug_timed_block(u16 counter, char *fileName ,char *blockName, u16 line)
    {
        this->Counter = counter;
        this->Finished = false;

        debug_record *record = GlobalDebugTable->Records[__UnitIndex] + counter;
        record->FileName = fileName;
        record->BlockName = blockName;
        record->Line = line;

        RecordDebugEvent(this->Counter, DebugEvent_BeginBlock);
    }

    void EndRecording()
    {
        RecordDebugEvent(this->Counter, DebugEvent_EndBlock);
        this->Finished = true;
    }

    ~debug_timed_block()
    {
        if (!this->Finished) {
            this->EndRecording();
        }
    }
};

#define TIMED_FUNCTION debug_timed_block COMBINE(debugCounterBlock, __LINE__)(__COUNTER__, (char *)__FILE__, (char *)__FUNCTION__, (u16)__LINE__);
#define TIMED_BLOCK(blockName) debug_timed_block COMBINE(debugCounterBlock, blockName)(__COUNTER__, (char *)__FILE__, (char *)#blockName, (u16)__LINE__);

#define BEGIN_TIMED_BLOCK(label) debug_timed_block COMBINE(debugNamedCounterBlock_, label)(__COUNTER__, (char *)__FILE__, (char *)#label, (u16)__LINE__);
#define END_TIMED_BLOCK(label) COMBINE(debugNamedCounterBlock_, label).EndRecording();


#define FRAME_MARKER \
    {                                                                   \
        u16 counter = __COUNTER__;                                      \
        debug_record *record = GlobalDebugTable->Records[__UnitIndex] + counter; \
        record->FileName = __FILE__;                                    \
        record->BlockName = "Frame Marker";                             \
        record->Line = __LINE__;                                        \
                                                                        \
        RecordDebugEvent(counter, DebugEvent_FrameMarker);              \
    }

#endif
