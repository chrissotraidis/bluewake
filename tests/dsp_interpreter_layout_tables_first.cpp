// The interpreter table's layout as DSPIntTables.cpp sees it: its own header
// first, before anything defines the Interpreter class.
#include "Core/DSP/Interpreter/DSPIntTables.h"

#include <cstddef>

#include "dsp_interpreter_layout.h"

DspTableLayout dsp_table_layout_tables_first() {
    using DSP::Interpreter::DecodedInterpreterOp;
    return {sizeof(DecodedInterpreterOp), offsetof(DecodedInterpreterOp, extension),
            offsetof(DecodedInterpreterOp, extended)};
}
