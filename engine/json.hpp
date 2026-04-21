// We'll use a minimal JSON helper instead of nlohmann to keep it simple
// This is a lightweight JSON builder/parser for our engine's stdin/stdout protocol

#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace json {

class Value;
using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

class Value {
public:
    enum Type { NUL, BOOL, INT, FLOAT, STRING, ARRAY, OBJECT };
    
    Value() : type_(NUL) {}
    Value(bool b) : type_(BOOL), bool_(b) {}
    Value(int i) : type_(INT), int_(i) {}
    Value(int64_t i) : type_(INT), int_(i) {}
    Value(double d) : type_(FLOAT), float_(d) {}
    Value(const char* s) : type_(STRING), str_(s) {}
    Value(const std::string& s) : type_(STRING), str_(s) {}
    Value(const Array& a) : type_(ARRAY), arr_(a) {}
    Value(const Object& o) : type_(OBJECT), obj_(o) {}
    
    Type type() const { return type_; }
    
    bool asBool() const { return bool_; }
    int64_t asInt() const { return int_; }
    double asFloat() const { 
        if (type_ == INT) return (double)int_;
        return float_; 
    }
    const std::string& asString() const { return str_; }
    const Array& asArray() const { return arr_; }
    const Object& asObject() const { return obj_; }
    Array& asArray() { return arr_; }
    Object& asObject() { return obj_; }
    
    bool has(const std::string& key) const {
        return type_ == OBJECT && obj_.find(key) != obj_.end();
    }
    
    const Value& operator[](const std::string& key) const {
        static Value null_val;
        if (type_ != OBJECT) return null_val;
        auto it = obj_.find(key);
        if (it == obj_.end()) return null_val;
        return it->second;
    }
    
    Value& operator[](const std::string& key) {
        if (type_ != OBJECT) { type_ = OBJECT; obj_.clear(); }
        return obj_[key];
    }
    
    const Value& operator[](size_t idx) const {
        return arr_[idx];
    }
    
    size_t size() const {
        if (type_ == ARRAY) return arr_.size();
        if (type_ == OBJECT) return obj_.size();
        return 0;
    }
    
    void push_back(const Value& v) {
        if (type_ != ARRAY) { type_ = ARRAY; arr_.clear(); }
        arr_.push_back(v);
    }
    
    std::string dump() const {
        std::ostringstream ss;
        write(ss);
        return ss.str();
    }
    
private:
    Type type_;
    bool bool_ = false;
    int64_t int_ = 0;
    double float_ = 0.0;
    std::string str_;
    Array arr_;
    Object obj_;
    
    void write(std::ostringstream& ss) const {
        switch (type_) {
            case NUL: ss << "null"; break;
            case BOOL: ss << (bool_ ? "true" : "false"); break;
            case INT: ss << int_; break;
            case FLOAT: {
                ss << std::fixed;
                // Remove trailing zeros
                std::ostringstream tmp;
                tmp << float_;
                ss << tmp.str();
                break;
            }
            case STRING: {
                ss << "\"";
                for (char c : str_) {
                    if (c == '"') ss << "\\\"";
                    else if (c == '\\') ss << "\\\\";
                    else if (c == '\n') ss << "\\n";
                    else if (c == '\r') ss << "\\r";
                    else if (c == '\t') ss << "\\t";
                    else ss << c;
                }
                ss << "\"";
                break;
            }
            case ARRAY: {
                ss << "[";
                for (size_t i = 0; i < arr_.size(); i++) {
                    if (i > 0) ss << ",";
                    arr_[i].write(ss);
                }
                ss << "]";
                break;
            }
            case OBJECT: {
                ss << "{";
                bool first = true;
                for (auto& pair : obj_) {
                    const std::string& k = pair.first;
                    const Value& v = pair.second;
                    if (!first) ss << ",";
                    first = false;
                    ss << "\"" << k << "\":";
                    v.write(ss);
                }
                ss << "}";
                break;
            }
        }
    }
};

// Simple JSON parser
class Parser {
public:
    static Value parse(const std::string& input) {
        Parser p(input);
        return p.parseValue();
    }
    
private:
    const std::string& src_;
    size_t pos_ = 0;
    
    Parser(const std::string& s) : src_(s) {}
    
    void skipWhitespace() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\n' || src_[pos_] == '\r' || src_[pos_] == '\t'))
            pos_++;
    }
    
    char peek() { skipWhitespace(); return pos_ < src_.size() ? src_[pos_] : 0; }
    char next() { skipWhitespace(); return pos_ < src_.size() ? src_[pos_++] : 0; }
    
    Value parseValue() {
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') return parseNull();
        return parseNumber();
    }
    
    Value parseObject() {
        next(); // {
        Object obj;
        if (peek() == '}') { next(); return Value(obj); }
        while (true) {
            std::string key = parseString().asString();
            next(); // :
            obj[key] = parseValue();
            char c = next();
            if (c == '}') break;
            // c == ','
        }
        return Value(obj);
    }
    
    Value parseArray() {
        next(); // [
        Array arr;
        if (peek() == ']') { next(); return Value(arr); }
        while (true) {
            arr.push_back(parseValue());
            char c = next();
            if (c == ']') break;
        }
        return Value(arr);
    }
    
    Value parseString() {
        next(); // opening "
        std::string s;
        while (pos_ < src_.size()) {
            char c = src_[pos_++];
            if (c == '"') break;
            if (c == '\\') {
                c = src_[pos_++];
                if (c == 'n') s += '\n';
                else if (c == 't') s += '\t';
                else if (c == 'r') s += '\r';
                else s += c;
            } else {
                s += c;
            }
        }
        return Value(s);
    }
    
    Value parseBool() {
        if (src_[pos_] == 't') { pos_ += 4; return Value(true); }
        pos_ += 5; return Value(false);
    }
    
    Value parseNull() { pos_ += 4; return Value(); }
    
    Value parseNumber() {
        skipWhitespace();
        size_t start = pos_;
        bool isFloat = false;
        if (src_[pos_] == '-') pos_++;
        while (pos_ < src_.size() && (std::isdigit(src_[pos_]) || src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E' || src_[pos_] == '+' || src_[pos_] == '-')) {
            if (src_[pos_] == '.' || src_[pos_] == 'e' || src_[pos_] == 'E') isFloat = true;
            pos_++;
        }
        std::string num = src_.substr(start, pos_ - start);
        if (isFloat) return Value(std::stod(num));
        return Value((int64_t)std::stoll(num));
    }
};

inline Value parse(const std::string& s) { return Parser::parse(s); }

} // namespace json
