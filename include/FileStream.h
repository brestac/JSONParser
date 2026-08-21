#pragma once
// Like Arduino Stream.h but for files
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits.h>

#include "Stream.h"

class File : public Stream {
public:
  explicit File(FILE *file, const char *mode = "r") : _file(file), _size(0) {
    if (_file == nullptr) return;
  
    if (strcmp(mode, "r") == 0) {
      fseek(_file, 0, SEEK_END);
      _size = ftell(_file);
      fseek(_file, 0, SEEK_SET);
    }
  }

  File(const File &) = delete;
  File &operator=(const File &) = delete;

  File(File &&other) noexcept : _file(other._file), _size(other._size) {
    other._file = nullptr;
    other._size = 0;
  }

  File &operator=(File &&other) noexcept {
    if (this != &other) {
      close();
      _file = other._file;
      _size = other._size;
      other._file = nullptr;
      other._size = 0;
    }
    return *this;
  }

  ~File() {
    close();
  }

  void close() {
    if (_file) {
      fseek(_file, 0, SEEK_SET);
      fclose(_file);
      _file = nullptr;
      _size = 0;
    }
  }

  void seek(size_t pos) { fseek(_file, pos, SEEK_SET); }

  void _update_size() {
    long current_pos = ftell(_file);
    if (current_pos >= 0 && (size_t)current_pos > _size) {
      _size = (size_t)current_pos;
    }
  }
  void flush() override {}
  bool outputCanTimeout() override { return false; }
  bool inputCanTimeout() override { return false; }
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

  size_t read(uint8_t* buffer, size_t maxLen) override {
    return fread(buffer, 1, maxLen, _file);
  }

  size_t readBytes(uint8_t* buffer, size_t length) override {
    return fread(buffer, 1, length, _file);
  }

  int peek() override {
    int c = fgetc(_file);
    if (c != EOF) ungetc(c, _file);
    return c;
  }
  bool eof() const { return feof(_file); }
  size_t bytesConsumed() const { return ftell(_file); }
  size_t size() { return _size; }

  template <typename... Args>
  size_t printf(const char *format, Args &&...args) {
    size_t written = fprintf(_file, format, std::forward<Args>(args)...);
    _update_size();
    return written;
  }

  template <size_t N> size_t print(const char (&buffer)[N]) {
    size_t written = fprintf(_file, "%s", buffer);
    _update_size();
    return written;
  }

  template <size_t N> size_t println(const char (&buffer)[N]) {
    size_t written = fprintf(_file, "%s\n", buffer);
    _update_size();
    return written;
  }

  bool operator!() const { return _file == nullptr; }
  operator bool() const { return _file != nullptr; }

  operator File*() { return this; }

private:
  FILE *_file;
  size_t _size;
};

class FileSystemImpl {
public:
  File open(const char *filename, const char *mode) {
    FILE *f = fopen(filename, mode);

    if (f == nullptr) {
      return File(nullptr);
    }

    return File(f, mode);
  }

  bool exists(const char *filename) {
    return std::filesystem::exists(filename);
  }

  bool remove(const char *filename) { return std::remove(filename); }
};

FileSystemImpl LittleFS;