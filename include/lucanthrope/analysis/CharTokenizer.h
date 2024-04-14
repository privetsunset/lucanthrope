#pragma once

#include <cctype>  // isalpha(), tolower(), toupper()
#include <cstddef> // size_t
#include <cstdint>
#include <istream>
#include <string>

#include "Analysis.h"

namespace lucanthrope {

// Class template for simple, character-oriented tokenizers.
// Predicate and Normalizer are function objects. For Predicate pred and
// Normalizer norm:
// - bool pred(char c) returns true iff c should be included in a token. This
// tokenizer generates as tokens adjacent sequences of characters which satisfy
// this predicate.  Characters for which this is false are used to define token
// boundaries and are not included in tokens;
// - char norm(char c) is called on each token character c to normalize it
// before it is added to the token.
template <typename Predicate, typename Normalizer>
class CharTokenizer : public Tokenizer {
private:
  // Remember that until C++20 the closure type associated with a
  // lambda-expression has no default constructor, so don't call constructor
  // with 1 parameter if you instantiating CharTokenizer on closure type(s).
  Predicate isTokenChar;
  Normalizer normalize;
  std::string buffer;
  uint64_t offset = 0;

public:
  CharTokenizer(std::istream &input) : Tokenizer(input) {}

  CharTokenizer(std::istream &input, Predicate pred)
      : Tokenizer(input), isTokenChar(pred) {}

  CharTokenizer(std::istream &input, Normalizer norm)
      : Tokenizer(input), normalize(norm) {}

  CharTokenizer(std::istream &input, Predicate pred, Normalizer norm)
      : Tokenizer(input), isTokenChar(pred), normalize(norm) {}

  virtual ~CharTokenizer() = default;

  virtual bool next() override {
    buffer.clear();
    size_t tok_len = 0;
    uint64_t start_pos;
    while (true) {
      if (input_.eof()) {
        if (tok_len)
          break; // collect last token
        return false;
      }
      char c = input_.get();
      offset++;
      if (isTokenChar(c)) {
        if (!tok_len) // start of the token
          start_pos = offset - 1;
        buffer.push_back(normalize(c));
        tok_len++;
      } else if (tok_len) // end of the token
        break;
    }
    tok = Token(buffer, start_pos, start_pos + tok_len);
    return true;
  }
};

namespace {

struct noop_normalizer {
  char operator()(char c) { return c; }
};

struct tolower_normalizer {
  char operator()(char c) { return std::tolower(c); }
};

struct toupper_normalizer {
  char operator()(char c) { return std::toupper(c); }
};

struct isalpha_predicate {
  bool operator()(char c) { return std::isalpha(c); }
};

struct iswhitespace_predicate {
  bool operator()(char c) {
    if (c == ' ' || c == '\t' || c == '\n')
      return false;
    return true;
  }
};

} // unnamed namespace

// Tokenizer that divides text at non-letters, as determined by std::isalpha()
template <class Normalizer = noop_normalizer>
using AlphaCharTokenizer = CharTokenizer<isalpha_predicate, Normalizer>;

// Tokenizer that divides text at non-letters, as determined by std::isalpha(),
// and converts each token character to lower case, as determined by
// std::tolower()
using LowerCaseTokenizer = AlphaCharTokenizer<tolower_normalizer>;

// Tokenizer that divides text at non-letters, as determined by std::isalpha(),
// and converts each token character to upper case, as determined by
// std::toupper()
using UpperCaseTokenizer = AlphaCharTokenizer<toupper_normalizer>;

// Tokenizer that divides text at whitespace: adjacent sequences of
// non-whitespace characters form tokens
template <class Normalizer = noop_normalizer>
using WhiteSpaceTokenizer = CharTokenizer<iswhitespace_predicate, Normalizer>;

} // namespace lucanthrope
