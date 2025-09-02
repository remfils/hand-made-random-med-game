#if !defined(HANDMADE_DEBUG_H)
#define HANDMADE_DEBUG_H

// c++ template compiler junk
#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

////////////////////////////////////////////////////////////////////////////////////////////////////
// debug data macros
////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_INIT_RECORD_ARRAY extern const u32 COMBINE(UnitPrefix, _DebugRecordsCount) = __COUNTER__; debug_record COMBINE(UnitPrefix, _DebugRecords)[COMBINE(UnitPrefix, _DebugRecordsCount)];

#define DEBUG_DECLARE_RECORD_ARRAY_(prefix) extern const u32 COMBINE(prefix, _DebugRecordsCount); debug_record COMBINE(prefix, _DebugRecords)[];
#define DEBUG_DECLARE_RECORD_ARRAY extern const u32 COMBINE(UnitPrefix, _DebugRecordsCount); debug_record COMBINE(UnitPrefix, _DebugRecords)[];
#define GET_DEBUG_ARRAY_RECORD(id) COMBINE(UnitPrefix, _DebugRecords) + id;

struct debug_record
{
    u64 Clocks_and_HitCount;

    char *FileName;
    char *Function;
    u16 Line;
};

DEBUG_DECLARE_RECORD_ARRAY;

////////////////////////////////////////////////////////////////////////////////////////////////////
// debug macros
////////////////////////////////////////////////////////////////////////////////////////////////////

struct debug_timed_block
{
    debug_record *Record;

    u64 StartCounter;
    u32 HitCount = 0;
    
    debug_timed_block(u16 counter, char *fileName ,char *functionName, u16 line)
    {
        this->HitCount = 1;
        this->Record = GET_DEBUG_ARRAY_RECORD(counter);
        
        this->Record->FileName = fileName;
        this->Record->Function = functionName;
        this->Record->Line = line;

        this->StartCounter = __rdtsc();
    }

    ~debug_timed_block()
    {
        u64 delta = (__rdtsc() - this->StartCounter) | ((u64)this->HitCount << 32);
        AtomicAdd64(&this->Record->Clocks_and_HitCount, delta);
    }
};


#define TIMED_BLOCK debug_timed_block COMBINE(debugCounterBlock, __LINE__)(__COUNTER__, (char *)__FILE__, (char *)__FUNCTION__, (u16)__LINE__);


#endif