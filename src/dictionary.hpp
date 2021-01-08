#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include "variant.hpp"

#include <iostream>
#include <exception>
#include <vector>
#include <unordered_map>

namespace wraper {
typedef mpark::variant<mpark::monostate, int64_t, double, bool, std::string> arraylessVariant;
// This type is nutty!
typedef mpark::variant<mpark::monostate, int64_t, double, bool, std::string, std::vector<arraylessVariant>> variant;
}

struct Variant: public wraper::variant {
    using wraper::variant::variant;
    using Array = std::vector<Variant>;

    enum Type {NIL, INT, FLOAT, BOOL, STRING, ARRAY};

    Variant(const wraper::arraylessVariant o) {
        if(mpark::holds_alternative<int64_t>(o)) *this = mpark::get<int64_t>(o);
        else if(mpark::holds_alternative<double>(o)) *this = mpark::get<double>(o);
        else if(mpark::holds_alternative<bool>(o)) *this = mpark::get<bool>(o);
        else if(mpark::holds_alternative<std::string>(o)) *this = mpark::get<std::string>(o);
        else *this = mpark::monostate{};
    }
    Variant(const int o) : wraper::variant(int64_t(o)) {}
    Variant(const float o) : wraper::variant(double(o)) {}
    Variant(const Array arr) { *this = arr; }
    Variant(const std::initializer_list<Variant> list) { *this = list; }
    Variant(const char* s) : wraper::variant(std::string(s)) {}


    Variant& operator= (const char* s) { return *this = std::string(s); }
    Variant& operator= (const Array arr) {
        std::vector<wraper::arraylessVariant> _new;
        _new.reserve(arr.size());
        for(const Variant& cur: arr) _new.push_back(cur);
        return *this = _new;
    }
    Variant& operator= (const std::initializer_list<Variant> list) {
        std::vector<wraper::arraylessVariant> _new;
        _new.reserve(list.size());
        for(Variant cur: list) _new.push_back(cur);
        return *this = _new;
    }


    Type getType() const {
        if(mpark::holds_alternative<int64_t>(*this)) return INT;
        else if(mpark::holds_alternative<double>(*this)) return FLOAT;
        else if(mpark::holds_alternative<bool>(*this)) return BOOL;
        else if(mpark::holds_alternative<std::string>(*this)) return STRING;
        else if(mpark::holds_alternative<std::vector<wraper::arraylessVariant>>(*this)) return ARRAY;
        return NIL;
    }

    std::string getString() const { return mpark::get<std::string>(*this); }
    Array getArray() const {
        const std::vector<wraper::arraylessVariant>& arrayless = mpark::get<std::vector<wraper::arraylessVariant>>(*this);
        Array out;
        out.reserve(arrayless.size());
        for (const wraper::arraylessVariant& cur: arrayless) out.push_back(cur);
        return out;
    }

    template <typename T>
    operator T() const { return mpark::get<T>(*this); }
    operator int() const { return (int) int64_t(*this); }
    operator char() const { return int(*this); }
    operator float() const { return (float) double(*this); }
    operator Array() const { return getArray(); }

    operator wraper::arraylessVariant() const {
        switch(getType()){
        case INT: return (int64_t) *this;
        case FLOAT: return (double) *this;
        case BOOL: return (bool) *this;
        case STRING: return getString();
        default: return mpark::monostate{};
        }
    }

public:
    Variant operator[] (const size_t index) const {
        if(mpark::holds_alternative<std::string>(*this)) return getString()[index];
        else if(mpark::holds_alternative<std::vector<wraper::arraylessVariant>>(*this)) return getArray()[index];
        throw mpark::bad_variant_access();
    }
};

std::string to_string(const Variant::Type t){
    switch(t){
    case Variant::INT: return "INT";
    case Variant::FLOAT: return "FLOAT";
    case Variant::BOOL: return "BOOL";
    case Variant::STRING: return "STRING";
    case Variant::ARRAY: return "ARRAY";
    default: return "NULL";
    }
}

std::ostream& operator<< (std::ostream& s, const Variant v){
    const Variant::Type t = v.getType();
    s << "{" << to_string(t) << ": ";

    switch(t){
    case Variant::INT: s << (int64_t) v; break;
    case Variant::FLOAT: s << (double) v; break;
    case Variant::BOOL: s << (bool) v; break;
    case Variant::STRING: s << v.getString(); break;
    case Variant::ARRAY: s << v.getArray().size() << "elems"; break;
    default: s << "\b\b";
    }

    return s << "}";
}

struct Dictionary: public std::unordered_map<std::string, Variant> {
    using std::unordered_map<std::string, Variant>::unordered_map;

    bool has(const std::string needle){ return find(needle) != end(); }
};


#endif /* end of include guard: __DICTIONARY_H__ */
