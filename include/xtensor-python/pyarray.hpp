/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef PY_ARRAY_HPP
#define PY_ARRAY_HPP

#include <cstddef>
#include <algorithm>

#include "pybind11/numpy.h"

#include "xtensor/xexpression.hpp"
#include "xtensor/xsemantic.hpp"
#include "xtensor/xiterator.hpp"

namespace xt
{

    using pybind_array = pybind11::array;
    using buffer_info = pybind11::buffer_info;

    /***********************
     * pyarray declaration *
     ***********************/

    template <class T, int ExtraFlags>
    class pyarray;

    template <class T, int ExtraFlags>
    struct array_inner_types<pyarray<T, ExtraFlags>>
    {
        using temporary_type = pyarray<T, ExtraFlags>;
    };

    /**
     * @class pyarray
     * @brief Wrapper on the Python buffer protocol.
     */
    template <class T, int ExtraFlags = pybind_array::forcecast>
    class pyarray : public pybind_array,
                     public xarray_semantic<pyarray<T, ExtraFlags>>
    {

    public:

        using self_type = pyarray<T, ExtraFlags>;
        using base_type = pybind_array;
        using semantic_base = xarray_semantic<self_type>;
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using stepper = xstepper<T>;
        using const_stepper = xstepper<const T>;

        using iterator = xiterator<stepper>;
        using const_iterator = xiterator<const_stepper>;
        
        using storage_iterator = T*;
        using const_storage_iterator = const T*;

        using shape_type = xshape<size_type>;
        using strides_type = xstrides<size_type>;

        using closure_type = const self_type&;

        PYBIND11_OBJECT_CVT(pyarray, pybind_array, is_non_null, m_ptr = ensure_(m_ptr));

        pyarray();

        explicit pyarray(const buffer_info& info);

        pyarray(const xshape<size_type>& shape,
                const xstrides<size_type>& strides, 
                const T* ptr = nullptr,
                handle base = handle());

        explicit pyarray(const xshape<size_type>& shape, 
                         const T* ptr = nullptr,
                         handle base = handle());

        explicit pyarray(size_type count,
                         const T* ptr = nullptr,
                         handle base = handle());

        auto dimension() const -> size_type;

        template<typename... Args>
        auto operator()(Args... args) -> reference;

        template<typename... Args>
        auto operator()(Args... args) const -> const_reference;

        template<typename... Args>
        auto data(Args... args) -> pointer;

        template<typename... Args>
        auto data(Args... args) const -> const_pointer;

        bool broadcast_shape(shape_type& shape) const;
        bool is_trivial_broadcast(const strides_type& strides) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        iterator xbegin(const shape_type& shape);
        iterator xend(const shape_type& shape);

        const_iterator xbegin(const shape_type& shape) const;
        const_iterator xend(const shape_type& shape) const;
        const_iterator cxbegin(const shape_type& shape) const;
        const_iterator cxend(const shape_type& shape) const;

        stepper stepper_begin(const shape_type& shape);
        stepper stepper_end(const shape_type& shape);

        const_stepper stepper_begin(const shape_type& shape) const;
        const_stepper stepper_end(const shape_type& shape) const;

        storage_iterator storage_begin();
        storage_iterator storage_end();

        const_storage_iterator storage_begin() const;
        const_storage_iterator storage_end() const;

        shape_type shape() const;

        template <class E>
        pyarray(const xexpression<E>& e);

        template <class E>
        pyarray& operator=(const xexpression<E>& e);

    private:

        template<typename... Args>
        auto index_at(Args... args) const -> size_type;

        static constexpr auto itemsize() -> size_type;

        static bool is_non_null(PyObject* ptr);

        static PyObject *ensure_(PyObject* ptr);

    };

    /**************************
     * pyarray implementation *
     **************************/

    template <class T, int ExtraFlags>
    inline pyarray<T, ExtraFlags>::pyarray()
         : pybind_array()
    {
    }

    template <class T, int ExtraFlags>
    inline pyarray<T, ExtraFlags>::pyarray(const buffer_info& info)
        : pybind_array(info)
    {
    }

    template <class T, int ExtraFlags>
    inline pyarray<T, ExtraFlags>::pyarray(const xshape<size_type>& shape,
                                           const xstrides<size_type>& strides, 
                                           const T *ptr,
                                           handle base)
        : pybind_array(shape, strides, ptr, base)
    {
    }

    template <class T, int ExtraFlags>
    inline pyarray<T, ExtraFlags>::pyarray(const xshape<size_type>& shape, 
                                           const T* ptr,
                                           handle base)
        : pybind_array(shape, ptr, base)
    {
    }

