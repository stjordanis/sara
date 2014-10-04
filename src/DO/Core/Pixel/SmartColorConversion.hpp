#ifndef DO_CORE_PIXEL_SMARTCOLORCONVERSION_HPP
#define DO_CORE_PIXEL_SMARTCOLORCONVERSION_HPP


#include <DO/Core/Meta.hpp>
#include <DO/Core/Pixel/ChannelConversion.hpp>
#include <DO/Core/Pixel/ColorConversion.hpp>
#include <DO/Core/StaticAssert.hpp>


// Smart color conversion between colorspace regardless of the channel type.
// We will treat the grayscale conversion separately
namespace DO {

  //! \brief Smart color conversion from a colorspace to another.
  template <typename SrcColSpace, typename DstColSpace>
  inline void smart_convert_color(const Pixel<double, SrcColSpace>& src,
                                  Pixel<double, DstColSpace>& dst)
  {
    convert_color<double>(src, dst);
  }

  //! \brief Smart color conversion from a colorspace to another.
  template <typename SrcT, typename SrcColSpace, typename DstColSpace>
  inline void smart_convert_color(const Pixel<SrcT, SrcColSpace>& src,
                                  Pixel<double, DstColSpace>& dst)
  {
    Pixel<double, SrcColSpace> double_src;
    convert_channel(src, double_src);
    convert_color(double_src, dst);
  }

  //! \brief Smart color conversion from a colorspace to another.
  template <typename DstT, typename SrcColSpace, typename DstColSpace>
  inline void smart_convert_color(const Pixel<double, SrcColSpace>& src,
                                  Pixel<DstT, DstColSpace>& dst)
  {
    Pixel<double, DstColSpace> double_dst;
    convert_color(src, double_dst);
    convert_channel(double_dst, dst);
  }

  //! \brief Smart color conversion from a colorspace to another.
  template <
    typename SrcT, typename DstT, typename SrcColSpace, typename DstColSpace
  >
  inline void smart_convert_color(const Pixel<SrcT, SrcColSpace>& src,
                                  Pixel<DstT, DstColSpace>& dst)
  {
    Pixel<double, SrcColSpace> double_src;
    Pixel<double, DstColSpace> double_dst;
    convert_channel(src, double_src);
    convert_color(double_src, double_dst);
    convert_channel(double_dst, dst);
  }

}

// Smart color conversion from a colorspace to grayscale regardless of the
// channel type.
namespace DO {

  //! \brief Convert from 'double' pixel to 'double' grayscale.
  template <typename ColorSpace>
  inline void smart_convert_color(const Pixel<double, ColorSpace>& src,
                                  double& dst)
  {
    convert_color(src, dst);
  }

  //! \brief Convert from 'double' pixel to 'any' grayscale.
  template <typename T, typename ColorSpace>
  inline void smart_convert_color(const Pixel<double, ColorSpace>& src, T& dst)
  {
    double double_dst;
    convert_color(src, double_dst);
    convert_channel(double_dst, dst);
  }

  //! \brief Convert from 'any' pixel to 'double' grayscale.
  template <typename T, typename ColorSpace>
  inline void smart_convert_color(const Pixel<T, ColorSpace>& src, double& dst)
  {
    Pixel<double, ColorSpace> double_src;
    convert_channel(src, double_src);
    convert_color(double_src, dst);
  }

  //! \brief Convert from 'any' pixel to 'any' grayscale.
  template <typename T, typename U, typename ColorSpace>
  inline void smart_convert_color(const Pixel<T, ColorSpace>& src, U& dst)
  {
    Pixel<double, ColorSpace> double_src;
    double double_dst;
    convert_channel(src, double_src);
    convert_color(double_src, double_dst);
    convert_channel(double_dst, dst);
  }

}


// Smart color conversion from grayscale to another colorspace regardless of
// the channel type.
namespace DO {

  //! \brief Convert from 'double' grayscale to 'double' pixel.
  template <typename ColorSpace>
  inline void smart_convert_color(double src, Pixel<double, ColorSpace>& dst)
  {
    convert_color(src, dst);
  }

  //! \brief Convert from 'double' grayscale to 'any' pixel.
  template <typename T, typename ColorSpace>
  inline void smart_convert_color(double src, Pixel<T, ColorSpace>& dst)
  {
    Pixel<double, ColorSpace> double_dst;
    convert_color(src, double_dst);
    convert_channel(double_dst, dst);
  }

  //! \brief Convert from 'any' grayscale to 'double' pixel.
  template <typename T, typename ColorSpace>
  inline void smart_convert_color(T src, Pixel<double, ColorSpace>& dst)
  {
    double double_src;
    convert_channel(src, double_src);
    convert_color(double_src, dst);
  }

  //! \brief Convert from 'any' grayscale to 'any' pixel.
  template <typename T, typename U, typename ColorSpace>
  inline void smart_convert_color(T src, Pixel<U, ColorSpace>& dst)
  {
    double double_src;
    Pixel<double, ColorSpace> double_dst;
    convert_channel(src, double_src);
    convert_color(double_src, double_dst);
    convert_channel(double_dst, dst);
  }

}


// Smart color conversion from 'any' grayscale to 'any' grayscale.
namespace DO {

  //! \brief Convert from 'any' grayscale to 'any' grayscale.
  template <typename SrcGray, typename DstGray>
  inline void smart_convert_color(SrcGray src, DstGray& dst)
  {
    static const bool same_type = Meta::IsSame<SrcGray, DstGray>::value;
    DO_STATIC_ASSERT(!same_type, THE_GRAYSCALE_TYPES_ARE_IDENTICAL);
    convert_channel(src, dst);
  }

}


#endif /* DO_CORE_PIXEL_SMARTCOLORCONVERSION_HPP */