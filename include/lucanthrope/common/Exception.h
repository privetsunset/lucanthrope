#pragma once

#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace lucanthrope {

// Simple class for all exceptions thrown by the lucanthrope library components.
// Right now it has only the barest fuctionality required to communicate an
// error info to an exception handler, but will become more sophisticated in the
// future.
class Exception : public std::exception {
public:
  enum Code {
    FileAlreadyExistsException,
    FileNotFoundException,
    IOErrorException,
    // thrown when contents of an index file cannot be parsed, or, for example,
    // when FieldInfos doesn't contain some field when it has to be there
    IndexCorruptionException,
  };

  Exception(Code code) : code_(code) {}
  Exception(Code code, std::string &msg) : code_(code), msg_(std::move(msg)) {}
  Exception(Code code, std::string_view msg) : code_(code), msg_(msg) {}
  Exception(const Exception &) = default;
  Exception &operator=(const Exception &) = default;
  virtual ~Exception() = default;

  virtual const char *what() const noexcept override { return msg_.c_str(); }

  Code code() const { return code_; }

private:
  Code code_;
  std::string msg_;
};

} // namespace lucanthrope