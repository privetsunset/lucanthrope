#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "Analysis.h"

namespace lucanthrope {

// This analyzer is used to facilitate scenarios where different fields require
// different analysis techniques.  Use addAnalyzer() to add a non-default
// analyzer on a field name basis.
class PerFieldAnalyzerWrapper : public Analyzer {
private:
  std::unordered_map<std::string, Analyzer *> analyzerMap;
  Analyzer &defaultAnalyzer;

public:
  // Constructs with default analyzer.
  // Any fields not specifically defined to use a different analyzer will use
  // the one provided here.
  PerFieldAnalyzerWrapper(Analyzer &default_) : defaultAnalyzer(default_) {}

  // Defines an analyzer to use for the specified field.
  void addAnalyzer(const std::string &filedName, Analyzer *analyzer) {
    analyzerMap[filedName] = analyzer;
  }

  virtual std::unique_ptr<TokenStream>
  getTokenStream(std::istream &input,
                 std::string_view fieldName = std::string_view()) override {
    if (!fieldName.size())
      return defaultAnalyzer.getTokenStream(input);
    auto it = analyzerMap.find(std::string(fieldName.data(), fieldName.size()));
    if (it != analyzerMap.end())
      return it->second->getTokenStream(input);
    return defaultAnalyzer.getTokenStream(input);
  }
};

} // namespace lucanthrope
