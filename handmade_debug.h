#if !defined(HANDMADE_DEBUG_H)
#define HANDMADE_DEBUG_H

// c++ template compiler junk
#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

////////////////////////////////////////////////////////////////////////////////////////////////////
// debug data macros
////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEBUG_INIT_RECORD_ARRAY debug_record COMBINE(DebugRecords_, UnitPrefix)[__COUNTER__];
#define DEBUG_DECLARE_RECORD_ARRAY debug_record COMBINE(DebugRecords_, UnitPrefix)[];
#define GET_DEBUG_ARRAY_RECORD(id) COMBINE(DebugRecords_, UnitPrefix) + id;

struct debug_record
{
    u64 Clocks;
    u32 HitCount;

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
    
    debug_timed_block(u16 counter, char *fileName ,char *functionName, u16 line)
    {
        this->Record = GET_DEBUG_ARRAY_RECORD(counter);
        
        this->Record->FileName = fileName;
        this->Record->Function = functionName;
        this->Record->Line = line;

        this->Record->Clocks -= __rdtsc();
        ++this->Record->HitCount;
    }

    ~debug_timed_block()
    {
        this->Record->Clocks += __rdtsc();
    }
};


#define TIMED_BLOCK debug_timed_block COMBINE(debugCounterBlock, __LINE__)(__COUNTER__, (char *)__FILE__, (char *)__FUNCTION__, (u16)__LINE__);


#endif