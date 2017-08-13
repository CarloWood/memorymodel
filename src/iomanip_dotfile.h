#include "utils/iomanip.h"

class IOManipDotFile : public utils::iomanip::Unsticky<2>
{
 private:
  static utils::iomanip::Index s_index;

 public:
  IOManipDotFile(bool dot_file) : Unsticky(s_index, dot_file) { }

  static bool is_dot_file(std::ostream& os) { return get_iword_from(os, s_index); }
};

extern IOManipDotFile dotfile;
