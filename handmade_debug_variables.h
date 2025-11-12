#if !defined(HANDMADE_DEBUG_VARAIBLES_H)
#define HANDMADE_DEBUG_VARAIBLES_H

enum debug_global_variable_type
{
    DebugGlobalVariableType_Boolean,
    DebugGlobalVariableType_Group,
    DebugGlobalVariableType_Count,
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
    result->Name = name;
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
    debug_global_variable* result = DebugAddVariable_(ctx, DebugGlobalVariableType_Boolean, name);
    result->Bool = value;
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

#define DEBUG_GLOBAL_VARIABLE_LINE(name) DebugAddVariable(ctx, #name, name);

    DebugBeginVariableGroup(ctx, "Root");

    debugState->RootVariable = ctx->Group;
    
    {
        DebugBeginVariableGroup(ctx, "Camera");
        DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_UseDebugCamera);
        DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_CameraIsTileBased);
        DebugEndVariableGroup(ctx);
    }

    {
        DebugBeginVariableGroup(ctx, "Render");
        DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_FountainForceDisplay);
        DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_RenderEntitySpace);
        {
            DebugBeginVariableGroup(ctx, "Chunks");
            DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_GroundChunkOutlines);
            DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_CheckerChunks);
            DebugEndVariableGroup(ctx);
        }
        DEBUG_GLOBAL_VARIABLE_LINE(DEBUGUI_ShowLightingSamples);
        
        DebugEndVariableGroup(ctx);
    }

    DebugEndVariableGroup(ctx);

#undef DEBUG_GLOBAL_VARIABLE_LINE
}

#endif