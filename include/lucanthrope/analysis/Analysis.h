#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <utility> // move()

namespace lucanthrope {

class TokenStream;

// A Token is an occurence of a term from the text of a field.  It consists of
// a term's text, the start and end offset of the term in the text of the field,
// and a reference to type string.
//
// The start and end offsets permit applications to re-associate a token with
// its source text, e.g., to display highlighted query terms in a document
// browser, or to show matching text fragments in a KWIC (KeyWord In Context)
// display, etc.
//
// The type is a string, assigned by a lexical analyzer (a.k.a. tokenizer),
// naming the lexical or syntactic class that the token belongs to.  For example
// an end of sentence marker token might be implemented with type "eos". The
// default token type is an empty string.
struct Token {
  std::string termText;  // the text of the term
  uint64_t startPos;     // start in source text
  int endPos;            // end in source text
  std::string_view type; // lexical type

  Token() = default;
  // Constructs a Token with the given term text, and start & end offsets.
  // The type defaults to "word."
  Token(std::string_view text, uint64_t start, uint64_t end)
      : termText(text), startPos(start), endPos(end) {}

  // Constructs a Token with the given text, start and end offsets, & type.
  Token(std::string_view text, uint64_t start, uint64_t end,
        std::string_view type)
      : termText(text), startPos(start), endPos(end),
        type(type) {}

  Token(const Token &) = default;
  Token(Token &&) = default;
  Token &operator=(const Token &) = default;
  Token &operator=(Token &&) = default;
  ~Token() = default;

  std::string toString() const {
    std::string ret("[type: ");
    ret.append(type.size() ? type
                           : "<no type>")
        .append(", ");
    ret.append("text: ").append(termText).append(", ");
    ret.append("start: ").append(std::to_string(startPos)).append(", ");
    ret.append("end: ").append(std::to_string(endPos)).push_back(']');
    return ret;
  }
};

// A TokenStream enumerates the sequence of tokens, either from
// fields of a document or from query text.
//
// This is an abstract class.  Concrete subclasses are:
// Tokenizer, a TokenStream whose input is a Reader; and
// TokenFilter, a TokenStream whose input is another TokenStream.
class TokenStream {
protected:
  Token tok;

public:
  TokenStream() = default;
  TokenStream(const TokenStream &) = delete;
  void operator=(const TokenStream &) = delete;
  virtual ~TokenStream() = default;

  // Tries to parse next token in the stream, returns true on success,
  // false if the end of stream is reached. Throws an exception if something is
  // wrong (I/O error, unable to parse a token, etc.). If next() returns true,
  // then getToken() returns lvalue refernce to the token just lexed.
  //
  // FIXME! This has to be rewritten so that this function is not virtual.
  // Calling virtual function for each token sounds like a bad idea, but let's
  // keep it simple for now.
  virtual bool next() = 0;

  // Retuns the last lexed token; next() must be called (and evaluate to true)
  // before this function is called.
  const Token &getToken() const { return tok; }
};

// An Analyzer builds TokenStreams, which analyze text.  It thus represents a
// policy for extracting index terms from text.
//
// Typical implementations first build a Tokenizer, which breaks the stream of
// characters from the Reader into raw Tokens.  One or more TokenFilters may
// then be applied to the output of the Tokenizer.
class Analyzer {
public:
  Analyzer() = default;

  Analyzer(const Analyzer &) = delete;
  Analyzer &operator=(const Analyzer &) = delete;
  virtual ~Analyzer() = default;
  // Creates a TokenStream which tokenizes all the text in the provided istream.
  // istream is owned by Document, so getTokenStream() doesn't take ownership
  virtual std::unique_ptr<TokenStream> getTokenStream(
      std::istream &input,
      [[maybe_unused]] std::string_view fieldName = std::string_view()) = 0;
};

// A Tokenizer is a TokenStream whose input is a Reader.
class Tokenizer : public TokenStream {
protected:
  // The text source for this Tokenizer.
  std::istream &input_;

public:
  Tokenizer(std::istream &input) : input_(input) {}
  virtual ~Tokenizer() = default;
};

// A TokenFilter is a TokenStream whose input is another token stream.
// TokenFilter takes ownership of the provided token stream.
class TokenFilter : public TokenStream {
protected:
  // The source of tokens for this filter.
  std::unique_ptr<TokenStream> input_;

public:
  TokenFilter(std::unique_ptr<TokenStream> input) : input_(std::move(input)){};
  virtual ~TokenFilter() = default;
};

} // namespace lucanthrope
