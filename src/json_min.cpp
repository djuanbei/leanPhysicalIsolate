#include "json_min.h"
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace leanffi {

namespace {

void emit_string(std::ostream& os, const std::string& s) {
    os << '"';
    for (char c : s) {
        switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    os << buf;
                } else {
                    os << c;
                }
        }
    }
    os << '"';
}

void emit_indent(std::ostream& os, int depth) {
    for (int i = 0; i < depth; ++i) os << "  ";
}

void emit_value(std::ostream& os, const JsonValue& v, bool pretty, int depth) {
    if (v.is_null()) { os << "null"; return; }
    if (v.is_bool()) { os << (v.as_bool() ? "true" : "false"); return; }
    if (v.is_number()) {
        double d = v.as_number();
        if (std::isfinite(d)) {
            if (d == static_cast<long long>(d)) {
                os << static_cast<long long>(d);
            } else {
                std::ostringstream ss;
                ss << std::setprecision(15) << d;
                os << ss.str();
            }
        } else {
            os << "null";
        }
        return;
    }
    if (v.is_string()) { emit_string(os, v.as_string()); return; }
    if (v.is_array()) {
        const auto& a = v.as_array();
        if (a.empty()) { os << "[]"; return; }
        if (pretty) {
            os << "[\n";
            for (size_t i = 0; i < a.size(); ++i) {
                emit_indent(os, depth + 1);
                emit_value(os, a[i], pretty, depth + 1);
                os << (i + 1 < a.size() ? ",\n" : "\n");
            }
            emit_indent(os, depth);
            os << "]";
        } else {
            os << "[";
            for (size_t i = 0; i < a.size(); ++i) {
                emit_value(os, a[i], pretty, depth + 1);
                os << (i + 1 < a.size() ? "," : "");
            }
            os << "]";
        }
        return;
    }
    if (v.is_object()) {
        const auto& o = v.as_object();
        if (o.empty()) { os << "{}"; return; }
        if (pretty) {
            os << "{\n";
            size_t i = 0;
            for (const auto& [k, val] : o) {
                emit_indent(os, depth + 1);
                emit_string(os, k);
                os << ": ";
                emit_value(os, val, pretty, depth + 1);
                os << (++i < o.size() ? ",\n" : "\n");
            }
            emit_indent(os, depth);
            os << "}";
        } else {
            os << "{";
            size_t i = 0;
            for (const auto& [k, val] : o) {
                emit_string(os, k);
                os << ":";
                emit_value(os, val, pretty, depth + 1);
                os << (++i < o.size() ? "," : "");
            }
            os << "}";
        }
        return;
    }
    os << "null";
}

// ----- parser -----
struct Parser {
    std::string_view s;
    size_t pos = 0;

    explicit Parser(std::string_view sv) : s(sv) {}

    void skip_ws() {
        while (pos < s.size()) {
            char c = s[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++pos;
            else break;
        }
    }

    [[noreturn]] void fail(const std::string& m) {
        throw std::runtime_error("json: " + m + " at " + std::to_string(pos));
    }

    char peek() {
        skip_ws();
        if (pos >= s.size()) fail("unexpected end");
        return s[pos];
    }

    char get() {
        skip_ws();
        if (pos >= s.size()) fail("unexpected end");
        return s[pos++];
    }

    bool consume(char c) {
        skip_ws();
        if (pos < s.size() && s[pos] == c) { ++pos; return true; }
        return false;
    }

    void expect(char c) {
        skip_ws();
        if (pos >= s.size() || s[pos] != c) {
            fail(std::string("expected ") + c);
        }
        ++pos;
    }

    JsonValue parse_value();

    JsonValue parse_string() {
        expect('"');
        std::string out;
        while (pos < s.size()) {
            char c = s[pos++];
            if (c == '"') return JsonValue(std::move(out));
            if (c == '\\') {
                if (pos >= s.size()) fail("bad escape");
                char e = s[pos++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': {
                        if (pos + 4 > s.size()) fail("bad unicode");
                        unsigned cp = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = s[pos++];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= (h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                            else fail("bad hex");
                        }
                        if (cp < 0x80) {
                            out += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            out += static_cast<char>(0xc0 | (cp >> 6));
                            out += static_cast<char>(0x80 | (cp & 0x3f));
                        } else {
                            out += static_cast<char>(0xe0 | (cp >> 12));
                            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
                            out += static_cast<char>(0x80 | (cp & 0x3f));
                        }
                        break;
                    }
                    default: fail("bad escape char");
                }
            } else {
                out += c;
            }
        }
        fail("unterminated string");
    }

    JsonValue parse_number() {
        skip_ws();
        size_t start = pos;
        if (pos < s.size() && s[pos] == '-') ++pos;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        if (pos < s.size() && s[pos] == '.') {
            ++pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        }
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            ++pos;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        }
        std::string num(s.substr(start, pos - start));
        if (num.empty()) fail("expected number");
        return JsonValue(std::stod(num));
    }

    JsonValue parse_literal(const std::string& lit, JsonValue v) {
        skip_ws();
        if (pos + lit.size() > s.size()) fail("expected " + lit);
        if (s.substr(pos, lit.size()) != lit) fail("expected " + lit);
        pos += lit.size();
        return v;
    }

    JsonValue parse_array() {
        expect('[');
        JsonArray a;
        skip_ws();
        if (consume(']')) return JsonValue(std::move(a));
        while (true) {
            a.push_back(parse_value());
            skip_ws();
            if (consume(',')) continue;
            if (consume(']')) break;
            fail("expected , or ]");
        }
        return JsonValue(std::move(a));
    }

    JsonValue parse_object() {
        expect('{');
        JsonObject o;
        skip_ws();
        if (consume('}')) return JsonValue(std::move(o));
        while (true) {
            skip_ws();
            JsonValue k = parse_string();
            skip_ws();
            expect(':');
            o.emplace(k.as_string(), parse_value());
            skip_ws();
            if (consume(',')) continue;
            if (consume('}')) break;
            fail("expected , or }");
        }
        return JsonValue(std::move(o));
    }
};

JsonValue Parser::parse_value() {
    skip_ws();
    if (pos >= s.size()) fail("empty");
    char c = s[pos];
    if (c == '"') return parse_string();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == 't') return parse_literal("true", JsonValue(true));
    if (c == 'f') return parse_literal("false", JsonValue(false));
    if (c == 'n') return parse_literal("null", JsonValue());
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
    fail(std::string("unexpected ") + c);
}

} // anonymous

std::string serialize(const JsonValue& v, bool pretty) {
    std::ostringstream os;
    emit_value(os, v, pretty, 0);
    return os.str();
}

std::optional<JsonValue> parse(std::string_view text) {
    try {
        Parser p(text);
        JsonValue v = p.parse_value();
        p.skip_ws();
        if (p.pos != text.size()) return std::nullopt;
        return v;
    } catch (...) {
        return std::nullopt;
    }
}

JsonObject& set(JsonValue& v, const std::string& k, JsonValue vv) {
    if (!v.is_object()) {
        v = JsonValue(JsonObject{});
    }
    auto& o = v.as_object();
    auto it = o.find(k);
    if (it == o.end()) {
        o.emplace(k, std::move(vv));
        return o;
    }
    it->second = std::move(vv);
    return o;
}

void set_arr(JsonValue& arr_v, JsonValue vv) {
    if (!arr_v.is_array()) arr_v = JsonValue(JsonArray{});
    arr_v.as_array().push_back(std::move(vv));
}

} // namespace leanffi