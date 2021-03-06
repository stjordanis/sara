// ========================================================================== //
// This file is part of Sara, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2015-2019 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

//! @file

#pragma once

#include <DO/Sara/Core/ArrayIterators.hpp>
#include <DO/Sara/Core/MultiArray/ElementTraits.hpp>

#include <cstdint>
#include <numeric>


namespace DO { namespace Sara {

  //! @ingroup Drafts
  //! @defgroup DynamicArrayView Dynamic Array Views
  //! @{

  //! @{
  //! @brief Forward declaration of the multi-dimensional array classes.
  template <typename T, int StorageOrder = ColMajor>
  class DynamicArrayView;

  template <typename DynamicArrayView, template <typename> class Allocator>
  class DynamicArrayBase;

  template <typename T, int StorageOrder = ColMajor,
            template <typename> class Allocator = std::allocator>
  using DynamicArray =
      DynamicArrayBase<DynamicArrayView<T, StorageOrder>, Allocator>;
  //! @}


  template <typename T, int S>
  class DynamicArrayView
  {
    using self_type = DynamicArrayView;

  public: /* typedefs. */
    //! Storage order.
    enum
    {
      Dimension = N,
      StorageOrder = S
    };

    //! @{
    //! @brief STL-compatible interface.
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T *;
    using const_iterator = const T *;
    //! @}

    //! @{
    //! @brief Slice type.
    using slice_type = DynamicArrayView<T, N - 1, S>;
    using const_slice_type = const DynamicArrayView<T, N - 1, S>;
    //! @}

    //! @{
    //! @brief Vector type.
    using vector_type = Matrix<int, N, 1>;
    using slice_vector_type = Matrix<int, N - 1, 1>;
    //! @}

    //! @{
    //! @brief N-dimensional iterator type.
    using array_iterator = ArrayIterator<false, T, N, StorageOrder>;
    using const_array_iterator = ArrayIterator<true, T, N, StorageOrder>;
    //! @}

    //! @{
    //! @brief N-dimensional subrange iterator.
    using subarray_iterator = SubarrayIterator<false, T, N, StorageOrder>;
    using const_subarray_iterator = SubarrayIterator<true, T, N, StorageOrder>;
    //! @}

    //! @{
    //! @brief N-dimensional subrange iterator.
    using stepped_nd_iterator =
        SteppedSubarrayIterator<false, T, N, StorageOrder>;
    using const_stepped_nd_iterator =
        SteppedSubarrayIterator<true, T, N, StorageOrder>;
    //! @}

    //! @{
    //! @brief Array views for linear algebra.
    using flat_array_view_type =
        Map<Array<typename ElementTraits<T>::value_type, Dynamic, 1>>;
    using const_flat_array_view_type =
        Map<const Array<typename ElementTraits<T>::value_type, Dynamic, 1>>;
    //! @}

    //! @{
    //! @brief Matrix views for linear algebra.
    using matrix_view_type = Map<Matrix<typename ElementTraits<T>::value_type,
                                        Dynamic, Dynamic, StorageOrder>>;
    using const_matrix_view_type =
        Map<const Matrix<typename ElementTraits<T>::value_type, Dynamic,
                         Dynamic, StorageOrder>>;
    //! @}

    //! @{
    //! @brief Vector views for linear algebra.
    using vector_view_type =
        Map<Matrix<typename ElementTraits<T>::value_type, Dynamic, 1>>;

    using const_vector_view_type =
        Map<const Matrix<typename ElementTraits<T>::value_type, Dynamic, 1>>;

    using row_vector_view_type =
        Map<Matrix<typename ElementTraits<T>::value_type, 1, Dynamic>>;

    using const_row_vector_view_type =
        Map<const Matrix<typename ElementTraits<T>::value_type, 1, Dynamic>>;
    //! @}

  public: /* methods */
    //! @brief Default constructor.
    inline DynamicArrayView() = default;

    //! @brief Copy constructor.
    inline DynamicArrayView(const self_type&) = default;

    //! @brief Move constructor.
    inline DynamicArrayView(self_type&& other)
    {
      swap(other);
    }

