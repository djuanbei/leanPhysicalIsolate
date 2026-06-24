#pragma once
#include <string>
#include <string_view>
#include <map>
#include <variant>
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <iomanip>

namespace leanffi {

struct JsonNull {};
struct JsonValue;

using JsonObject = std::map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;
using JsonStorage = std::variant<JsonNull, bool, double, std::string, JsonArray, JsonObject>;

struct JsonValue {
    JsonStorage storage;
    JsonValue() : storage(JsonNull{}) {}
    JsonValue(JsonNull) : storage(JsonNull{}) {}
    JsonValue(bool b) : storage(b) {}
    JsonValue(int i) : storage((double)i) {}
    JsonValue(long i) : storage((double)i) {}
    JsonValue(long long i) : storage((double)i) {}
    JsonValue(unsigned i) : storage((double)i) {}
    JsonValue(unsigned long i) : storage((double)i) {}
    JsonValue(unsigned long long i) : storage((double)i) {}
    JsonValue(double d) : storage(d) {}
    JsonValue(const char* s) : storage(std::string(s)) {}
    JsonValue(std::string s) : storage(std::move(s)) {}
    JsonValue(JsonArray a) : storage(std::move(a)) {}
    JsonValue(JsonObject o) : storage(std::move(o)) {}

    bool is_null()   const { return std::holds_alternative<JsonNull>(storage); }
    bool is_bool()   const { return std::holds_alternative<bool>(storage); }
    bool is_number() const { return std::holds_alternative<double>(storage); }
    bool is_string() const { return std::holds_alternative<std::string>(storage); }
    bool is_array()  const { return std::holds_alternative<JsonArray>(storage); }
    bool is_object() const { return std::holds_alternative<JsonObject>(storage); }

    double as_number() const { return std::get<double>(storage); }
    const std::string& as_string() const { return std::get<std::string>(storage); }
    bool as_bool() const { return std::get<bool>(storage); }
    const JsonArray& as_array() const { return std::get<JsonArray>(storage); }
    const JsonObject& as_object() const { return std::get<JsonObject>(storage); }
    JsonArray& as_array() { return std::get<JsonArray>(storage); }
    JsonObject& as_object() { return std::get<JsonObject>(storage); }

    bool contains(const std::string& k) const {
        if (!is_object()) return false;
        return as_object().count(k) > 0;
    }
    const JsonValue& at(const std::string& k) const {
        return as_object().at(k);
    }
    JsonValue& at(const std::string& k) {
        return as_object().at(k);
    }
};

std::string serialize(const JsonValue& v, bool pretty = false);
std::optional<JsonValue> parse(std::string_view text);

// Helpers
inline JsonValue obj() { return JsonValue(JsonObject{}); }
inline JsonValue arr() { return JsonValue(JsonArray{}); }

template <typename T>
JsonValue kv(const std::string& k, T&& v) {
    JsonObject o;
    o.emplace(k, JsonValue(std::forward<T>(v)));
    return JsonValue(std::move(o));
}

JsonObject& set(JsonValue& v, const std::string& k, JsonValue vv);
void set_arr(JsonValue& arr_v, JsonValue vv);

}