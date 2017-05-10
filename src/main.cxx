#include "sys.h"
#include "debug.h"

int main()
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "Hello World");
}
