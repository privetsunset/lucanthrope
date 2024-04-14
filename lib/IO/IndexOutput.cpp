#include <cassert>
#include <cstring> // memcpy()

#include "IO/IndexOutput.h"

namespace lucanthrope {

IndexOutput &IndexOutput::writeByte(char c) {
  if (!available()) {
    if (!bufStart) {
      initInternalBuffer();
      return writeByte(c);
    }
    flushNonEmpty();
  }
  pos++;
  *bufCur++ = c;
  return *this;
}

IndexOutput &IndexOutput::write(const char *ptr, size_t size) {
  if (available() < size) {
    if (!bufStart) {
      initInternalBuffer();
      return write(ptr, size);
    }
    size_t avail = available();

    // We don't have enough space in the buffer to fit the string in. Insert as
    // much as possible, flush and start over with the remainder.
    copyToBuffer(ptr, avail);
    pos += avail;
    flushNonEmpty();
    return write(ptr + avail, size - avail);
  }
  copyToBuffer(ptr, size);
  pos += size;
  return *this;
}

void IndexOutput::copyToBuffer(const char *ptr, size_t size) {
  assert(size <= available() && "Buffer overrun!");

  // Handle short strings specially, memcpy isn't very good at very short
  // strings.
  switch (size) {
  case 4:
    bufCur[3] = ptr[3];
    [[fallthrough]];
  case 3:
    bufCur[2] = ptr[2];
    [[fallthrough]];
  case 2:
    bufCur[1] = ptr[1];
    [[fallthrough]];
  case 1:
    bufCur[0] = ptr[0];
    [[fallthrough]];
  case 0:
    break;
  default:
    std::memcpy(bufCur, ptr, size);
    break;
  }

  bufCur += size;
}

IndexOutput &IndexOutput::writeInt32(uint32_t num) {
  char buf[4];
  buf[0] = static_cast<char>(num);
  buf[1] = static_cast<char>(num >> 8);
  buf[2] = static_cast<char>(num >> 16);
  buf[3] = static_cast<char>(num >> 24);
  write(buf, 4);
  return *this;
}

IndexOutput &IndexOutput::writeInt64(uint64_t num) {
  char buf[8];
  buf[0] = static_cast<char>(num);
  buf[1] = static_cast<char>(num >> 8);
  buf[2] = static_cast<char>(num >> 16);
  buf[3] = static_cast<char>(num >> 24);
  buf[4] = static_cast<char>(num >> 32);
  buf[5] = static_cast<char>(num >> 40);
  buf[6] = static_cast<char>(num >> 48);
  buf[7] = static_cast<char>(num >> 56);
  write(buf, 8);
  return *this;
}

IndexOutput &IndexOutput::writeVarint32(uint32_t num) {
  uint8_t buf[kMaxVarintLength32];
  uint8_t *curr = buf;
  static const int B = 128;
  if (num < (1 << 7)) {
    *(curr++) = num;
  } else if (num < (1 << 14)) {
    *(curr++) = num | B;
    *(curr++) = num >> 7;
  } else if (num < (1 << 21)) {
    *(curr++) = num | B;
    *(curr++) = (num >> 7) | B;
    *(curr++) = num >> 14;
  } else if (num < (1 << 28)) {
    *(curr++) = (num >> 7) | B;
    *(curr++) = (num >> 14) | B;
    *(curr++) = num >> 21;
  } else {
    *(curr++) = num | B;
    *(curr++) = (num >> 7) | B;
    *(curr++) = (num >> 14) | B;
    *(curr++) = (num >> 21) | B;
    *(curr++) = num >> 28;
  }
  write(reinterpret_cast<char *>(buf), curr - buf);
  return *this;
}

IndexOutput &IndexOutput::writeVarint64(uint64_t num) {
  uint8_t buf[kMaxVarintLength64];
  uint8_t *ptr = buf;
  static const int B = 128;
  while (num >= B) {
    *(ptr++) = num | B;
    num >>= 7;
  }
  *(ptr++) = static_cast<uint8_t>(num);
  write(reinterpret_cast<char *>(buf), ptr - buf);
  return *this;
}

} // namespace lucanthrope