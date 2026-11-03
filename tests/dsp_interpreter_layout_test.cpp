// The DSP interpreter's predecoded op table must have one layout in every
// file that uses it. Under the Microsoft ABI a pointer to a member function of
// a class that is only forward-declared is 24 bytes, and 8 once the class is
// complete, so DSPIntTables.cpp (its header first) and DSPInterpreter.cpp (the
// class first) once disagreed: 56- against 24-byte entries, and Exact (LLE)
// audio on Windows called address 0 at its first DSP instruction
// (patches/recompcore/0114). This file sees the class first, as
// DSPInterpreter.cpp does; the other sees the table header first.
#include "Core/DSP/Interpreter/DSPInterpreter.h"
#include "Core/DSP/Interpreter/DSPIntTables.h"

#include <cstddef>
#include <cstdio>

#include "dsp_interpreter_layout.h"

int main() {
    using DSP::Interpreter::DecodedInterpreterOp;
    const DspTableLayout here{sizeof(DecodedInterpreterOp), offsetof(DecodedInterpreterOp, extension),
                              offsetof(DecodedInterpreterOp, extended)};
    const DspTableLayout first = dsp_table_layout_tables_first();
    std::printf("decoded op: %zu bytes (extension at %zu, extended at %zu); table header first: %zu (%zu, %zu)\n",
                here.size, here.extension, here.extended, first.size, first.extension, first.extended);
    if (here.size != first.size || here.extension != first.extension || here.extended != first.extended) {
        std::fprintf(stderr, "FAIL: the DSP interpreter table's layout depends on include order\n");
        return 1;
    }
    std::puts("dsp interpreter table layout: PASS");
    return 0;
}
