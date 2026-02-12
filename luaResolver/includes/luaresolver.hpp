#pragma once


#include "src/lua.hpp"

#include <variant>
#include <string>
#include <unordered_map>
#include <optional>

#include <utility>
#include <stdexcept>
#include <concepts>

#include "selfdefs.hpp"

#include <iostream>


// lua_State Utmp
template<>
struct IntelliPtr<lua_State> {
    IntelliPtr() = default;

    IntelliPtr(const IntelliPtr&) = delete;
    IntelliPtr& operator=(const IntelliPtr&) = delete;
    IntelliPtr(IntelliPtr&& IntelliPtr) : rawptr(IntelliPtr.rawptr) { IntelliPtr.rawptr = nullptr; }
    IntelliPtr& operator=(IntelliPtr&& IntelliPtr) noexcept {
        if (this == &IntelliPtr) return *this;
        rawptr = IntelliPtr.rawptr;
        IntelliPtr.rawptr = nullptr;
        return *this;
    }

    lua_State* Raw() { return rawptr; }
    lua_State& RefRaw() { return *rawptr; }

    ~IntelliPtr() {
        if (rawptr) lua_close(rawptr);
    }
private:
    lua_State* rawptr = luaL_newstate();
};


template<typename __Type>
concept _Var_Ty = std::same_as<__Type,int>||std::same_as<__Type,double>||
				std::same_as<__Type,String>||std::same_as<__Type,bool>;

template<typename __Type>
concept _Key_Ty = std::same_as<__Type,int>||std::same_as<__Type,String>;

using var_tys = Variant<int, double, String, bool>;
using key_tys = Variant<String,int>;

struct Node {    
    HMap<key_tys, var_tys> mulitdict;
    HMap<key_tys, IntelliPtr<Node>> recnode;

    template<_Var_Ty T>
    Option<T> get(auto key) const {
        auto it = mulitdict.find(key);
        if(it==mulitdict.end()) return std::nullopt;
        return std::get<T>(it->second);
    }
};



struct rCursor {
    rCursor(const rCursor&) = delete;
    rCursor& operator=(const rCursor&) = delete;
    rCursor(rCursor&& s): rt(s.rt) { s.rt = nullptr; }
    rCursor& operator=(rCursor&& s) noexcept {
        if(this==&s) return *this;
        rt = s.rt;
        s.rt = nullptr;
        return *this; 
    }
    
    rCursor& node(auto c){
        auto it = rt->recnode.find(c);
        if(it==rt->recnode.end()) {
            throw std::runtime_error("Invalid Node");
        }
        rt = it->second.Raw();
        return *this;
    }

    template<_Var_Ty T>
    Option<T> as(auto c) const {
        return rt->get<T>(c);
    }

    rCursor(Node* r): rt(r) {}
private:
    Node* rt;
};

struct sCursor {
    sCursor(const sCursor&) = delete;
    sCursor& operator=(const sCursor&) = delete;
    sCursor(sCursor&& s): bsptr(s.bsptr), trptr(s.trptr), history(s.history) { 
        s.bsptr = nullptr; 
        s.trptr = nullptr;
        s.history = nullptr; 
    }
    sCursor& operator=(sCursor&& s) noexcept {
        if(this==&s) return *this;
        bsptr = s.bsptr; 
        trptr = s.trptr;
        history = s.history;
        s.bsptr = nullptr; 
        s.trptr = nullptr;
        s.history = nullptr;
        return *this; 
    }

    sCursor& node(auto c) {
        auto it = trptr->recnode.find(c);
        if(it==trptr->recnode.end()) {
            throw std::runtime_error("Invalid Node");
        }
        trptr = it->second.Raw();
        return *this;
    }
    
    
    template<_Var_Ty T>
    Option<T> as(auto c) {
        Node* incur = trptr;
        trptr = bsptr;
        return incur->get<T>(c);
    }

    sCursor& recover() { trptr = bsptr; return *this; }
    sCursor& update() { history = bsptr; bsptr = trptr; return *this; }
    sCursor& cancel() { 
        bsptr = history;
        trptr = history; 
        history = nullptr; 
        return *this;
    }

    sCursor(Node* b): bsptr(b), trptr(b) {}
private:
    Node* history;
    Node* bsptr; 
    Node* trptr; 
};

struct RWNode{
    RWNode() = default;
    RWNode(const RWNode&) = delete;
    RWNode& operator=(const RWNode&) = delete;

