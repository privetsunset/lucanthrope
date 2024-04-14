#pragma once

#include <cstddef> // size_t
#include <memory>  // unique_ptr
#include <string>
#include <unordered_set>
#include <vector>

#include "Analysis.h"
#include "CharTokenizer.h"
#include "StopFilter.h"

namespace lucanthrope {

class StopAnalyzer : public Analyzer {
public:
  // An array containing some common English words that are not usually useful
  // for searching.
  static constexpr const char *EnglishStopWords[] = {
      "a",    "an", "and",  "are",  "as", "at",   "be",   "been", "but",   "by",   "for",
      "if",    "in",   "into", "is", "it",   "no",   "not",   "of",   "on",
      "or",    "s",    "such", "t",  "that", "the", "their", "then", "there",
      "these", "those","they", "this", "to", "was",  "were", "will", "with"};

private:
  std::unordered_set<std::string> stopWords;

public:
  // Builds an analyzer which removes words in EnglishCommonWords
  StopAnalyzer() {
    size_t n = sizeof EnglishStopWords / sizeof(char *);
    const char *const *ptr = EnglishStopWords;
    while (n--)
      stopWords.insert(*ptr++);
  }

  // Builds an analyzer which removes words in the provided vector
  StopAnalyzer(const std::vector<std::string> &words) {
    for (auto &word : words)
      stopWords.insert(word);
  }

  virtual std::unique_ptr<TokenStream>
  getTokenStream(std::istream &input,
                 [[maybe_unused]] std::string_view fieldName =
                     std::string_view()) override {
    std::unique_ptr<TokenStream> tokenizer(new LowerCaseTokenizer(input));
    return std::unique_ptr<TokenStream>(
        new StopFilter(std::move(tokenizer), stopWords));
  }
};

} // namespace lucanthrope
