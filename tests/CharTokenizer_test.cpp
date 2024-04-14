#include "lucanthrope/analysis/CharTokenizer.h"

#include <iostream>
#include <sstream>

int main() {
  using namespace lucanthrope;

  // AlphaCharTokenizer<>
  std::istream *ref_type_adjustment = new std::istringstream(
      "If an expression initially has the type \"reference to T\", the type "
      "is adjusted to T prior to any further analysis. The expression "
      "designates "
      "the object or function denoted by the reference, and the expression is "
      "an "
      "lvalue or an xvalue, depending on the expression.");

  Tokenizer *alpha_char_tokenizer =
      new AlphaCharTokenizer<>(*ref_type_adjustment);

  size_t counter = 0;
  std::cout << "Tokenized by AlphaCharTokenizer<>:\n";
  while (alpha_char_tokenizer->next()) {
    std::cout << ++counter << ": "
              << alpha_char_tokenizer->getToken().toString() << '\n';
  }
  std::cout << '\n';

  // LowerCaseTokenizer
  std::istream *glvalue = new std::istringstream(
      "A glvalue is an expression whose evaluation determines the identity of "
      "an object or function.");

  Tokenizer *lower_case_tokenizer = new LowerCaseTokenizer(*glvalue);
  counter = 0;
  std::cout << "Tokenized by LowerCaseTokenizer:\n";
  while (lower_case_tokenizer->next()) {
    std::cout << ++counter << ": "
              << lower_case_tokenizer->getToken().toString() << '\n';
  }
  std::cout << '\n';

  // UpperCaseTokenizer
  std::istream *prvalue = new std::istringstream(
      "A prvalue is an expression whose evaluation initializes an object or "
      "computes the value of an operand of an operator, as specified by the "
      "context in which it appears, or an expression that has type cv void.");

  Tokenizer *upper_case_tokenizer = new UpperCaseTokenizer(*prvalue);
  counter = 0;
  std::cout << "Tokenized by UpperCaseTokenizer:\n";
  while (upper_case_tokenizer->next()) {
    std::cout << ++counter << ": "
              << upper_case_tokenizer->getToken().toString() << '\n';
  }
  std::cout << '\n';

  // WhiteSpaceTokenizer<>
  std::istream *xvalue = new std::istringstream(
      "An xvalue is a glvalue that denotes an object whose resources can be "
      "reused (usually because it is near the end of its lifetime).");

  Tokenizer *whitespace_tokenizer = new WhiteSpaceTokenizer<>(*xvalue);
  counter = 0;
  std::cout << "Tokenized by WhiteSpaceTokenizer<>:\n";
  while (whitespace_tokenizer->next()) {
    std::cout << ++counter << ": "
              << whitespace_tokenizer->getToken().toString() << '\n';
  }
  std::cout << '\n';

  // AlphaCharTokenizer<decltype(lambda_normalizer_x)>
  auto lambda_normalizer_x = [](char c) {
    if (c == 'a')
      return 'X';
    return c;
  };
  std::istream *lvalue =
      new std::istringstream("An lvalue is a glvalue that is not an xvalue.");

  Tokenizer *custom_alpha_char_tokizer_x =
      new AlphaCharTokenizer<decltype(lambda_normalizer_x)>(
          *lvalue, lambda_normalizer_x);
  counter = 0;
  std::cout << "Tokenized by custom "
               "AlphaCharTokenizer<decltype(lambda_normalizer_x)>:\n";
  while (custom_alpha_char_tokizer_x->next()) {
    std::cout << ++counter << ": "
              << custom_alpha_char_tokizer_x->getToken().toString() << '\n';
  }
  std::cout << '\n';

  // WhiteSpaceTokenizer<decltype(lambda_normalizer_y)>
  auto lambda_normalizer_y = [](char c) {
    if (c == 'a')
      return 'Y';
    return c;
  };
  std::istream *rvalue =
      new std::istringstream("An rvalue is a prvalue or an xvalue.");

  Tokenizer *custom_alpha_char_tokizer_y =
      new WhiteSpaceTokenizer<decltype(lambda_normalizer_y)>(
          *rvalue, lambda_normalizer_y);
  counter = 0;
  std::cout << "Tokenized by custom "
               "WhiteSpaceTokenizer<decltype(lambda_normalizer_y)>:\n";
  while (custom_alpha_char_tokizer_y->next()) {
    std::cout << ++counter << ": "
              << custom_alpha_char_tokizer_y->getToken().toString() << '\n';
  }

  return 0;
}