    RWNode(RWNode&& n): rt(std::move(n.rt)), cur(n.cur) {}
    RWNode& operator=(RWNode&& n) noexcept {
        if(this==&n) return *this;
        rt = std::move(n.rt);
        cur = n.cur;
        return *this;
    }

    Node& Ref() { return rt.RefRaw(); }

    RWNode& node(auto c){
        auto it = cur->recnode.find(c);
        if(it==cur->recnode.end()) {
            throw std::runtime_error("Invalid Node");
        }
        cur = it->second.Raw();
        return *this;
    }

    template<_Var_Ty T>
    Option<T> as(auto c) {
        Node* incur = cur; 
        cur = rt.Raw();
        return incur->get<T>(c);
    }

    rCursor root() { return rCursor(rt.Raw()); }
    sCursor state() { return sCursor(cur); }

    RWNode& reset() { cur = rt.Raw(); return *this; }

private:
    IntelliPtr<Node> rt;
    Node* cur = rt.Raw();
};

void _lua_Parse(lua_State*,Node&);

struct Resolver {
    Resolver() = default;
    Resolver(const char* f): _N(f) { 
        IntelliPtr<lua_State> State;
        auto _luaL = State.Raw();
        auto ok = luaL_dofile(_luaL,f);

        if(ok!=LUA_OK || !lua_istable(_luaL,1)){
            throw std::runtime_error("Format Error");
        }else{
            _lua_Parse(_luaL,rt.Ref());
        }
    }
    Resolver(const Resolver&) = delete;
    Resolver& operator=(const Resolver&) = delete;
    Resolver(Resolver&& r) : _N(r._N), rt(std::move(r.rt)) {}
    Resolver& operator=(Resolver&& r) noexcept {
        if(this==&r) return *this;
        _N = r._N;
        rt = std::move(r.rt);
        return *this;
    }
    
    RWNode& node(auto c) { return rt.node(c); }
    RWNode& reset() { return rt.reset(); }
    
    rCursor root() { return rt.root(); }
    sCursor state() { return rt.state(); }

    template<_Var_Ty T>
    Option<T> as(auto c) { return rt.as<T>(c); }

    ~Resolver(){}
private:
    String _N;
    RWNode rt;
};


// root -> state -> default
void testfunc() {
    auto r = Resolver("config.lua");
    
    RWNode& mid = r.node("config").node("app");
    auto mid2 = r.root().node("config").node("servers").node(1).as<String>("host");
    std::cout<<*mid2<<std::endl;
    std::cout<<*mid.as<String>("name")<<std::endl;

    auto state = r.node("config").node("app").state();
    r.reset();

    std::cout<<*state.node("nested").as<int>("a")<<std::endl;
    state.node("list_of_maps").update();
    std::cout<<*state.node(1).as<String>("name")<<":"<<*state.node(2).as<String>("name")<<std::endl;
    state.cancel();

    std::cout<<*state.node("nested").node("b").as<String>("s") << std::endl;
    state.recover();
    
    std::cout<<*r.node("config").as<String>("name")<<std::endl;
    std::cout<<*r.node(1).as<int>(1)<<std::endl;
}




void _lua_Parse(lua_State* _luaL,Node& node) {
    int offset = lua_gettop(_luaL);
    lua_pushnil(_luaL);
    while(lua_next(_luaL,offset)){

        var_tys c; key_tys _cat;

        auto func = [&](key_tys _cat) {
            if (lua_isinteger(_luaL, -1)) {
                node.mulitdict[_cat] = (int)lua_tointeger(_luaL, -1);
            }
            else if (lua_isnumber(_luaL, -1)) {
                node.mulitdict[_cat] = (double)lua_tonumber(_luaL, -1);
            }
            else if (lua_isboolean(_luaL, -1)) {
                node.mulitdict[_cat] = (bool)lua_toboolean(_luaL, -1);
            }
            else if (lua_isstring(_luaL, -1)) {
                node.mulitdict[_cat] = String(lua_tostring(_luaL, -1));
            }
            else if (lua_istable(_luaL, -1)) {
                node.recnode.insert({ _cat,IntelliPtr<Node>() });
                _lua_Parse(_luaL, node.recnode[_cat].RefRaw());
            }
            else {
                throw std::runtime_error("Type Error");
            }
        };

        if(lua_isinteger(_luaL,-2)){
            _cat = (int)lua_tointeger(_luaL,-2);
            func(_cat);
        }else if(lua_isstring(_luaL,-2)){
            _cat = lua_tostring(_luaL, -2);
            func(_cat);
        }else {
            throw std::runtime_error("Type Error");
        }

        lua_pop(_luaL,1);
    }
}