#include "lucanthrope/analysis/Analysis.h"
#include "lucanthrope/analysis/StopAnalyzer.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

int main() {
  using namespace lucanthrope;

  std::istream *template_arguments_deduction = new std::istringstream(
      "Template arguments can be deduced in several different contexts, but in "
      "each case a type that is specified in terms of template parameters "
      "(call it P) is compared with an actual type (call it A), and an attempt "
      "is made to find template argument values (a type for a type parameter, "
      "a value for a non-type parameter, or a template for a template "
      "parameter) that will make P, after substitution of the deduced values "
      "(call it the deduced A), compatible with A.");

  Analyzer *value_initialized_analyzer = new StopAnalyzer();
  std::unique_ptr<TokenStream> value_initialized_analyzer_tokens(
      value_initialized_analyzer->getTokenStream(
          *template_arguments_deduction));

  size_t counter = 0;
  std::cout << "Tokens produced by value-inititialized StopAnalyzer:\n";
  while (value_initialized_analyzer_tokens->next()) {
    std::cout << ++counter << ": "
              << value_initialized_analyzer_tokens->getToken().toString()
              << '\n';
  }
  std::cout << '\n';

  std::istream *forwarding_reference = new std::istringstream(
      "A forwarding reference is an rvalue reference to a cv-unqualified "
      "template parameter that does not represent a template parameter of a "
      "class template (during class template argument deduction). If P is a "
      "forwarding reference and the argument is an lvalue, the type \"lvalue "
      "reference to A\" is used in place of A for type deduction.");

  std::vector<std::string> stopWords = {"template", "rvalue", "lvalue"};

  Analyzer *vector_initialized_analyzer = new StopAnalyzer(stopWords);
  std::unique_ptr<TokenStream> vector_initialized_analyzer_tokens(
      vector_initialized_analyzer->getTokenStream(*forwarding_reference));

  counter = 0;
  std::cout << "Tokens produced by StopAnalyzer initialized with "
               "std::vector<std::string>:\n";
  while (vector_initialized_analyzer_tokens->next()) {
    std::cout << ++counter << ": "
              << vector_initialized_analyzer_tokens->getToken().toString()
              << '\n';
  }

  return 0;
}
