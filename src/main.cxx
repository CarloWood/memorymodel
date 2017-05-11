#include "sys.h"
#include "debug.h"
#include "tracked.h"

// SB = Sequenced Before.
//
//   This means that within a single thread the memory access belongs
//   to a statement that was sequenced before a memory access belonging
//   to the current statement.
//
//   For example:
//
//   r1 = (x==x) == (x==x);     // Does four unsequenced accesses (to x).
//   r2 = (x==x);               // Does two unsequenced accesses (to x).
//
//   then each of the four (tail) accesses are sequenced before each of
//   the two (head) accesses. Named tail and head because in the final
//   graph each SB edge is directed from tail to head.
//
namespace { constexpr char const* const name_SBTracker = "SBTracker"; }

struct SBTracker : tracked::Tracked<&name_SBTracker> {
  bool m_used;
  using tracked::Tracked<&name_SBTracker>::Tracked;
  SBTracker() : m_used(false) { }
  ~SBTracker() { if (!m_used) Dout(dc::notice, "End of statement."); }
  void used() { m_used = true; }
};

SBTracker g(SBTracker&& transfer1)
{
  Dout(dc::notice, "Inside g(); using " << transfer1);
  transfer1.used();
  SBTracker transfer;
  Dout(dc::notice, "Leaving g()...");
  return transfer;
}

SBTracker f()
{
  SBTracker transfer;
  Dout(dc::notice, "Leaving f()...");
  return transfer;
}

int main()
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "Before f()");
  g(g(f()));
  Dout(dc::notice, "After f()");
}
