#include "requirements.h"
#include "validation.h"
#include "logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

namespace fs = std::filesystem;

namespace lpi {

Requirements& Requirements::get() {
    static Requirements r;
    return r;
}

std::vector<Requirement> Requirements::load_all() {
    std::vector<Requirement> out;
    if (!fs::is_directory(root_)) return out;
    for (auto& area_dir : fs::directory_iterator(root_)) {
        if (!area_dir.is_directory()) continue;
        std::string area = area_dir.path().filename().string();
        for (auto& f : fs::directory_iterator(area_dir.path())) {
            if (f.path().extension() != ".md") continue;
            std::ifstream in(f.path());
            std::stringstream ss; ss << in.rdbuf();
            std::string body = ss.str();
            Requirement r;
            r.area = area;
            r.path = f.path().string();
            r.body = body;
            r.id = area + "/" + f.path().stem().string();
            // Title = first markdown header.
            size_t pos = body.find("\n# ");
            if (pos != std::string::npos) {
                size_t eol = body.find('\n', pos + 1);
                r.title = body.substr(pos + 1, eol - pos - 1);
            } else {
                r.title = f.path().stem().string();
            }
            out.push_back(std::move(r));
        }
    }
    return out;
}

bool Requirements::evaluate(const Requirement& req) {
    // Inspect `check:` and `expect:` lines in the body.
    std::istringstream iss(req.body);
    std::string line, check, expect = "pass";
    while (std::getline(iss, line)) {
        // Trim leading whitespace.
        size_t a = 0;
        while (a < line.size() && std::isspace((unsigned char)line[a])) ++a;
        std::string t = line.substr(a);
        if (t.rfind("check:", 0) == 0)  check = t.substr(6);
        else if (t.rfind("expect:", 0) == 0) expect = t.substr(7);
    }
    // Trim leading whitespace from check name.
    size_t i = 0;
    while (i < check.size() && std::isspace((unsigned char)check[i])) ++i;
    check = check.substr(i);
    // Trim trailing whitespace too.
    while (!check.empty() && std::isspace((unsigned char)check.back())) check.pop_back();
    // Same for expect.
    i = 0;
    while (i < expect.size() && std::isspace((unsigned char)expect[i])) ++i;
    expect = expect.substr(i);
    while (!expect.empty() && std::isspace((unsigned char)expect.back())) expect.pop_back();

    if (check.empty()) return true;  // no check defined -> vacuous

    // Resolve repl path: the workspace dir is the parent of the executable
    // we are running. The workspace is /root/mycode/lean_physical_isolate.
    // We try a few common locations.
    std::string repl_path = "/root/mycode/lean_physical_isolate/repl";

    // Translate to a Validation::Result.
    Validation::Result vr;
    if (check == "semantic_correctness")      vr = Validation::semantic_correctness(repl_path);
    else if (check == "isolation_integrity")  vr = Validation::isolation_integrity();
    else if (check == "memory_check")         vr = Validation::memory_check();
    else if (check == "snapshot_correctness") vr = Validation::snapshot_correctness(repl_path);
    else if (check == "kernel_semantic_match")vr = Validation::kernel_semantic_match(repl_path);
    else if (check == "throughput")           vr = Validation::throughput_check(0, 0.0);
    else return false;

    bool satisfied = (expect == "pass") ? vr.ok : !vr.ok;
    return satisfied;
}

void Requirements::evaluate_all() {
    auto all = load_all();
    fs::create_directories("evidence");
    std::ofstream f("evidence/requirements.jsonl");
    for (auto& r : all) {
        r.satisfied = evaluate(r);
        f << "{\"id\":\"" << r.id << "\",\"area\":\"" << r.area
          << "\",\"title\":\"" << r.title
          << "\",\"satisfied\":" << (r.satisfied ? "true" : "false")
          << ",\"path\":\"" << r.path << "\"}\n";
        Logger::get().event(r.id, "requirement.eval", r.path,
                            r.satisfied ? "pass" : "fail", "{}");
    }
}

}  // namespace lpi
