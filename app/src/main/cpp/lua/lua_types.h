#pragma once
/**
 * Luau/Lua type definitions for Roblox Android
 * Based on Lua 5.1 / Luau source with Roblox extensions
 */
#include <cstdint>
#include <cstddef>

// Lua basic types
#define LUA_TNONE          (-1)
#define LUA_TNIL           0
#define LUA_TBOOLEAN       1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER        3
#define LUA_TVECTOR        4
#define LUA_TSTRING        5
#define LUA_TTABLE         6
#define LUA_TFUNCTION      7
#define LUA_TUSERDATA      8
#define LUA_TTHREAD        9

// Lua call/yield status
#define LUA_OK             0
#define LUA_YIELD          1
#define LUA_ERRRUN         2
#define LUA_ERRSYNTAX      3
#define LUA_ERRMEM         4
#define LUA_ERRERR         5

// Max stack size
#define LUA_MAXSTACK       8000
#define LUA_REGISTRYINDEX  (-10000)
#define LUA_GLOBALSINDEX   (-10002)

typedef double lua_Number;
typedef int lua_Integer;
typedef unsigned int lua_Unsigned;

// Opaque lua_State — actual layout determined at runtime
struct lua_State;
struct global_State;

// C function type callable from Lua
typedef int (*lua_CFunction)(lua_State* L);

// Memory allocator type
typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);

// ── Function pointer typedefs ─────────────────────────────────────────────

// State management
typedef lua_State* (*lua_newstate_t)(lua_Alloc f, void* ud);
typedef lua_State* (*lua_newthread_t)(lua_State* L);
typedef void       (*lua_close_t)(lua_State* L);

// Script loading
typedef int (*luaL_loadbuffer_t)(lua_State* L, const char* buff, size_t sz, const char* name);
typedef int (*luaL_loadstring_t)(lua_State* L, const char* s);
typedef int (*luau_load_t)(lua_State* L, const char* chunkname, const char* data, size_t size, int env);

// Execution
typedef int (*lua_pcall_t)(lua_State* L, int nargs, int nresults, int errfunc);
typedef int (*lua_resume_t)(lua_State* L, int narg);

// Stack
typedef void        (*lua_pushstring_t)(lua_State* L, const char* s);
typedef void        (*lua_pushnumber_t)(lua_State* L, lua_Number n);
typedef void        (*lua_pushboolean_t)(lua_State* L, int b);
typedef void        (*lua_pushnil_t)(lua_State* L);
typedef void        (*lua_pushcclosure_t)(lua_State* L, lua_CFunction fn, const char* debugname, int n);
typedef const char* (*lua_tostring_t)(lua_State* L, int idx);
typedef lua_Number  (*lua_tonumber_t)(lua_State* L, int idx);
typedef int         (*lua_toboolean_t)(lua_State* L, int idx);
typedef int         (*lua_type_t)(lua_State* L, int idx);
typedef int         (*lua_gettop_t)(lua_State* L);
typedef void        (*lua_settop_t)(lua_State* L, int idx);

// Tables
typedef void (*lua_getfield_t)(lua_State* L, int idx, const char* k);
typedef void (*lua_setfield_t)(lua_State* L, int idx, const char* k);
typedef void (*lua_newtable_t)(lua_State* L);
typedef void (*lua_getglobal_t)(lua_State* L, const char* name);
typedef void (*lua_setglobal_t)(lua_State* L, const char* name);

// Errors
typedef int (*luaL_error_t)(lua_State* L, const char* fmt, ...);

// ── Roblox-specific ───────────────────────────────────────────────────────
// Thread identity setter (Roblox extension) — sets privilege level 0-8
typedef void (*SetThreadIdentity_t)(lua_State* L, int identity);
typedef int  (*GetThreadIdentity_t)(lua_State* L);
