#include <iostream>
#include "boolexpr/boolexpr.h"

int main()
{
  using namespace boolexpr;

  boolexpr::Context context;
  one_t one{boolexpr::one()};
  zero_t zero{boolexpr::zero()};

  std::vector<var_t> A1;

  char const* vars[] = { "x", "y", "z" };
  for (int i = 0; i < 3; ++i)
    A1.push_back(context.get_var(vars[i]));

  bx_t xy = and_({A1[0], A1[1]});
  bx_t xYZ = and_({A1[0], ~A1[1], ~A1[2]});
  bx_t XyZ = and_({~A1[0], A1[1], ~A1[2]});
  bx_t XY = and_({~A1[0], ~A1[1]});

  array_t array{new boolexpr::Array({xy, xYZ, XyZ, zero})};
  std::cout << "array =\n";
  for (auto&& item : *array)
    std::cout << "= " << item->to_string() << std::endl;

  bx_t combined = or_({xy, XY, XyZ, xYZ, A1[2]});

  soln_t sol0 = combined->sat();
  std::cout << "The expression is " << (sol0.first ? "" : "not ") << "solvable." << std::endl;
  soln_t sol1 = (~combined)->sat();
  std::cout << "The inverse is " << (sol1.first ? "" : "not ") << "solvable." << std::endl;

  bool is_zero = combined->equiv(zero);
  if (is_zero)
    std::cout << "The expression equals zero!" << std::endl;
  bool is_one = combined->equiv(one);
  if (is_one)
    std::cout << "The expression equals one!" << std::endl;
  assert(!is_zero || !sol0.first);
  assert(!is_one || !sol1.first);

  for (int i = 0; i < 3; ++i)
  {
    point_t r;
    r[A1[i]] = zero;
    bx_t x0 = combined->restrict_(r)->simplify();
    std::cout << combined->to_string() << " with " << A1[i]->to_string() << "==0 is " << x0->to_string() << std::endl;

    r[A1[i]] = one;
    bx_t x1 = combined->restrict_(r)->simplify();
    std::cout << combined->to_string() << " with " << A1[i]->to_string() << "==1 is " << x1->to_string() << std::endl;

    if (x0->equiv(x1))
      std::cout << "The expression does not depend on " << A1[i]->to_string() << "." << std::endl;
  }
}
