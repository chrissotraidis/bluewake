#pragma once

#include <cstddef>

struct DspTableLayout {
    std::size_t size, extension, extended;
};

DspTableLayout dsp_table_layout_tables_first();
