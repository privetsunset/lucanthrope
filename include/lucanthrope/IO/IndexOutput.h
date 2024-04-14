#pragma once

#include <cstddef> // size_t
#include <cstdint>
#include <string_view>

#include "IndexIOBase.h"

namespace lucanthrope {

class IndexOutput : public IndexIOBase {
public:
  IndexOutput() = default;
  virtual ~IndexOutput() override = default;

  void setExternalBuffer(char *bufferStart, size_t size) {
    IndexIOBase::setExternalBuffer(bufferStart, size);
    bufCur = bufStart;
  }

  size_t getNumWritableBytes() const { return bufCur - bufStart; }

  // Number of unused bytes after cursor
  size_t available() const { return bufEnd - bufCur; }

  void flush() {
    if (bufCur != bufStart)
      flushNonEmpty();
  }

  virtual void sync() { flush(); }

  // Data output Interface:
  IndexOutput &writeByte(char c);

  IndexOutput &write(const char *Ptr, size_t Size);

  IndexOutput &writeInt32(uint32_t num);

  IndexOutput &writeInt64(uint64_t num);

  IndexOutput &writeVarint32(uint32_t num);

  IndexOutput &writeVarint64(uint64_t num);

  IndexOutput &writeString(std::string_view str) {
    return writeVarint32(static_cast<uint32_t>(str.size()))
        .write(str.data(), str.size());
  }

private:
  // Flushes the buffer which is known to be non-empty and resets the position
  // to the beginning of the buffer. Throws an exception if something is wrong.
  void flushNonEmpty() {
    assert(getNumWritableBytes() && "Buffer is currently empty!");
    writeImpl();
    bufCur = bufStart;
  }

  // Copy data into the buffer. Size must not be greater than the number of
  // unused bytes in the buffer.
  void copyToBuffer(const char *Ptr, size_t Size);

  // Write the data in the buffer (from the beginning of the buffer to the
  // current position).
  // Throws an exception if something is wrong.
  virtual void writeImpl() = 0;
};

} // namespace lucanthrope