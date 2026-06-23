#include "json_helpers.h"

namespace lpi::json {

static std::string trim_ws(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b - a);
}

// Find the colon separating key from value, honoring string quoting.
static size_t find_colon(const std::string& s, size_t from) {
    bool in_str = false, esc = false;
    for (size_t i = from; i < s.size(); ++i) {
        char c = s[i];
        if (esc) { esc = false; continue; }
        if (c == '\\' && in_str) { esc = true; continue; }
        if (c == '"') { in_str = !in_str; continue; }
        if (!in_str && c == ':') return i;
    }
    return std::string::npos;
}

static size_t find_comma_or_end(const std::string& s, size_t from) {
    bool in_str = false, esc = false;
    int depth = 0;
    for (size_t i = from; i < s.size(); ++i) {
        char c = s[i];
        if (esc) { esc = false; continue; }
        if (c == '\\' && in_str) { esc = true; continue; }
        if (c == '"') { in_str = !in_str; continue; }
        if (!in_str) {
            if (c == '{' || c == '[') ++depth;
            else if (c == '}' || c == ']') --depth;
            else if (c == ',' && depth == 0) return i;
            if (depth < 0) return i;
        }
    }
    return s.size();
}

std::string escape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    o += buf;
                } else {
                    o += c;
                }
        }
    }
    return o;
}

std::optional<std::string> get_string(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return std::nullopt;
    size_t colon = find_colon(json, pos + needle.size());
    if (colon == std::string::npos) return std::nullopt;
    size_t i = colon + 1;
    while (i < json.size() && std::isspace((unsigned char)json[i])) ++i;
    if (i >= json.size() || json[i] != '"') return std::nullopt;
    ++i;
    std::string out;
    while (i < json.size() && json[i] != '"') {
        if (json[i] == '\\' && i + 1 < json.size()) {
            char nx = json[i+1];
            switch (nx) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '"': out += '"';  break;
                case '\\': out += '\\'; break;
                default:   out += nx;
            }
            i += 2;
        } else {
            out += json[i++];
        }
    }
    return out;
}

std::optional<std::string> get_object(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return std::nullopt;
    size_t colon = find_colon(json, pos + needle.size());
    if (colon == std::string::npos) return std::nullopt;
    size_t i = colon + 1;
    while (i < json.size() && std::isspace((unsigned char)json[i])) ++i;
    if (i >= json.size()) return std::nullopt;
    if (json[i] == '{' || json[i] == '[') {
        char open = json[i];
        char close = (open == '{') ? '}' : ']';
        int depth = 0;
        bool in_str = false, esc = false;
        size_t start = i;
        for (; i < json.size(); ++i) {
            char c = json[i];
            if (esc) { esc = false; continue; }
            if (c == '\\' && in_str) { esc = true; continue; }
            if (c == '"') { in_str = !in_str; continue; }
            if (!in_str) {
                if (c == open) ++depth;
                else if (c == close) { --depth; if (depth == 0) return json.substr(start, i - start + 1); }
            }
        }
    }
    return std::nullopt;
}

std::optional<bool> get_bool(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return std::nullopt;
    size_t colon = find_colon(json, pos + needle.size());
    if (colon == std::string::npos) return std::nullopt;
    size_t i = colon + 1;
    while (i < json.size() && std::isspace((unsigned char)json[i])) ++i;
    if (json.compare(i, 4, "true") == 0)  return true;
    if (json.compare(i, 5, "false") == 0) return false;
    return std::nullopt;
}

std::string make_request(const std::string& cmd, const std::string& payload_json) {
    return std::string("{\"cmd\":\"") + escape(cmd) + "\",\"payload\":" + payload_json + "}";
}

}  // namespace lpi::json
