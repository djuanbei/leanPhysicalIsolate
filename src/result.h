#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>

namespace lpi {

struct Diag {
    std::string severity;  // "error" | "warning" | "info"
    std::string message;
    std::optional<std::string> pos;
};

struct Result {
    bool ok = false;
    std::vector<Diag> diagnostics;
    std::string raw_json;
    std::string error;

    static Result success(std::string raw) {
        Result r; r.ok = true; r.raw_json = std::move(raw); return r;
    }
    static Result failure(std::string err, std::vector<Diag> diags = {}) {
        Result r; r.ok = false; r.error = std::move(err); r.diagnostics = std::move(diags); return r;
    }
};

}  // namespace lpi
