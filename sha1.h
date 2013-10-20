
#ifndef __TASKFORCE_SHA1_H__
#define __TASKFORCE_SHA1_H__

#include <stdint.h>
#include <stddef.h>
#include <string>

class SHA1 {
 public:
  SHA1();
  void Digest(const void* data, size_t len);
  void Finish(unsigned char out[20]);

 private:
  void DigestBlock();
  int block_bytes_;
  uint32_t block_[16];
  uint32_t state_[5];
};

inline std::string GetSHA1(const std::string& str) {
  SHA1 sha1;
  unsigned char out[20];
  sha1.Digest(str.c_str(), str.size());
  sha1.Finish(out);
  std::string outstr((char*)out, 20);
  return outstr;
}

#endif
