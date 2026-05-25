// Like Arduino Stream.h but for files
#include <iostream>
#include "Stream.h"

class FileStream : public Stream {
public:
  FileStream(FILE *file) : _file(file) {
    fseek(_file, 0, SEEK_END);
    _size = ftell(_file);
    fseek(_file, 0, SEEK_SET);
  }
  void _update_size() {
    long current_pos = ftell(_file);
    if (current_pos > _size) {
      _size = current_pos;
    }
  }
  void flush() override {}
  bool outputCanTimeout() override { return false; }
  int availableForWrite() override { return INT_MAX; }
  size_t write(uint8_t c) override {
    size_t written = fputc(c, _file);
    _update_size();
    return written;
  }
  size_t write(const uint8_t *buffer, size_t size) override {
    size_t written = fwrite(buffer, 1, size, _file);
    _update_size();
    return written;
  }

  int available() override {
    long current_pos = ftell(_file);
    return current_pos < 0 ? 0 : (int)(_size - current_pos);
  }
  int read() override { return fgetc(_file); }
  int peek() override { return fgetc(_file); }
  bool eof() const { return feof(_file); }
  size_t bytesConsumed() const { return ftell(_file); }
  size_t size() { return _size; }
  template <typename... Args> size_t printf(const char *format, Args &&...args) {
    size_t written = fprintf(_file, format, std::forward<Args>(args)...);
    _update_size();
    return written;
  }
  template <size_t N> size_t printf(const char (&buffer)[N]) {
    size_t written = fprintf(_file, "%s", buffer);
    _update_size();
    return written;
  }
  template <size_t N> size_t println(const char (&buffer)[N]) {
    size_t written = fprintf(_file, "%s\n", buffer);
    _update_size();
    return written;
  }
private:
  FILE *_file;
  size_t _size;
};