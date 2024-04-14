#pragma once

#include <cassert>
#include <cstddef>
#include <istream>
#include <memory> // unique_ptr
#include <string>
#include <string_view>
#include <utility> // move()
#include <variant>
#include <vector>

namespace lucanthrope {

// A field is a section of a Document.  Each field has two parts, a name and a
// value.  Values may be free text, provided as a string or as a std::istream,
// or they may be atomic keywords, which are not further processed.  Such
// keywords may be used to represent dates, urls, etc.  Fields are optionally
// stored in the index, so that they may be returned with hits on the document.
class Field {
private:
  std::string name_;
  // Using std::variant allows us to easily extend to other value types in the
  // future.
  std::variant<std::string, std::unique_ptr<std::istream>> value_;
  unsigned char isStored_ : 1;
  unsigned char isIndexed_ : 1;
  unsigned char isTokenized_ : 1;

public:
  // We use std::string_view instead of references to strings because:
  // 1. If field's name is initially string literal with a static storage
  // duration (which typically would be true), then it will be copied through
  // std::string_view to name_; if we were using reference to string, then
  // temporary string object would be created for string literal, copied (not
  // moved!) to name_, then destroyed. That is, we would get unnecessary
  // allocation and deallocation. std::move() would not help for const
  // std::string &name in signature since it will not cast away const. The only
  // solution would be to define different constructors for std::string &&name,
  // which would complicate the API.
  // 2. Field values typically will not be string literals, but they will be
  // read from index for stored fields, which means that again construction of
  // temporary string object would occur if we were using const std::string
  // &value in signature. These temporaries would be created to copy the
  // contents of read buffer. On the contrary, std::string_view just refer to
  // the contents of the buffer, so extra allocation and deallocation is again
  // avoided.
  Field(std::string_view name, std::string_view value, bool store, bool index,
        bool token)
      : name_(name), value_(std::string(value)), isStored_(store),
        isIndexed_(index), isTokenized_(token) {
    assert(!name_.empty() && "Name cannot be empty!");
    assert(!std::get<std::string>(value_).empty() && "Value cannot be empty!");
  }

  // Constructs a field with std::istream value. Note that it takes ownership of
  // passed std::istream.
  Field(std::string_view &name, std::unique_ptr<std::istream> value)
      : name_(name), value_(std::move(value)), isStored_(false),
        isIndexed_(true), isTokenized_(true) {
    assert(!name_.empty() && "Name cannot be empty!");
    assert(std::get<std::unique_ptr<std::istream>>(value_).get() != nullptr &&
           "Value cannot be null!");
  }

  // Copying is prohibited due to owned std::istream.
  Field(const Field &) = delete;
  Field &operator=(const Field &) = delete;

  // But moving is allowed
  Field(Field &&) = default;
  Field &operator=(Field &&) = default;

  ~Field() = default;

  // Constructs a String-valued Field that is not tokenized, but is indexed
  // and stored.  Useful for non-text fields, e.g. date or url.
  static Field keyword(std::string_view name, std::string_view value) {
    return Field(name, value, true, true, false);
  }

  // Constructs a String-valued Field that is not tokenized or indexed,
  // but is stored in the index, for return with hits.
  static Field unindexed(std::string_view name, std::string_view value) {
    return Field(name, value, true, false, false);
  }

  // Constructs a String-valued Field that is tokenized and indexed,
  // and is stored in the index, for return with hits.  Useful for short text
  // fields, like "title" or "subject".
  static Field text(std::string_view name, std::string_view value) {
    return Field(name, value, true, true, true);
  }

  // Constructs a String-valued Field that is tokenized and indexed,
  // but that is not stored in the index.
  static Field unstored(std::string_view name, std::string_view value) {
    return Field(name, value, false, true, true);
  }

