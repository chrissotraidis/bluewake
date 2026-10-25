// <filesystem> for the built-in Wind Waker HD importer. The Windows host's
// POSIX shim (windows/compat/bw_posix_compat.h) defines rename() as a macro,
// which breaks <filesystem>; it is set aside for the include. Call
// std::filesystem::rename as (fs::rename)(...) so the macro cannot expand.
#pragma once

#pragma push_macro("rename")
#undef rename
#include <filesystem>
#pragma pop_macro("rename")
