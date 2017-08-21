#pragma once

#include "cwds/debug.h"

#ifdef CWDEBUG
#include <atomic>
#include <iosfwd>

#define DebugMarkUpStr "\e[42;33;1m⇧\e[0m"
#define DebugMarkUpRightStr "\e[43;33;1m↱\e[0m"
#define DebugMarkDownStr "\e[45;33;1m⇩\e[0m"
#define DebugMarkDownRightStr "\e[43;33;1m↳\e[0m"
#define DebugMarkUp debug::Mark marker(DebugMarkUpStr)
#define DebugMarkUpRight debug::Mark marker(DebugMarkUpRightStr)
#define DebugMarkDown debug::Mark marker(DebugMarkDownStr)
#define DebugMarkDownRight debug::Mark marker(DebugMarkDownRightStr)
#define DoutTag(cntrl, data, tag) LibcwDout(LIBCWD_DEBUGCHANNELS, ::libcwd::libcw_do, cntrl, data << " '" << tag << "' " << LOCATION(tag) << "]")

std::ostream& operator<<(std::ostream& os, std::memory_order);

NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct sb_edge;
extern channel_ct asw_edge;
NAMESPACE_DEBUG_CHANNELS_END

#else // CWDEBUG

#define DebugMarkUp
#define DebugMarkUpRight
#define DebugMarkDown
#define DebugMarkDownRight
#define DoutTag(cntrl, data, tag)

#endif // CWDEBUG
