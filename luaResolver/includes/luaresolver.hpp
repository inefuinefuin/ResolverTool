#pragma once


#include "../thrid_libs/lua_src/lua.hpp"

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include <utility>
#include <stdexcept>
#include <concepts>

#include "./selfdefs.hpp"

// lua_State Utmp
template<>
struct IntelliPtr<lua_State> {
    lua_State* rawptr = nullptr;

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
};


template<typename __Type>
concept _Var_Ty = std::same_as<__Type,int>||std::same_as<__Type,double>||
				std::same_as<__Type,String>||std::same_as<__Type,bool>;

using var_tys = Variant<int, double, String, bool>;

struct Node {
    // affix [sign]type to show type/ append to mulit-dict
    HMap<String, var_tys> mulitdict;
    // table to renode
    HMap<String, IntelliPtr<Node>> recnode;

    template<typename T> requires _Var_Ty<T>||std::same_as<T,Node*>
    Option<T> g_Val(String key){
        if constexpr(_Var_Ty<T>){
            auto it = mulitdict.find(key);
            if(it==mulitdict.end()) return std::nullopt;
            return std::get<T>(it->second);
        }else{
            auto it = recnode.find(key);
            if (it == recnode.end()) return std::nullopt;
            return it->second.Raw();
        }
    }
};


