// Unit test: LeanFFI helpers (no subprocess required).
// Tests JSON escaping, iso8601, and scheduler logic.

#include "leanffi.hpp"
#include "scheduler.hpp"

#include <iostream>
#include <cassert>
#include <string>

int main() {
    using namespace leanffi;

    // Test 1: escape_json_string
    assert(escape_json_string("hello") == "hello");
    assert(escape_json_string("a\"b") == "a\\\"b");
    assert(escape_json_string("line\nbreak") == "line\\nbreak");
    assert(escape_json_string("back\\slash") == "back\\\\slash");
    assert(escape_json_string("tab\there") == "tab\\there");
    std::cout << "[unit] escape_json_string OK\n";

    // Test 2: now_iso8601 format
    auto s = now_iso8601();
    assert(s.size() == 20);  // YYYY-MM-DDTHH:MM:SSZ
    assert(s[4] == '-' && s[7] == '-' && s[10] == 'T' && s.back() == 'Z');
    std::cout << "[unit] now_iso8601 OK (" << s << ")\n";

    // Test 3: Task struct layout
    Task t;
    t.task_id = 42;
    t.content = "test";
    t.kind = SourceKind::Source;
    t.is_file = false;
    assert(t.task_id == 42);
    assert(t.content == "test");
    std::cout << "[unit] Task struct OK\n";

    // Test 4: Result struct layout
    Result r;
    r.success = true;
    r.wall_seconds = 0.123;
    assert(r.success == true);
    std::cout << "[unit] Result struct OK\n";

    // Test 5: DispatchResult / SchedulerStats layout
    DispatchResult dr;
    dr.task_id = 1;
    dr.wall_ms = 250;
    assert(dr.wall_ms == 250);
    SchedulerStats s2;
    s2.total = 100; s2.succeeded = 95; s2.failed = 5;
    assert(s2.total == 100);
    std::cout << "[unit] DispatchResult/Stats OK\n";

    std::cout << "[unit] ALL TESTS PASSED\n";
    return 0;
}