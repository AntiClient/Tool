#ifndef PROCESS_ITERATOR_H
#define PROCESS_ITERATOR_H

// clang-format off
#include <functional>
// clang-format on

using ProcessCallback = std::function<void(
    const std::string& process_name, uint32_t process_id, uint32_t parent_process_id, const std::string& start_date)>;

void IterateProcesses(const ProcessCallback &cb);

#endif