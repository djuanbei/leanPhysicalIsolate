#pragma once
#include <string>
#include <vector>
#include <optional>

namespace lpi {

// Minimal, dependency-free JSON helpers tailored to Pantograph's output
// (compact one-line JSON, no streaming). We avoid nlohmann/json to keep
// the binary tiny and the build hermetic.
namespace json {

// Escape a string for inclusion in a JSON string literal.
std::string escape(const std::string& s);

// Find the value for a top-level string field in a flat JSON object.
// Returns std::nullopt if the field is missing or not a string.
std::optional<std::string> get_string(const std::string& json, const std::string& key);

// Get a sub-object string by its raw content.
std::optional<std::string> get_object(const std::string& json, const std::string& key);

// Top-level bool field. Returns std::nullopt if missing/non-bool.
std::optional<bool> get_bool(const std::string& json, const std::string& key);

// Build a request of form { "cmd": "...", "payload": {...} } or null payload.
std::string make_request(const std::string& cmd, const std::string& payload_json = "null");

}  // namespace json
}  // namespace lpi
