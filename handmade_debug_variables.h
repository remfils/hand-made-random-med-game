#if !defined(HANDMADE_DEBUG_VARAIBLES_H)
#define HANDMADE_DEBUG_VARAIBLES_H

enum debug_global_variable_type
{
    DebugGlobalVariableType_b32,
    DebugGlobalVariableType_s32,
    DebugGlobalVariableType_u32,
    DebugGlobalVariableType_r32,
    DebugGlobalVariableType_Group,
    DebugGlobalVariableType_ProfileGraph,
    DebugGlobalVariableType_Bitmap,
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

struct debug_global_variable_reference
{
    debug_global_variable *Var;
    debug_global_variable_reference *Parent;
    debug_global_variable_reference *Next;
};

struct debug_global_variable_group
{
    b32 IsOpen;
    debug_global_variable_reference *FirstChild;
    debug_global_variable_reference *LastChild;
};

struct debug_global_variable_graph
{
    v2 Dim;
};

struct debug_global_variable_bitmap
{
    v2 Dim;
    bitmap_id Id;
};


struct debug_global_variable
{
    char *Name;
    debug_global_variable_type Type;

    b32 IsStored;

    union {
        b32 Bool;
        u32 Uint;
        s32 Int;
        r32 Real;
        debug_global_variable_group Group;
        debug_global_variable_graph Graph;
        debug_global_variable_bitmap Bitmap;
    };
};


struct debug_global_variable_context
{
    debug_state *DebugState;
    memory_arena *Arena;

    debug_global_variable_reference *Group;
    
};

internal debug_global_variable *
DebugAddUnreferencedVariable(debug_state *debugState, debug_global_variable_type type, char *name)
{
    debug_global_variable *result = PushStruct(&debugState->DebugArena, debug_global_variable);
    result->Name = (char *)PushCopy(&debugState->DebugArena, StringLength(name) + 1, name);
    result->Type = type;

    result->IsStored = true;
    if (type == DebugGlobalVariableType_ProfileGraph || type == DebugGlobalVariableType_Bitmap) {
        result->IsStored = false;
    }

    return result;
}

internal debug_global_variable_reference*
DebugAddVariableReference(debug_state *debugState, debug_global_variable_reference *groupRef, debug_global_variable *var)
{
    debug_global_variable_reference * result = PushStruct(&debugState->DebugArena, debug_global_variable_reference);
    result->Var = var;
    result->Next = 0;

    result->Parent = groupRef;
    debug_global_variable *group = result->Parent ? result->Parent->Var : 0;

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

internal debug_global_variable_reference*
DebugAddVariable_(debug_state *debugState, debug_global_variable_reference *groupRef, debug_global_variable_type type, char *name)
{
    debug_global_variable *var = DebugAddUnreferencedVariable(debugState, type, name);

    debug_global_variable_reference *result = DebugAddVariableReference(debugState, groupRef, var);
    
    return result;
}

internal debug_global_variable_reference*
DebugAddVariable_(debug_global_variable_context *ctx, debug_global_variable_type type, char *name)
{
    debug_global_variable *var = DebugAddUnreferencedVariable(ctx->DebugState, type, name);

    debug_global_variable_reference *result = DebugAddVariableReference(ctx->DebugState, ctx->Group, var);
    
    return result;
}

internal debug_global_variable_reference*
DebugAddVariable(debug_global_variable_context *ctx, char *name, b32 value)
{
    debug_global_variable_reference* result = DebugAddVariable_(ctx, DebugGlobalVariableType_b32, name);
    result->Var->Bool = value;
    return result;
}

internal debug_global_variable_reference*
DebugAddVariable(debug_global_variable_context *ctx, char *name, r32 value)
{
    debug_global_variable_reference* result = DebugAddVariable_(ctx, DebugGlobalVariableType_r32, name);
    result->Var->Real = value;
    return result;
}

internal debug_global_variable_reference*
DebugAddVariable(debug_global_variable_context *ctx, char *name, u32 value)
{
    debug_global_variable_reference* result = DebugAddVariable_(ctx, DebugGlobalVariableType_u32, name);
    result->Var->Uint = value;
    return result;
}

internal debug_global_variable_reference *
DebugNewVariableGroup(char *name, debug_global_variable_context *ctx, debug_state *debugState)
{
    debug_global_variable_reference* result = ctx
        ? DebugAddVariable_(ctx, DebugGlobalVariableType_Group, name)
        : DebugAddVariable_(debugState, 0, DebugGlobalVariableType_Group, name);

    result->Var->Group.IsOpen = false;
    result->Var->Group.FirstChild = result->Var->Group.LastChild = 0;
    return result;
}

internal void
DebugBeginVariableGroup(debug_global_variable_context *ctx, char *name)
{
    debug_global_variable_reference* group = DebugNewVariableGroup(name, ctx, 0);
    ctx->Group = group;
}

internal void
DebugEndVariableGroup(debug_global_variable_context *ctx)
{
    Assert(ctx->Group);
    ctx->Group = ctx->Group->Parent;
}

internal void
DEBUGInitVaraibles(debug_global_variable_context *ctx)
{
#define DEBUG_GLOBAL_VARIABLE_LINE_b32(name) DebugAddVariable(ctx, #name, (b32)name);
#define DEBUG_GLOBAL_VARIABLE_LINE_s32(name) DebugAddVariable(ctx, #name, (s32)name);
#define DEBUG_GLOBAL_VARIABLE_LINE_u32(name) DebugAddVariable(ctx, #name, (u32)name);
#define DEBUG_GLOBAL_VARIABLE_LINE_r32(name) DebugAddVariable(ctx, #name, (r32)name);

    DebugBeginVariableGroup(ctx, "Root");

    ctx->DebugState->RootVariable = ctx->Group;
    
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

#undef DEBUG_GLOBAL_VARIABLE_LINE_b32
#undef DEBUG_GLOBAL_VARIABLE_LINE_s32
#undef DEBUG_GLOBAL_VARIABLE_LINE_u32
#undef DEBUG_GLOBAL_VARIABLE_LINE_r32

}

#endif