    template <class T, int ExtraFlags>
    inline pyarray<T, ExtraFlags>::pyarray(size_type count,
                                           const T* ptr,
                                           handle base)
        : pybind_array(count, ptr, base)
    {
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::dimension() const -> size_type
    {
        return pybind_array::ndim();
    }

    template <class T, int ExtraFlags>
    template<typename... Args> 
    inline auto pyarray<T, ExtraFlags>::operator()(Args... args) -> reference
    {
        if (sizeof...(args) != dimension())
        {
            pybind_array::fail_dim_check(sizeof...(args), "index dimension mismatch");
        }
        // not using pybind_array::offset_at() / index_at() here so as to avoid another dimension check.
        return *(static_cast<pointer>(pybind_array::mutable_data()) + pybind_array::get_byte_offset(args...) / itemsize());
    }

    template <class T, int ExtraFlags>
    template<typename... Args> 
    inline auto pyarray<T, ExtraFlags>::operator()(Args... args) const -> const_reference
    {
        if (sizeof...(args) != dimension())
        {
            pybind_array::fail_dim_check(sizeof...(args), "index dimension mismatch");
        }
        // not using pybind_array::offset_at() / index_at() here so as to avoid another dimension check.
        return *(static_cast<const_pointer>(pybind_array::data()) + pybind_array::get_byte_offset(args...) / itemsize());
    }

    template <class T, int ExtraFlags>
    template<typename... Args> 
    inline auto pyarray<T, ExtraFlags>::data(Args... args) -> pointer
    {
        return static_cast<pointer>(pybind_array::mutable_data(args...));
    }

    template <class T, int ExtraFlags>
    template<typename... Args>
    inline auto pyarray<T, ExtraFlags>::data(Args... args) const -> const_pointer
    {
        return static_cast<const T*>(pybind_array::data(args...));
    }
    
    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::begin() -> iterator
    {
        return xbegin(shape());
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::end() -> iterator
    {
        return xend(shape());
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::begin() const -> const_iterator
    {
        return xbegin(shape());
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::end() const -> const_iterator
    {
        return xend(shape());
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::cbegin() const -> const_iterator
    {
        return begin();
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::cend() const -> const_iterator
    {
        return end();
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::xbegin(const shape_type& shape) -> iterator
    {
        return iterator(stepper_begin(shape), shape);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::xend(const shape_type& shape) -> iterator
    {
        return iterator(stepper_end(shape), shape);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::xbegin(const shape_type& shape) const -> const_iterator
    {
        return const_iterator(stepper_begin(shape), shape);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::xend(const shape_type& shape) const -> const_iterator
    {
        return const_iterator(stepper_end(shape), shape);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::cxbegin(const shape_type& shape) const -> const_iterator
    {
        return xbegin(shape);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::cxend(const shape_type& shape) const -> const_iterator
    {
        return xend(shape);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::stepper_begin(const shape_type& shape) -> stepper
    {
        size_type offset = shape.size() - dimension();
        return stepper(this, storage_begin(), offset);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::stepper_end(const shape_type& shape) -> stepper
    {
        size_type offset = shape.size() - dimension();
        return stepper(this, storage_end(), offset);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::stepper_begin(const shape_type& shape) const -> const_stepper
    {
        size_type offset = shape.size() - dimension();
        return const_stepper(this, storage_begin(), offset);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::stepper_end(const shape_type& shape) const -> const_stepper
    {
        size_type offset = shape.size() - dimension();
        return const_stepper(this, storage_end(), offset);
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::storage_begin() -> storage_iterator
    {
        return static_cast<storage_iterator>(PyArray_GET_(m_ptr, data));
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::storage_end() -> storage_iterator
    {
        return storage_begin() + pybind_array::size();
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::storage_begin() const -> const_storage_iterator
    {
        return static_cast<const_storage_iterator>(PyArray_GET_(m_ptr, data));
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::storage_end() const -> const_storage_iterator
    {
        return storage_begin() + pybind_array::size();
    }

    template <class T, int ExtraFlags>
    inline auto pyarray<T, ExtraFlags>::shape() const -> shape_type
    {
        // Until we have the CRTP on shape types, we copy the shape.
        shape_type shape(dimension());
        std::copy(pybind_array::shape(), pybind_array::shape() + dimension(), shape.begin());
        return shape;
    }

    template <class T, int ExtraFlags>
    template <class E>
    inline pyarray<T, ExtraFlags>::pyarray(const xexpression<E>& e)
         : pybind_array()
    {
        semantic_base::operator=(e);
    }

    template <class T, int ExtraFlags>
    template <class E>
    inline auto pyarray<T, ExtraFlags>::operator=(const xexpression<E>& e) -> self_type&
    {
        return semantic_base::operator=(e);
    }

    // Private methods

    template <class T, int ExtraFlags>
    template<typename... Args> 
    inline auto pyarray<T, ExtraFlags>::index_at(Args... args) const -> size_type
    {
        return pybind_array::offset_at(args...) / itemsize();
    }

    template <class T, int ExtraFlags>
    constexpr auto pyarray<T, ExtraFlags>::itemsize() -> size_type
    {
        return sizeof(value_type);
    }

    template <class T, int ExtraFlags>
    inline bool pyarray<T, ExtraFlags>::is_non_null(PyObject* ptr)
    {
        return ptr != nullptr;
    }

    template <class T, int ExtraFlags>
    inline PyObject* pyarray<T, ExtraFlags>::ensure_(PyObject* ptr)
    {
        if (ptr == nullptr)
        {
            return nullptr;
        }
        auto& api = pybind11::detail::npy_api::get();
        PyObject *result = api.PyArray_FromAny_(ptr, pybind11::dtype::of<T>().release().ptr(), 0, 0,
                                                pybind11::detail::npy_api::NPY_ENSURE_ARRAY_ | ExtraFlags, nullptr);
        if (!result)
        {
            PyErr_Clear();
        }
        Py_DECREF(ptr);
        return result;
    }

}

#endif

