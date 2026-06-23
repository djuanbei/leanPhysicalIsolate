#include "pantograph_protocol.h"
#include "json_helpers.h"

namespace lpi::protocol {

std::string build(const std::string& cmd, const std::string& payload_json_inline) {
    return json::make_request(cmd, payload_json_inline);
}

}  // namespace lpi::protocol
