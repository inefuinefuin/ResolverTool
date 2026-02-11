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

    explicit IntelliPtr(lua_State* t) : rawptr(t) {}

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
    lua_State* rawptr = nullptr;
};


template<typename __Type>
concept _Var_Ty = std::same_as<__Type,int>||std::same_as<__Type,double>||
				std::same_as<__Type,String>||std::same_as<__Type,bool>;
using var_tys = Variant<int, double, String, bool>;
struct Node {    
    HMap<String, var_tys> mulitdict;
    HMap<String, IntelliPtr<Node>> recnode;

    template<_Var_Ty T>
    Option<T> get(const char* key){
        auto it = mulitdict.find(key);
        if(it==mulitdict.end()) return std::nullopt;
        return std::get<T>(it->second);
    }
};


struct RWNode{
    RWNode() = default;
    RWNode(const RWNode&) = delete;
    RWNode& operator=(const RWNode&) = delete;
    RWNode(RWNode&& n): rt(std::move(n.rt)),cur(n.cur) {}
    RWNode& operator=(RWNode&& n) noexcept {
        if(this==&n) return *this;
        rt = std::move(n.rt);
        cur = n.cur;
        return *this;
    }

    Node& Ref() {
        return rt.RefRaw();
    }

    RWNode& node(const char* c) {
        auto incur = isrt ? rcur : cur;
        auto it = incur->recnode.find(c);
        if(it==incur->recnode.end()) {
            String err = "Error Node : " + String(c) + " Invalid Node";
            throw std::runtime_error(err);
        }
        if (isrt) { rcur = it->second.Raw(); }
        else { cur = it->second.Raw(); }
        return *this;
    }

    template<_Var_Ty T>
    Option<T> as(const char* c){
        auto incur = isrt ? rcur : cur;
        if(isrt){ 
            isrt = false;
            rcur = rt.Raw();
        }
        return incur->get<T>(c);
    }

    RWNode& reset() {
        cur = rt.Raw();
        return *this;
    }
    RWNode& root() {
        isrt = true;
        rcur = rt.Raw();
        return *this;
    }
private:
    IntelliPtr<Node> rt;
    Node* cur = rt.Raw();
    bool isrt = false;
    Node* rcur = nullptr;
};


void _lua_Parse(lua_State* _luaL,Node& node) {
    int offset = -1;
    lua_pushnil(_luaL);
    offset--;
    while(lua_next(_luaL,offset)){
        offset--;
        
        var_tys c; int idx = 0; String _cat;
        if(lua_isinteger(_luaL,-2)){
            idx = (int)lua_tointeger(_luaL,-2);
            _cat = std::to_string(idx)+"#i";

            if(lua_isinteger(_luaL,-1)){
                c = (int)lua_tointeger(_luaL,-1);
                node.mulitdict.insert({_cat,c});
            }else if(lua_isnumber(_luaL,-1)){
                c = (double)lua_tonumber(_luaL,-1);
                node.mulitdict.insert({_cat,c});
            }else if(lua_isboolean(_luaL,-1)){
                c = (bool)lua_toboolean(_luaL,-1);
                node.mulitdict.insert({_cat,c});
            }else if(lua_isstring(_luaL,-1)){
                c = String(lua_tostring(_luaL,-1));
                node.mulitdict.insert({_cat,c});
            }else if(lua_istable(_luaL,-1)){
                goto Recusion_lab;
            }else{
                throw std::runtime_error("Type Error");
            }

        }else if(lua_isstring(_luaL,-2)){
            _cat = lua_tostring(_luaL,-2);

            if(lua_isinteger(_luaL,-1)){
                c = (int)lua_tointeger(_luaL,-1);
                node.mulitdict.insert({_cat,c});
            }else if(lua_isnumber(_luaL,-1)){
                c = (double)lua_tonumber(_luaL,-1);
                node.mulitdict.insert({_cat,c});
            }else if(lua_isboolean(_luaL,-1)){
                c = (bool)lua_toboolean(_luaL,-1);
                node.mulitdict.insert({_cat,c});
            }else if(lua_isstring(_luaL,-1)){
                c = String(lua_tostring(_luaL,-1));
                node.mulitdict.insert({_cat,c});
            }else if(lua_istable(_luaL,-1)){
                goto Recusion_lab;
            }else {
                throw std::runtime_error("Type Error");
            }
            
        }else {
            throw std::runtime_error("Type Error");
        }

        lua_pop(_luaL,1);
        offset++;
        continue;
        
        Recusion_lab:
        if(idx!=0){
            node.recnode.insert({_cat,IntelliPtr<Node>()});
            _lua_Parse(_luaL,node.recnode[_cat].RefRaw());
        }else if(_cat!=""){
            node.recnode.insert({_cat,IntelliPtr<Node>()});
            _lua_Parse(_luaL,node.recnode[_cat].RefRaw());
        }else{
            throw std::runtime_error("Type Error");
        }
        lua_pop(_luaL,1);
        offset++;
    }
}




struct Resolver {
    Resolver() = default;
    Resolver(const char* f): _N(f) { 
        IntelliPtr<lua_State> State(luaL_newstate());
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

    RWNode& node(const char* c){
        return rt.node(c);
    }

    template<_Var_Ty T>
    Option<T> as(const char* c) {
        return rt.as<T>(c);
    }

    RWNode& reset() {
        return rt.reset();
    }

    RWNode& root(){
        return rt.root();
    }

    ~Resolver(){}
private:
    String _N;
    RWNode rt;
};


void testfunc() {
    auto r = Resolver("config.lua");
    RWNode& mid = r.node("config").node("app");
    auto mid2 = r.root().node("config").node("servers").node("1#i").as<String>("host");
    std::cout<<*mid2<<std::endl;

    std::cout<<*mid.as<String>("name")<<std::endl;

}
