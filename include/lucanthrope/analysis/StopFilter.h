#pragma once

#include <cstddef> // size_t
#include <memory>  // unique_ptr
#include <string>
#include <unordered_set>
#include <utility> // move()
#include <vector>

#include "Analysis.h"

#include <iostream>

namespace lucanthrope {

// StopFilter removes from the input TokenStream those tokens whose termText is
// a member of the provided set of words
class StopFilter : public TokenFilter {
private:
  const std::unordered_set<std::string> &stopWords;

public:
  StopFilter(std::unique_ptr<TokenStream> input,
             const std::unordered_set<std::string> &words)
      : TokenFilter(std::move(input)), stopWords(words) {}

  virtual bool next() override {
    while (input_->next()) {
      Token local_tok = input_->getToken();
      // std::cout << local_tok.toString() << '\n';
      auto it = stopWords.find(local_tok.termText);
      if (it == stopWords.end()) {
        // std::cout << local_tok.toString() << " is not skipped\n";
        tok = std::move(local_tok);
        return true;
      }
      // std::cout << local_tok.toString() << " is skipped because stopWords has " << *it << '\n';
    }
    return false;
  }
};

} // namespace lucanthrope