    //! @brief Constructor that wraps plain data with its known sizes.
    inline explicit DynamicArrayView(value_type *data, const vector_type& sizes)
      : _begin{data}
      , _end{data + compute_size<Dimension>(sizes)}
      , _sizes{sizes}
      , _strides{compute_strides(sizes)}
    {
    }

    //! @brief Return the size vector of the DynamicArray object.
    const vector_type& sizes() const
    {
      return _sizes;
    }

    //! @brief Return the number of elements in the internal data array.
    size_type size() const
    {
      return _end - _begin;
    }

    //! @brief Return the size of the DynamicArray object along the i-th
    //! dimension.
    int size(int i) const
    {
      return _sizes[i];
    }

    //! @brief Return the number of rows.
    int rows() const
    {
      return _sizes[0];
    }

    //! @brief Return the number of cols.
    int cols() const
    {
      return _sizes[1];
    }

    //! @brief Return the depth size.
    int depth() const
    {
      return _sizes[2];
    }

    //! @brief Return the stride vector of the DynamicArray object.
    inline const vector_type& strides() const
    {
      return _strides;
    }

    //! @brief Return the stride value along the i-th dimension.
    inline int stride(int i) const
    {
      return _strides[i];
    }

    //! @{
    //! @brief Return the array pointer
    inline pointer data()
    {
      return _begin;
    }

    inline const_pointer data() const
    {
      return _begin;
    }
    //! @}

    //! @{
    //! @brief Return reference to the element at the given coordinates.
    inline reference operator()(const vector_type& pos)
    {
      return _begin[offset(pos)];
    }

    inline reference operator()(int i, int j)
    {
      static_assert(N == 2, "DynamicArray must be 2D");
      return _begin[offset(Vector2i(i, j))];
    }

    inline reference operator()(int i, int j, int k)
    {
      static_assert(N == 3, "DynamicArray must be 3D");
      return _begin[offset(Vector3i(i, j, k))];
    }

    inline const_reference operator()(const vector_type& pos) const
    {
      return _begin[offset(pos)];
    }

    inline const_reference operator()(int i, int j) const
    {
      static_assert(N == 2, "DynamicArray must be 2D");
      return _begin[offset(Vector2i(i, j))];
    }

    inline const_reference operator()(int i, int j, int k) const
    {
      static_assert(N == 3, "DynamicArray must be 3D");
      return _begin[offset(Vector3i(i, j, k))];
    }
    //! @}

    //! @{
    //! @brief Return the i-th slice of the DynamicArray object.
    inline slice_type operator[](int i)
    {
      auto slice_sizes = _sizes.tail(N - 1).eval();
      auto slice_data = _begin + _strides[0] * i;
      return slice_type{slice_data, slice_sizes};
    }

    inline const_slice_type operator[](int i) const
    {
      auto slice_sizes = _sizes.tail(N - 1).eval();
      auto slice_data = _begin + _strides[0] * i;
      return slice_type{slice_data, slice_sizes};
    }
    //! @}

    //! @{
    //! @brief Return the begin iterator.
    inline iterator begin()
    {
      return _begin;
    }

    inline const_iterator begin() const
    {
      return _begin;
    }
    //! @}

    //! @{
    //! @brief Return the end iterator.
    inline iterator end()
    {
      return _end;
    }

    inline const_iterator end() const
    {
      return _end;
    }
    //! @}

    //! @{
    //! @brief Return the array view for linear algebra with Eigen libraries.
    inline flat_array_view_type flat_array()
    {
      return flat_array_view_type{
          reinterpret_cast<typename ElementTraits<T>::pointer>(data()),
          static_cast<int64_t>(size())};
    }

    inline const_flat_array_view_type flat_array() const
    {
      return const_flat_array_view_type{
          reinterpret_cast<const typename ElementTraits<T>::const_pointer>(
              data()),
          static_cast<int64_t>(size())};
    }
    //! @}

    //! @{
    //! @brief Apply the vectorization math operation and return a vector view
    //! object ! for linear algebra with Eigen libraries.
    inline auto vector() -> vector_view_type
    {
      return {reinterpret_cast<typename ElementTraits<T>::pointer>(data()),
              static_cast<int64_t>(size())};
    }

