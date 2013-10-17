#include <cstdio>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

extern "C" {
# include <libjpeg/jpeglib.h>
# include <libpng/png.h>
# include <libtiff/libtiff/tiffio.h>
# include <setjmp.h>
}

class FileError : public std::exception
{
  std::string filepath_;
  std::string mode_;
public:
  FileError(const std::string& filepath, const std::string& mode);
  virtual ~FileError() throw () {}
  virtual const char * what() const throw();
};

class FileHandler
{
protected:
  FILE* file_;
public:
  FileHandler() : file_(NULL) {}
  FileHandler(const std::string& filepath, const std::string& mode);
  virtual ~FileHandler();
};

class ImageFileReader : public FileHandler
{
public:
  ImageFileReader() : FileHandler() {}
  ImageFileReader(const std::string& filepath, const std::string& mode)
    : FileHandler(filepath, mode) {};
  virtual ~ImageFileReader() {};
  virtual bool operator()(unsigned char *& data,
                          int& width, int& height, int& depth) = 0;
};

class ImageFileWriter : public FileHandler
{
protected:
  const unsigned char *data_;
  const int width_, height_, depth_;
public:
  ImageFileWriter(const unsigned char *data,
                  int width, int height, int depth)
    : FileHandler()
    , data_(data)
    , width_(width), height_(height), depth_(depth) {}
  virtual ~ImageFileWriter() {}
  virtual bool operator()(const std::string& filepath, int quality) = 0;
};

struct JpegErrorMessage {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

class JpegFileReader : public ImageFileReader
{
  struct jpeg_decompress_struct cinfo_;
  struct JpegErrorMessage jerr_;
public:
  //! TODO: make better exception?
  JpegFileReader(const std::string& filepath);
  ~JpegFileReader();
  bool operator()(unsigned char *& data, int& width, int& height, int& depth);
};

class JpegFileWriter : public ImageFileWriter
{
  struct jpeg_compress_struct cinfo_;
  struct jpeg_error_mgr jerr_;
public:
  JpegFileWriter(const unsigned char *data, int width, int height, int depth);
  ~JpegFileWriter();
  bool operator()(const std::string& filepath, int quality);
};

class PngFileReader : public ImageFileReader
{
  png_structp png_ptr;
  png_infop info_ptr;
public:
  PngFileReader(const std::string& filepath);
  ~PngFileReader();
  bool operator()(unsigned char *& data, int& width, int& height, int& depth);
};

class PngFileWriter : public ImageFileWriter
{
  png_structp png_ptr;
  png_infop info_ptr;
public:
  PngFileWriter(const unsigned char *data, int width, int height, int depth);
  ~PngFileWriter();
  bool operator()(const std::string& filepath, int quality);
};

class TiffFileReader : public ImageFileReader
{
  TIFF *tiff_;
public:
  TiffFileReader(const std::string& filepath);
  ~TiffFileReader();
  bool operator()(unsigned char *& data, int& width, int& height, int& depth);
};

class TiffFileWriter : public ImageFileWriter
{
  TIFF *out;
public:
  TiffFileWriter(const unsigned char *data, int width, int height, int depth);
  ~TiffFileWriter();
  //! Quick-and-dirty but it works.
  bool operator()(const std::string& filepath, int quality);
};