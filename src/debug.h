#include "cwds/debug.h"

#ifdef CWDEBUG
#include <atomic>
#include <iosfwd>

#define DebugMarkUp debug::Mark marker("\e[42m⇧\e[0m")
#define DebugMarkDown debug::Mark marker("\e[42m⇩\e[0m")
#define DoutTag(cntrl, data, tag) LibcwDout(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data << " '" << tag << "' " << LOCATION(tag) << "]")

std::ostream& operator<<(std::ostream& os, std::memory_order);

#else

#define DebugMarkUp
#define DebugMarkDown
#define DoutTag(cntrl, data, tag)

#endif
