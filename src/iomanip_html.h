#include "utils/iomanip.h"

class IOManipHtml : public utils::iomanip::Unsticky<2>
{
 private:
  static utils::iomanip::Index s_index;

 public:
  IOManipHtml(bool html) : Unsticky(s_index, html) { }

  static bool is_html(std::ostream& os) { return get_iword_from(os, s_index); }
};

extern IOManipHtml html;
