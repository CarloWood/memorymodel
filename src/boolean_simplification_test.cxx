#include "sys.h"
#include "debug.h"
#include "BooleanExpression.h"
#include "utils/MultiLoop.h"
#include <iostream>

int main()
{
#ifdef DEBUGGLOBAL
  GlobalObjectManager::main_entered();
#endif
  Debug(NAMESPACE_DEBUG::init());

  using namespace boolean;

  Variable const vA{Context::instance().create_variable("A")};
  Variable const vB{Context::instance().create_variable("B")};
  Variable const vC{Context::instance().create_variable("C")};
  Variable const vD{Context::instance().create_variable("D")};

  Product const A(vA);
  Product const B(vB);
  Product const C(vC);
  Product const D(vD);
  Product const notA(vA, true);
  Product const notB(vB, true);
  Product const notC(vC, true);
  Product const notD(vD, true);

  Expression e2(Expression(D) + Expression(notD * notC) + Expression(C * notB) + Expression(B * A) + Expression(notA));
  Dout(dc::notice, "e2 = " << e2);
  if (e2.equivalent(Expression(1)))
    Dout(dc::notice, "... which is equivalent with 1");
  else
    Dout(dc::notice, "... which is not equivalent with 1");
  return 0;

  // A̅BC̅D + ABC + ̅B̅CD + A̅B + ̅AC
  // AB'CD' + ABC + B'C'D + AB' + A' + C
  Product p1 = notB * C * A * notD;
  Product p2 = C * A * C * B;
  Product p3 = notC * D * notB;
  Product p4 = notB * A * A * notB;
  Product p5 = A * B * notC;

  Expression e(p1);
  Dout(dc::notice, "e = " << e);
  Dout(dc::notice, "adding " << C);
  e += C;
  Dout(dc::notice, "adding " << p4);
  e += p4;
  Dout(dc::notice, "adding " << notA);
  e += notA;
  Dout(dc::notice, "adding " << p3);
  e += p3;
  Dout(dc::notice, "adding " << p2);
  e += p2;
  Dout(dc::notice, "adding " << p5);
  e += p5;

  Dout(dc::notice, "e = " << e);
  if (e.equivalent(Expression(1)))
    Dout(dc::notice, "... which is equivalent with 1");
  else
    Dout(dc::notice, "... which is not equivalent with 1");
  e.simplify();
  Dout(dc::notice, "e = " << e);
  if (e.equivalent(Expression(1)))
    Dout(dc::notice, "... which is equivalent with 1");
  else
    Dout(dc::notice, "... which is not equivalent with 1");
}