    inline auto vector() const -> const_vector_view_type
    {
      return {
          reinterpret_cast<typename ElementTraits<T>::const_pointer>(data()),
          static_cast<int64_t>(size())};
    }

    inline auto row_vector() -> row_vector_view_type
    {
      return {reinterpret_cast<typename ElementTraits<T>::pointer>(data()),
              static_cast<int64_t>(size())};
    }

    inline auto row_vector() const -> const_row_vector_view_type
    {
      return {
          reinterpret_cast<typename ElementTraits<T>::const_pointer>(data()),
          static_cast<int64_t>(size())};
    }
    //! @}

    //! @{
    //! @brief Return the matrix view for linear algebra with Eigen libraries.
    inline matrix_view_type matrix()
    {
      static_assert(N == 2, "DynamicArray must be 2D");
      return matrix_view_type{
          reinterpret_cast<typename ElementTraits<T>::pointer>(data()), rows(),
          cols()};
    }

    inline const_matrix_view_type matrix() const
    {
      static_assert(N == 2, "DynamicArray must be 2D");
      return const_matrix_view_type{
          reinterpret_cast<typename ElementTraits<T>::const_pointer>(data()),
          rows(), cols()};
    }
    //! @}

    //! @{
    //! @brief Return the begin iterator of the whole multi-array.
    inline array_iterator begin_array()
    {
      return array_iterator{false, _begin, vector_type::Zero(), _sizes,
                            _strides};
    }

    inline const_array_iterator begin_array() const
    {
      return const_array_iterator{false, _begin, vector_type::Zero(), _sizes,
                                  _strides};
    }
    //! @}

    //! @{
    //! @brief Return the begin iterator of the sub-array.
    inline subarray_iterator begin_subarray(const vector_type& start,
                                            const vector_type& end)
    {
      return subarray_iterator{false, _begin, start, end, _strides, _sizes};
    }

    inline const_subarray_iterator begin_subarray(const vector_type& start,
                                                  const vector_type& end) const
    {
      return const_subarray_iterator{false, _begin,   start,
                                     end,   _strides, _sizes};
    }
    //! @}

    //! @{
    //! @brief Return the begin stepped iterator of the sub-array.
    inline auto begin_stepped_subarray(const vector_type& start,
                                       const vector_type& end,
                                       const vector_type& steps)
        -> stepped_nd_iterator
    {
      return {false, _begin, start, end, _strides, _sizes, steps};
    }

    inline auto begin_stepped_subarray(const vector_type& start,
                                       const vector_type& end,
                                       const vector_type& steps) const
        -> const_stepped_nd_iterator
    {
      return {false, _begin, start, end, _strides, _sizes, steps};
    }
    //! @}

    //! @}
    //! @brief Swap multi-array objects.
    inline void swap(self_type& other)
    {
      using std::swap;
      swap(_begin, other._begin);
      swap(_end, other._end);
      swap(_sizes, other._sizes);
      swap(_strides, other._strides);
    }

    //! @{
    //! @brief Equality comparison.
    inline bool operator==(const self_type& other) const
    {
      if (_sizes != other._sizes)
        return false;
      return std::equal(_begin, _end, other._begin);
    }

    inline bool operator!=(const self_type& other) const
    {
      return !(*this == other);
    }
    //! @}

    //! @{
    //! @brief Copy the source array deeply like std::copy.
    /*!
     *
     *  Throws if the source array view sizes does not match the destination
     *  array.
     *
     */
    inline void copy(const self_type& other) const
    {
      if (this == &other)
        return;

      if (_sizes != other._sizes)
        throw std::domain_error{
            "Source and destination image sizes are not equal!"};

      std::copy(other._begin, other._end, _begin);
    }

    self_type& operator=(self_type other)
    {
      copy(other);
      return *this;
    }
    //! @}

    //! @brief Perform coefficient-wise transform in place.
    template <typename Op>
    inline auto cwise_transform_inplace(Op op) -> self_type&
    {
      for (auto pixel = begin(); pixel != end(); ++pixel)
        op(*pixel);
      return *this;
    }

