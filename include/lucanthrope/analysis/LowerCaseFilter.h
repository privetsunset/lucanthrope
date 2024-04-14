#pragma once

#include <cctype>  // tolower()
#include <memory>  // unique_ptr
#include <utility> // move()

#include "Analysis.h"

namespace lucanthrope {

// Normalizes token text to lower case.
class LowerCaseFilter : public TokenFilter {
public:
  LowerCaseFilter(std::unique_ptr<TokenStream> input)
      : TokenFilter(std::move(input)) {}
  virtual ~LowerCaseFilter() = default;
  virtual bool next() {
    if (input_->next()) {
      tok = input_.get()->getToken();
      for (auto &ch : tok.termText)
        ch = std::tolower(ch);
      return true;
    }
    return false;
  }
};

} // namespace lucanthrope
