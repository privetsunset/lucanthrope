#pragma once

#include <memory> // unique_ptr

#include "Analysis.h"
#include "CharTokenizer.h"

namespace lucanthrope {

class WhiteSpaceAnalyzer : public Analyzer {
  virtual std::unique_ptr<TokenStream>
  getTokenStream(std::istream &input,
                 [[maybe_unused]] std::string_view fieldName =
                     std::string_view()) override {
    return std::unique_ptr<TokenStream>(new WhiteSpaceTokenizer<>(input));
  }
};

} // namespace lucanthrope
