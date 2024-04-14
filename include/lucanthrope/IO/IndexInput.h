#pragma once

#include <cstddef> // size_t
#include <cstdint>
#include <stdint.h>
#include <string>

#include "IndexIOBase.h"

namespace lucanthrope {

class IndexInput : public IndexIOBase {
protected:
  // One more pointer is required (points to the one-past-the-last byte that was
  // read from source into buffer). Buffer has to be filled with the new data
  // when bufCur == sentinel.
  char *sentinel = nullptr;

public:
  IndexInput() = default;
  virtual ~IndexInput() override = default;

  void setExternalBuffer(char *bufferStart, size_t size) {
    IndexIOBase::setExternalBuffer(bufferStart, size);
    bufCur = bufEnd;
  }

  size_t getNumReadableBytes() const { return sentinel - bufCur; }

  bool hasPendingData() const { return getNumReadableBytes() > 0; }

  // If all buffered data was consumed, then try
  bool eof() { return !hasPendingData() && !fillImpl(); }

  // Data input Interface:

  // read byte or throw
  char readByte();

  // read as much as possible, but no more than size
  size_t read(char *ptr, size_t size);

  // read uint32_t or throw
  uint32_t readInt32();

  // read uint64_t or throw
  uint64_t readInt64();

  // parse varint32 or throw
  uint32_t readVarint32();

  // parse varint64 or throw
  uint64_t readVarint64();

  // Takes a reference to string so that the same external string could be used
  // multiple times as a buffer
  void readString(std::string &buff) {
    buff.clear();
    uint32_t size = readVarint32();
    while (size--)
      buff.push_back(readByte());
  }

private:
  // Reads as mush data as possible into buffer, sets bufCur to the beginning of
  // read bytes, sets sentinel to the next byte after read bytes. Returns true
  // on success, returns false if there is no more data in the source, throws an
  // exception if something is wrong.
  virtual bool fillImpl() = 0;
};

} // namespace lucanthrope