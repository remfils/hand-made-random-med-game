#if !defined(HANDMADE_DEBUG_VARAIBLES_H)
#define HANDMADE_DEBUG_VARAIBLES_H

enum debug_global_variable_type
{
    DebugGlobalVariableType_b32,
    DebugGlobalVariableType_s32,
    DebugGlobalVariableType_u32,
    DebugGlobalVariableType_r32,
    DebugGlobalVariableType_Group,
    DebugGlobalVariableType_Count,
};

enum debug_global_variable_string_flag
{
    DebugGlobalVariableStringFlag_Define = 0x1,
    DebugGlobalVariableStringFlag_Name = 0x2,
    DebugGlobalVariableStringFlag_DotsNameDelimiter = 0x4,
    DebugGlobalVariableStringFlag_PressFForFloat = 0x8,
    DebugGlobalVariableStringFlag_LineFeed = 0x10,
    DebugGlobalVariableStringFlag_NullTerminated = 0x20,
};

struct debug_global_variable;

struct debug_global_variable_group
{
    b32 IsOpen;
    debug_global_variable *FirstChild;
    debug_global_variable *LastChild;
};

struct debug_global_variable {
    char *Name;
    debug_global_variable_type Type;
    debug_global_variable *Next;
    debug_global_variable *Parent;

    union {
        b32 Bool;
        u32 Uint;
        s32 Int;
        r32 Real;
        debug_global_variable_group Group;
    };
};


struct debug_global_variable_context
{
    debug_state *DebugState;
    memory_arena *Arena;

    debug_global_variable *Group;
    
};

internal debug_global_variable*
DebugAddVariable_(debug_global_variable_context *ctx, debug_global_variable_type type, char *name)
{
    debug_global_variable *result = PushStruct(ctx->Arena, debug_global_variable);
    result->Name = (char *)PushCopy(ctx->Arena, StringLength(name) + 1, name);
    result->Type = type;
    result->Next = 0;

    debug_global_variable *group = ctx->Group;
    
    result->Parent = group;

    if (group)
    {
        if (group->Group.LastChild)
        {
            group->Group.LastChild->Next = result;
            group->Group.LastChild = result;
        }
        else
        {
            group->Group.FirstChild = result;
            group->Group.LastChild = result;
        }
    }
    
    return result;
}

internal debug_global_variable*
DebugAddVariable(debug_global_variable_context *ctx, char *name, b32 value)
{
    debug_global_variable* result = DebugAddVariable_(ctx, DebugGlobalVariableType_b32, name);
    result->Bool = value;
    return result;
}

internal debug_global_variable*
DebugAddVariable(debug_global_variable_context *ctx, char *name, r32 value)
{
    debug_global_variable* result = DebugAddVariable_(ctx, DebugGlobalVariableType_r32, name);
    result->Real = value;
    return result;
}

internal debug_global_variable*
DebugAddVariable(debug_global_variable_context *ctx, char *name, u32 value)
{
    debug_global_variable* result = DebugAddVariable_(ctx, DebugGlobalVariableType_u32, name);
    result->Uint = value;
    return result;
}

internal void
DebugBeginVariableGroup(debug_global_variable_context *ctx, char *name)
{
    debug_global_variable* group = DebugAddVariable_(ctx, DebugGlobalVariableType_Group, name);
    group->Group.IsOpen = false;
    group->Group.FirstChild = group->Group.LastChild = 0;
    ctx->Group = group;
}

internal void
DebugEndVariableGroup(debug_global_variable_context *ctx)
{
    Assert(ctx->Group);
    ctx->Group = ctx->Group->Parent;
}

internal void
DEBUGInitVaraibles(debug_state *debugState, memory_arena *arena)
{
    debug_global_variable_context ctx_ = {debugState, arena};
    debug_global_variable_context *ctx = &ctx_;

#define DEBUG_GLOBAL_VARIABLE_LINE_b32(name) DebugAddVariable(ctx, #name, (b32)name);
#define DEBUG_GLOBAL_VARIABLE_LINE_s32(name) DebugAddVariable(ctx, #name, (s32)name);
#define DEBUG_GLOBAL_VARIABLE_LINE_u32(name) DebugAddVariable(ctx, #name, (u32)name);
#define DEBUG_GLOBAL_VARIABLE_LINE_r32(name) DebugAddVariable(ctx, #name, (r32)name);

    DebugBeginVariableGroup(ctx, "Root");

    debugState->RootVariable = ctx->Group;
    
    {
        DebugBeginVariableGroup(ctx, "Camera");
        DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_UseDebugCamera);
        DEBUG_GLOBAL_VARIABLE_LINE_r32(DEBUGUI_DebugCameraDistance);
        DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_CameraIsTileBased);
        DebugEndVariableGroup(ctx);
    }

    {
        DebugBeginVariableGroup(ctx, "Render");
        DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_FountainForceDisplay);
        DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_RenderEntitySpace);
        DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_ShowVariableConfigOutput);
        {
            DebugBeginVariableGroup(ctx, "Chunks");
            DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_GroundChunkOutlines);
            DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_CheckerChunks);
            DebugEndVariableGroup(ctx);
        }
        DEBUG_GLOBAL_VARIABLE_LINE_b32(DEBUGUI_ShowLightingSamples);
        
        DebugEndVariableGroup(ctx);
    }

    DebugEndVariableGroup(ctx);

#undef DEBUG_GLOBAL_VARIABLE_LINE_b32
#undef DEBUG_GLOBAL_VARIABLE_LINE_s32
#undef DEBUG_GLOBAL_VARIABLE_LINE_u32
#undef DEBUG_GLOBAL_VARIABLE_LINE_r32

}

#endif