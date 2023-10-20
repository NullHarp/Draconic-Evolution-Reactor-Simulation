extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#include <CraftOS-PC.hpp>

class reactorPeripheral : public peripheral {
    int add(lua_State* L) {
        lua_pushnumber(L, luaL_checknumber(L, 1) + luaL_checknumber(L, 2));
        return 1;
    }
    int subtract(lua_State* L) {
        lua_pushnumber(L, luaL_checknumber(L, 1) - luaL_checknumber(L, 2));
        return 1;
    }
    int ping(lua_State* L) {
        lua_pushstring(L, "pong");
        return 1;
    }
public:
    static library_t methods;
    reactorPeripheral(lua_State* L, const char* side) {}
    ~reactorPeripheral() {}
    int call(lua_State* L, const char* method) {
        std::string m(method);
        if (m == "add") return add(L);
        else if (m == "subtract") return subtract(L);
        else if (m == "ping") return ping(L);
        else return luaL_error(L, "No such method");
    }
    static peripheral* init(lua_State* L, const char* side) { return new reactorPeripheral(L, side); }
    static void deinit(peripheral* p) { delete (reactorPeripheral*)p; }
    virtual destructor getDestructor() const { return deinit; }
    void update() {}
    virtual library_t getMethods() const { return methods; }
};

static luaL_Reg methods_reg[] = {
    {"add", NULL},
    {"subtract", NULL},
    {"ping", NULL},
};
static PluginInfo info;
library_t reactorPeripheral::methods = { "reactorPeripheral", methods_reg, nullptr, nullptr };

extern "C" {
    DLLEXPORT PluginInfo* plugin_init(PluginFunctions* func, const path_t& path) {
        func->registerPeripheralFn("reactorPeripheral", &reactorPeripheral::init);
        return &info;
    }
}