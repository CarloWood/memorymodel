#include "cwds/debug.h"

#ifdef CWDEBUG
#include <atomic>

#define DebugMarkUp debug::Mark marker("\e[42m⇧\e[0m")
#define DoutTag(cntrl, data, tag) LibcwDout(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data << " '" << tag << "' " << LOCATION(tag) << "]")

std::ostream& operator<<(std::ostream& os, std::memory_order);

#else

#define DebugMarkUp
#define DoutTag(cntrl, data, tag)

#endif
