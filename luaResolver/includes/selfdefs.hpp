#pragma once

#if defined(_GLIBCXX_STRING) || defined(_STRING_)
using String = std::string;
#endif

#if defined(_GLIBCXX_ARRAY) || defined(_ARRAY_)
template<typename __Type,size_t __Sz>
using Array = std::array<__Type,__Sz>;
#endif

#if defined(_GLIBCXX_VECTOR) || defined(_VECTOR_)
template<typename __Type>
using Vec = std::vector<__Type>;
#endif

#if defined(_GLIBCXX_MAP) || defined(_MAP_)
template<typename __Type,typename __Val>
using Map = std::map<__Type,__Val>;
#endif

#if defined(_GLIBCXX_UNORDERED_MAP) || defined(_UNORDERED_MAP_)
template<typename __Type,typename __Val>
using HMap = std::unordered_map<__Type,__Val>;
#endif

#if defined(_GLIBCXX_SET) || defined(_SET_)
template<typename __Type>
using Set = std::set<__Type>;
#endif

#if defined(_GLIBCXX_UNORDERED_SET) || defined(_UNORDERED_SET_)
template<typename __Type>
using HSet = std::unordered_set<__Type>;
#endif

#if defined(_GLIBCXX_VARIANT) || defined(_VARIANT_)
template<typename... __Type>
using Variant = std::variant<__Type...>;
#endif

#if defined(_GLIBCXX_OPTIONAL) || defined(_OPTIONAL_)
template<typename __Type>
using Option = std::optional<__Type>;
#endif



// unique-ptr
template<typename __Type>
struct IntelliPtr{
    __Type* rawptr = nullptr;

    template<typename... Args>
        requires std::is_constructible_v<__Type, Args...>
    IntelliPtr(Args&&... args) : rawptr(new __Type{std::forward<Args>(args)...}){}

    IntelliPtr(const IntelliPtr&) = delete;
    IntelliPtr& operator=(const IntelliPtr&) = delete;
    IntelliPtr(IntelliPtr&& IntelliPtr) : rawptr(IntelliPtr.rawptr) { IntelliPtr.rawptr = nullptr; }
    IntelliPtr& operator=(IntelliPtr&& IntelliPtr) noexcept {
        if(this==&IntelliPtr) return *this;
        rawptr = IntelliPtr.rawptr;
        IntelliPtr.rawptr = nullptr;
        return *this;
    } 

    __Type* Raw() { return rawptr; }
    __Type& RefRaw() { return *rawptr; }

    ~IntelliPtr(){
        delete rawptr;
    }
};

template<typename __Type, int N>
    requires (!std::same_as<__Type,void>)
struct IntelliPtr<__Type[N]> {
    __Type* rawptr = nullptr;
    
    template<typename... Args>
        requires std::is_constructible_v<__Type,Args...>
    IntelliPtr(Args&&... args): rawptr(new __Type[N]{std::forward<Args>(args)...}) {}

    IntelliPtr(const IntelliPtr&) = delete;
    IntelliPtr& operator=(const IntelliPtr&) = delete;
    IntelliPtr(IntelliPtr&& IntelliPtr) : rawptr(IntelliPtr.rawptr) { IntelliPtr.rawptr = nullptr; }
    IntelliPtr& operator=(IntelliPtr&& IntelliPtr) noexcept {
        if(this==&IntelliPtr) return *this;
        rawptr = IntelliPtr.rawptr;
        IntelliPtr.rawptr = nullptr;
        return *this;
    }

    __Type* Raw() { return rawptr; }

    ~IntelliPtr(){
        delete[] rawptr;
    }
};

template<>
struct IntelliPtr<void>{
    void* rawptr = nullptr;

    IntelliPtr() = default;

    explicit IntelliPtr(void* v) : rawptr(v) {}

    IntelliPtr(const IntelliPtr&) = delete;
    IntelliPtr& operator=(const IntelliPtr&) = delete;
    IntelliPtr(IntelliPtr&& IntelliPtr) : rawptr(IntelliPtr.rawptr) { IntelliPtr.rawptr = nullptr; }
    IntelliPtr& operator=(IntelliPtr&& IntelliPtr) noexcept {
        if(this==&IntelliPtr) return *this;
        rawptr = IntelliPtr.rawptr;
        IntelliPtr.rawptr = nullptr;
        return *this;
    } 

    void* Raw() { return rawptr; }

    ~IntelliPtr(){
        delete rawptr;
    }
};
