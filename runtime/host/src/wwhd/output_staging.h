// Private output ownership: one import per parent, recover only marked jobs.
#pragma once
#include "filesystem.h"
#include <memory>

namespace wwhd {
class OutputStaging {
public:
    // Locks the parent and recovers abandoned jobs before any new payload exists.
    explicit OutputStaging(const std::filesystem::path& output);
    ~OutputStaging();
    OutputStaging(const OutputStaging&) = delete;
    OutputStaging& operator=(const OutputStaging&) = delete;
    std::filesystem::path create();
    void publish(); // Rename only the payload; subsequent cleanup cannot undo success.
    void discard(); // Workers must have joined. Errors leave the marker for a retry.
private:
    struct State;
    std::unique_ptr<State> state_;
};
} // namespace wwhd