    //! @brief Perform coefficient-wise transform.
    template <typename Op>
    inline auto cwise_transform(Op op) const
        -> DynamicArray<decltype(op(std::declval<value_type>())), Dimension,
                      StorageOrder>
    {
      using ValueType = decltype(op(std::declval<value_type>()));

      auto dst = DynamicArray<ValueType, N, S>{sizes()};

      auto src_pixel = begin();
      auto dst_pixel = dst.begin();
      for (; src_pixel != end(); ++src_pixel, ++dst_pixel)
        *dst_pixel = op(*src_pixel);

      return dst;
    }

    //! @brief Reshape the array with the new sizes.
    template <typename Array>
    inline auto reshape(const Array& new_sizes) const
        -> DynamicArrayView<T, ElementTraits<Array>::size, StorageOrder>
    {
      constexpr int D = ElementTraits<Array>::size;
      if (compute_size<D>(new_sizes) != size())
        throw std::domain_error{"Invalid shape!"};
      return DynamicArrayView<T, D, StorageOrder>{const_cast<T*>(_begin),
                                                new_sizes};
    }

    //! @brief Transpose the array.
    inline auto transpose(const vector_type& order) const
      -> DynamicArray<T, Dimension, StorageOrder>
    {
      auto out_sizes = vector_type{};
      for (int i = 0; i < Dimension; ++i)
        out_sizes[i] = this->size(order[i]);

      auto out = DynamicArray<T, Dimension, StorageOrder>{out_sizes};

      auto in_it = begin_array();
      vector_type out_coord = Matrix<int, N, 1>::Zero();

      for (; !in_it.end(); ++in_it)
      {
        for (int i = 0; i < Dimension; ++i)
          out_coord[i] = in_it.position()[order[i]];

        out(out_coord) = *in_it;
      }

      return out;
    }

    //! @{
    //! @brief Reverse the storage-order view.
    inline auto colmajor_view() const -> DynamicArrayView<T, N, ColMajor>
    {
      static_assert(StorageOrder == static_cast<int>(RowMajor),
                    "Don't use this on a column-major DynamicArrayView object");
      auto sizes = this->_sizes;
      std::reverse(sizes.data(), sizes.data() + sizes.size());
      return DynamicArrayView<T, N, ColMajor>{_begin, sizes};
    }

    inline auto rowmajor_view() const -> DynamicArrayView<T, N, RowMajor>
    {
      static_assert(StorageOrder == static_cast<int>(ColMajor),
                    "Don't use this on a row-major DynamicArrayView object");
      auto sizes = this->_sizes;
      std::reverse(sizes.data(), sizes.data() + sizes.size());
      return DynamicArrayView<T, N, RowMajor>{_begin, sizes};
    }
    //! @}

  protected:
    //! @brief Compute the strides according the size vector and storage order.
    inline vector_type compute_strides(const vector_type& sizes) const
    {
      return StrideComputer<StorageOrder>::eval(sizes);
    }

    //! @brief Compute the raw size needed to allocate the internal data.
    template <int M>
    inline size_type compute_size(const Matrix<int, M, 1>& sizes) const
    {
      auto sz = sizes.template cast<size_type>().eval();
      return std::accumulate(sz.data(), sz.data() + sz.size(), size_type(1),
                             std::multiplies<size_type>());
    }

    //! @brief Compute the 1D index of the corresponding coordinates.
    inline int offset(const vector_type& pos) const
    {
      return jump(pos, _strides);
    }

  protected: /* data members. */
    //! @brief First element of the internal array.
    value_type *_begin{nullptr};
    //! @brief Last element of the internal array.
    value_type *_end{nullptr};
    //! @brief Sizes vector.
    vector_type _sizes{vector_type::Zero().eval()};
    //! @brief Strides vector.
    vector_type _strides{vector_type::Zero().eval()};
  };


  //! @brief Output stream operator.
  template <typename T, int N, int StorageOrder>
  std::ostream& operator<<(std::ostream& os,
                           const DynamicArrayView<T, N, StorageOrder>& M)
  {
    os << M.sizes() << std::endl;
    os << M.array() << std::endl;
    return os;
  }

  //! @}

} /* namespace Sara */
} /* namespace DO */