void _lua_Parse(lua_State* _luaL, String space ,Node& node,char gp,char ty) {
    if(gp==ty){
        throw std::runtime_error("Sign Error");
    }
    // root(_filename)
    int offset = -1;
    /*
                      root <<<---<<---<<---<<---<<---<<---<<---<<---<<--- +
        +--->>push --> nil --> key  --->>+--+-->> next == 0 -> pop -> nil +
        +<<--- pop <- value <- push <<---+

    */
    lua_pushnil(_luaL);
    offset--;
    // next == 0 -> pop key
    while(lua_next(_luaL,offset)){
        offset--;

        int idx = 0; String key = "";

        var_tys c; String _cat;
        if(lua_isinteger(_luaL,-2)){
            idx = (int)lua_tointeger(_luaL,-2);

            if(lua_istable(_luaL,-1)){
                goto Recusion_lab;
            }else if(lua_isinteger(_luaL,-1)){
                c = (int)lua_tointeger(_luaL,-1);
                _cat = space+gp+std::to_string(idx)+ty+"i";
                node.mulitdict.insert({_cat,c});
            }else if(lua_isnumber(_luaL,-1)){
                c = (double)lua_tonumber(_luaL,-1);
                _cat = space+gp+std::to_string(idx)+ty+"d";
                node.mulitdict.insert({_cat,c});
            }else if(lua_isboolean(_luaL,-1)){
                c = (bool)lua_toboolean(_luaL,-1);
                _cat = space+gp+std::to_string(idx)+ty+"b";
                node.mulitdict.insert({_cat,c});
            }else if(lua_isstring(_luaL,-1)){
                c = String(lua_tostring(_luaL,-1));
                _cat = space+gp+std::to_string(idx)+ty+"s";
                node.mulitdict.insert({_cat,c});
            }else{
                throw std::runtime_error("Type Error");
            }

        }else if(lua_isstring(_luaL,-2)){
            key = String(lua_tostring(_luaL,-2));

            if(lua_istable(_luaL,-1)){
                goto Recusion_lab;
            }else if(lua_isinteger(_luaL,-1)){
                c = (int)lua_tointeger(_luaL,-1);
                _cat = space+gp+key+ty+"i";
                node.mulitdict.insert({_cat,c});
            }else if(lua_isnumber(_luaL,-1)){
                c = (double)lua_tonumber(_luaL,-1);
                _cat = space+gp+key+ty+"d";
                node.mulitdict.insert({_cat,c});
            }else if(lua_isboolean(_luaL,-1)){
                c = (bool)lua_toboolean(_luaL,-1);
                _cat = space+gp+key+ty+"b";
                node.mulitdict.insert({_cat,c});
            }else if(lua_isstring(_luaL,-1)){
                c = String(lua_tostring(_luaL,-1));
                _cat = space+gp+key+ty+"s";
                node.mulitdict.insert({_cat,c});
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
            _cat = space+gp+std::to_string(idx)+ty+"t";
            String __cat = std::to_string(idx)+ty+"t";
            node.recnode.insert({_cat,IntelliPtr<Node>()});
            _lua_Parse(_luaL,__cat,node.recnode[_cat].RefRaw(),gp,ty);
        }else if(key!=""){
            _cat = space+gp+key+ty+"t";
            String __cat = key+ty+"t";
            node.recnode.insert({_cat,IntelliPtr<Node>()});
            _lua_Parse(_luaL,__cat,node.recnode[_cat].RefRaw(),gp,ty);
        }else{
            throw std::runtime_error("Type Error");
        }
        lua_pop(_luaL,1);
        offset++;
    }
}


template<typename T>
concept __Node_Parse_Ty = _Var_Ty<T>||std::same_as<T,Node*>;

String SpiltPrefix(String raw, char gp){
    auto pos = raw.find_first_of(gp);
    if(pos==String::npos) {
        throw std::runtime_error("Type Error");
    }
    return raw.substr(pos+1,raw.size()-pos-1);
}

// first find fistly ret
template<__Node_Parse_Ty T>
Option<T> _node_Parse(Node& node, String key,char gp,char ty) {
    bool parse_ty = [&]() -> bool {
        auto pos = key.find_first_of(ty);
        if(pos==String::npos) {
            throw std::runtime_error("Type Error");
        }
        char t = key[++pos];
        if(t=='t') return false;
        else return true;
    }();
   if(parse_ty){
        for(auto& [k,v] : node.mulitdict) {
            if(SpiltPrefix(k,gp)==key){
                return node.g_Val<T>(k);
            }
        }
        goto Recusion_lab;
   }else{
        for(auto& [k,n]: node.recnode){
            if(SpiltPrefix(k,gp)==key){
                return node.g_Val<T>(k);
            }
        }
        goto Recusion_lab;
   }

   Recusion_lab:
    for(auto& [k,n]: node.recnode){
        auto ret = _node_Parse<T>(n.RefRaw(),key,gp,ty);
        if(ret!=std::nullopt) return ret;
    }
    return std::nullopt;
}


Vec<String> SplitePath(String raw,char gp,char ty){
   Vec<String> v;
   String root = "root";
    while(raw!=""){
        auto pos = raw.find_first_of(gp);
        auto gppos = (pos!=String::npos ? pos : raw.size());

        String ls = raw.substr(0,gppos);
        String key = root+gp+ls; v.push_back(key);
        if(gppos==raw.size()) break;
        root = ls;

        String rs = raw.substr(gppos+1,raw.size()-gppos-1);
        auto typos = rs.find_first_of(ty);

        String rls = rs.substr(0,typos+2);
        String val = root+gp+rls; v.push_back(val);
        if(typos+3>=rs.size()) break;
        root = rls;
        raw = rs.substr(typos+3,rs.size()-typos-2);
   }
   return v;
}

// abs path ret
template<__Node_Parse_Ty T>
Option<T> _node_Parse_Abs(Node& node, String path,char gp,char ty){
    auto intable = [](Node& node,String& s,bool tail)->size_t{
        auto it = node.mulitdict.find(s);
        if(it==node.mulitdict.end()) goto node_lab;
        return 0x01;

        node_lab:
        auto it_ = node.recnode.find(s);
        if(it_==node.recnode.end()) return 0x00;
        if(tail) return 0x01;
        return 0x02;
    };
    Vec<String> v = std::move(SplitePath(path,gp,ty));

    size_t idx = 0;
    Node* track = &node;
    while(idx!=v.size()){
        bool tail = (idx == v.size() - 1);
        size_t lab = intable(*track,v[idx++],tail);
        if(lab==0x00) return std::nullopt;
        if(lab==0x01) return track->g_Val<T>(v[idx-1]);
        track = track->recnode[v[idx-1]].Raw();
    }
}




struct Resolver {
    Resolver() = default;
    Resolver(const char* f): _N(f), State(IntelliPtr<lua_State>(luaL_newstate())) {
        auto _luaL = State.Raw();

        // luaL_openlibs(_luaL);
        // state +1(root)
        auto ok = luaL_dofile(_luaL,f);
        if(ok!=LUA_OK || !lua_istable(_luaL,1)){
            throw std::runtime_error("Format Error");
        }else{
            // Parse Table
            _lua_Parse(_luaL,"root",root.RefRaw(),sign[0],sign[1]);
        }
    }
    Resolver(const Resolver&) = delete;
    Resolver& operator=(const Resolver&) = delete;
    Resolver(Resolver&& r) : _N(r._N), State(std::move(r.State)), root(std::move(r.root)) {}
    Resolver& operator=(Resolver&& r) noexcept {
        if(this==&r) return *this;
        _N = r._N;
        State = std::move(r.State);
        root = std::move(r.root);
        return *this;
    }

    template<char gp,char ty>
    constexpr void setSign() {
        static_assert(gp!=ty && gp!='_' && ty!='_');
        sign[0] = gp; sign[1] = ty;
    }
    template<__Node_Parse_Ty T>
    Option<T> get(const char* key){
        return _node_Parse<T>(root.RefRaw(),String(key),sign[0],sign[1]);
    }

    template<__Node_Parse_Ty T>
    Option<T> absGet(const char* p) {
       return _node_Parse_Abs<T>(root.RefRaw(),String(p),sign[0],sign[1]);
    }

    Node& RefRaw(){ return *root.Raw(); }

    ~Resolver(){}
private:
    String _N;
    IntelliPtr<lua_State> State;
    IntelliPtr<Node> root;
    char sign[2] = {'>','#'};
};

