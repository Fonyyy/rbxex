#pragma once
#include <string>
#include "lua_types.h"

namespace Executor {

struct ExecuteResult {
    bool success;
    std::string error;
};

// Called by the luau_execute hook when a lua_State is captured
void CaptureState(lua_State* L);

// Execute a Lua script string on the captured state
ExecuteResult Execute(const std::string& script);

// Check if executor is ready (has a valid lua_State)
bool IsReady();

// Get current status string
std::string GetStatus();

} // namespace Executor