  // Constructs a Reader-valued Field that is tokenized and indexed, but is
  // not stored in the index verbatim.  Useful for longer text fields, like
  // "body".
  static Field text(std::string_view name,
                    std::unique_ptr<std::istream> value) {
    return Field(name, std::move(value));
  }

  const std::string &getName() const { return name_; }

  // Returns true iff the value of the field is a string.  If false, then the
  // value of the field is std::unique_ptr<std::istream>.
  bool isStringValue() const {
    return std::holds_alternative<std::string>(value_);
  }

  // The value of the field as a Reader, or null.  If null, the String value
  // is used.  Exactly one of stringValue() and readerValue() must be set.
  bool isIStreamValue() const {
    return std::holds_alternative<std::unique_ptr<std::istream>>(value_);
  }

  // REQUIRES: isStringValue() == true
  const std::string &getStringValue() const {
    assert(isStringValue());
    return std::get<std::string>(value_);
  }

  // REQUIRES: isIStreamValue() == true
  std::istream &getIStreamValue() const {
    assert(isIStreamValue());
    return *std::get<std::unique_ptr<std::istream>>(value_).get();
  }

  // True iff the value of the field is to be stored in the index for return
  // with search hits.  It is an error for this to be true if a field is
  // Reader-valued.
  bool isStored() const { return isStored_; }

  // True iff the value of the field is to be indexed, so that it may be
  // searched on.
  bool isIndexed() const { return isIndexed_; }

  // True iff the value of the field should be tokenized as text prior to
  // indexing.  Un-tokenized fields are indexed as a single word and may not be
  // Reader-valued.
  bool isTokenized() const { return isTokenized_; }

  // Prints a Field for human consumption.
  std::string toString() const {
    std::string ret(name_);
    ret.push_back('(');
    ret.append(isStored() ? "stored," : "not stored,")
        .append(isIndexed() ? "indexed," : "not indexed,")
        .append(isTokenized() ? "tokenized," : "not tokenized,")
        .append(isStringValue() ? "string value)" : "istream value)");
    return ret;
  }
};

// Documents are the unit of indexing and search.
//
// A Document is a set of fields.  Each field has a name and a textual value.
// A field may be stored with the document, in which case it is returned with
// search hits on the document.  Thus each document should typically contain
// stored fields which uniquely identify it.
class Document {
private:
  std::vector<Field> fields_;

public:
  // Constructs a new document with no fields.
  Document() = default;
  Document(const Document &) = delete;
  Document &operator=(const Document &) = delete;

  using iterator = std::vector<Field>::iterator;
  using const_iterator = std::vector<Field>::const_iterator;
  iterator begin() { return fields_.begin(); }
  iterator end() { return fields_.end(); }
  const_iterator begin() const { return fields_.begin(); }
  const_iterator end() const { return fields_.end(); }

  // Adds a field to a document.  Several fields may be added with
  // the same name.  In this case, if the fields are indexed, their text is
  // treated as though appended for the purposes of search.
  Document &add(Field &&field) {
    fields_.push_back(std::move(field));
    return *this;
  }

  // Returns an iterator to the field with the given name (if any), or
  // past-the-end iterator. If multiple fields exists with this name, returns an
  // iterator to the first added field with this name.
  iterator find(const std::string &name) {
    auto it = fields_.begin();
    while (it != fields_.end()) {
      if (it->getName() == name)
        break;
      it++;
    }
    return it;
  }

  const_iterator find(const std::string &name) const {
    auto it = fields_.begin();
    while (it != fields_.end()) {
      if (it->getName() == name)
        break;
      it++;
    }
    return it;
  }

  // Prints the fields of a document for human consumption.
  std::string toString() const {
    std::string ret;
    ret.append("Document<");
    if (!fields_.empty()) {
      ret.append(fields_[0].toString());
      for (size_t i = 1; i < fields_.size(); i++) {
        ret.append(", ");
        ret.append(fields_[i].toString());
      }
    }
    ret.push_back('>');
    return ret;
  }
};

} // namespace lucanthrope
