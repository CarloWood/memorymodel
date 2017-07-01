#include "cwds/debug.h"

#ifdef CWDEBUG
#include <atomic>
#include <iosfwd>

#define DebugMarkUp debug::Mark marker("\e[42;33;1m⇧\e[0m")
#define DebugMarkUpRight debug::Mark marker("\e[43;33;1m↱\e[0m")
#define DebugMarkDown debug::Mark marker("\e[45;33;1m⇩\e[0m")
#define DebugMarkDownRight debug::Mark marker("\e[43;33;1m↳\e[0m")
#define DoutTag(cntrl, data, tag) LibcwDout(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data << " '" << tag << "' " << LOCATION(tag) << "]")

std::ostream& operator<<(std::ostream& os, std::memory_order);

#else

#define DebugMarkUp
#define DebugMarkUpRight
#define DebugMarkDown
#define DebugMarkDownRight
#define DoutTag(cntrl, data, tag)

#endif
