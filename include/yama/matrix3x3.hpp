// Yama
// Copyright (c) 2016-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "dim.hpp"
#include "quaternion.hpp"

namespace yama
{

template <typename T>
class matrix3x3_t
{
public:
    T m00, m10, m20;
    T m01, m11, m21;
    T m02, m12, m22;

    typedef T value_type;
    typedef size_t size_type;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef typename std::reverse_iterator<iterator> reverse_iterator;
    typedef typename std::reverse_iterator<const_iterator> const_reverse_iterator;

    static constexpr size_type rows_count = 3;
    static constexpr size_type columns_count = 3;
    static constexpr size_type value_count = 9;

    constexpr size_type max_size() const { return value_count; }
    constexpr size_type size() const { return max_size(); }

    ///////////////////////////////////////////////////////////////////////////
    // named constructors

    static constexpr matrix3x3_t columns(
        const T& cr00, const T& cr01, const T& cr02, //column 0
        const T& cr10, const T& cr11, const T& cr12, //column 1
        const T& cr20, const T& cr21, const T& cr22 //column 2
    )
    {
        return{
            cr00, cr01, cr02,
            cr10, cr11, cr12,
            cr20, cr21, cr22,
        };
    }

    static constexpr matrix3x3_t rows(
        const T& rc00, const T& rc01, const T& rc02, //row 0
        const T& rc10, const T& rc11, const T& rc12, //row 1
        const T& rc20, const T& rc21, const T& rc22 //row 2
    )
    {
        return{
            rc00, rc10, rc20,
            rc01, rc11, rc21,
            rc02, rc12, rc22,
        };
    }

    static constexpr matrix3x3_t uniform(const value_type& s)
    {
        return columns(
            s, s, s,
            s, s, s,
            s, s, s
        );
    }

    static constexpr matrix3x3_t zero()
    {
        return uniform(value_type(0));
    }

    static matrix3x3_t from_ptr(const value_type* ptr)
    {
        YAMA_ASSERT_CRIT(ptr, "Constructing yama::matrix3x3_t from nullptr");
        return columns(
            ptr[0], ptr[1], ptr[2],
            ptr[3], ptr[4], ptr[5],
            ptr[6], ptr[7], ptr[8]
        );
    }

    static constexpr matrix3x3_t identity()
    {
        return columns(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        );
    }

    ////////////////////////////////////////////////////////
    // transforms

    static constexpr matrix3x3_t scaling_uniform(const value_type& s)
    {
        YAMA_ASSERT_WARN(!close(s, value_type(0)), "scale shouldn't be zero");
        return columns(
            s, 0, 0,
            0, s, 0,
            0, 0, s
        );
    }

    static constexpr matrix3x3_t scaling(const value_type& x, const value_type& y, const value_type& z)
    {
        YAMA_ASSERT_WARN(!close(x, value_type(0)), "scale shouldn't be zero");
        YAMA_ASSERT_WARN(!close(y, value_type(0)), "scale shouldn't be zero");
        YAMA_ASSERT_WARN(!close(z, value_type(0)), "scale shouldn't be zero");
        return columns(
            x, 0, 0,
            0, y, 0,
            0, 0, z
        );
    }

    static constexpr matrix3x3_t scaling(const vector3_t<T>& s)
    {
        return scaling(s.x, s.y, s.z);
    }

    // for when you're sure that the axis is normalized
    static matrix3x3_t rotation_naxis(const vector3_t<value_type>& axis, value_type radians)
    {
        YAMA_ASSERT_BAD(axis.is_normalized(), "rotation axis should be normalized");

        const value_type c = std::cos(radians);
        const value_type s = std::sin(radians);
        const value_type c1 = 1 - c;
        const value_type& x = axis.x;
        const value_type& y = axis.y;
        const value_type& z = axis.z;

        return rows(
            c + c1*sq(x), c1*y*x - s*z, c1*z*x + s*y,
            c1*x*y + s*z, c + c1*sq(y), c1*z*y - s*x,
            c1*x*z - s*y, c1*y*z + s*x, c + c1*sq(z)
        );
    }

    static matrix3x3_t rotation_axis(const vector3_t<value_type>& axis, value_type radians)
    {
        auto naxis = yama::normalize(axis);
        return rotation_naxis(naxis, radians);
    }

    static matrix3x3_t rotation_x(const value_type& radians)
    {
        const value_type c = std::cos(radians);
        const value_type s = std::sin(radians);

        return rows(
            1, 0,  0,
            0, c, -s,
            0, s,  c
        );
    }

    static matrix3x3_t rotation_y(const value_type& radians)
    {
        const value_type c = std::cos(radians);
        const value_type s = std::sin(radians);

        return rows(
            c, 0, s,
            0, 1, 0,
           -s, 0, c
        );
    }

    static matrix3x3_t rotation_z(const value_type& radians)
    {
        const value_type c = std::cos(radians);
        const value_type s = std::sin(radians);

        return rows(
            c, -s, 0,
            s,  c, 0,
            0,  0, 1
        );
    }

    static matrix3x3_t rotation_vectors(const vector3_t<value_type>& src, const vector3_t<value_type>& target)
    {
        YAMA_ASSERT_BAD(src.is_normalized(), "source vector should be normalized");
        YAMA_ASSERT_BAD(target.is_normalized(), "target vector should be normalized");
        YAMA_ASSERT_WARN(!close(src, vector3_t<value_type>::zero()), "source vector shouldn't be zero");
        YAMA_ASSERT_WARN(!close(target, vector3_t<value_type>::zero()), "target vector shouldn't be zero");

        auto axis = cross(src, target);
        auto axis_length = axis.length();

        if (axis_length > constants_t<value_type>::EPSILON()) // not collinear
        {
            axis /= axis_length; // normalize
            auto angle = acos(dot(src, target));
            return rotation_naxis(axis, angle);
        }
        else
        {
            if (close(src, target))
            {
                // collinear
                return identity();
            }
            else
            {
                // opposite
                auto o = normalize(src.get_orthogonal());

                return rows(
                    2*sq(o.x) - 1, 2*o.y*o.x, 2*o.z*o.x,
                    2*o.x*o.y, 2*sq(o.y) - 1, 2*o.z*o.y,
                    2*o.x*o.z, 2*o.y*o.z, 2*sq(o.z) - 1
                );
            }
        }
    }

    static matrix3x3_t rotation_quaternion(const quaternion_t<T>& q)
    {
        YAMA_ASSERT_BAD(q.is_normalized(), "rotation with a non-normalized quaternion");
        YAMA_ASSERT_WARN(!close(q.length_sq(), value_type(0)), "rotating with a broken quaternion");

        const value_type x2 = sq(q.x);
        const value_type y2 = sq(q.y);
        const value_type z2 = sq(q.z);
        const value_type w2 = sq(q.w);
        const value_type xy = 2 * q.x * q.y;
        const value_type xz = 2 * q.x * q.z;
        const value_type xw = 2 * q.x * q.w;
        const value_type yz = 2 * q.y * q.z;
        const value_type yw = 2 * q.y * q.w;
        const value_type zw = 2 * q.z * q.w;

        return rows(
            w2 + x2 - y2 - z2, xy - zw,           xz + yw,
            xy + zw,           w2 - x2 + y2 - z2, yz - xw,
            xz - yw,           yz + xw,           w2 - x2 - y2 + z2
        );
    }

    ///////////////////////////
    // attach
    static matrix3x3_t& attach_to_ptr(value_type* ptr)
    {
        YAMA_ASSERT_BAD(ptr, "Attaching yama::matrix3x3_t to nullptr");
        return *reinterpret_cast<matrix3x3_t*>(ptr);
    }

    static const matrix3x3_t& attach_to_ptr(const value_type* ptr)
    {
        YAMA_ASSERT_BAD(ptr, "Attaching yama::matrix3x3_t to nullptr");
        return *reinterpret_cast<const matrix3x3_t*>(ptr);
    }

    static matrix3x3_t* attach_to_array(value_type* ptr)
    {
        YAMA_ASSERT_WARN(ptr, "Attaching yama::matrix3x3_t to nullptr");
        return reinterpret_cast<matrix3x3_t*>(ptr);
    }

    static const matrix3x3_t* attach_to_array(const value_type* ptr)
    {
        YAMA_ASSERT_WARN(ptr, "Attaching yama::matrix3x3_t to nullptr");
        return reinterpret_cast<const matrix3x3_t*>(ptr);
    }

    ///////////////////////////////////////////////////////////////////////////
    // access
    value_type* data()
    {
        return reinterpret_cast<value_type*>(this);
    }

    constexpr const value_type* data() const
    {
        return reinterpret_cast<const value_type*>(this);
    }

    value_type& at(size_type i)
    {
        YAMA_ASSERT_CRIT(i < value_count, "yama::matrix3x3_t index overflow");
        return data()[i];
    }

    constexpr const value_type& at(size_type i) const
    {
        YAMA_ASSERT_CRIT(i < value_count, "yama::matrix3x3_t index overflow");
        return data()[i];
    }

    value_type& operator[](size_type i)
    {
        return at(i);
    }

    constexpr const value_type& operator[](size_type i) const
    {
        return at(i);
    }

    value_type* column(size_t i)
    {
        YAMA_ASSERT_CRIT(i < columns_count, "yama::matrix3x3_t column index overflow");
        return data() + rows_count * i;
    }

    constexpr const value_type* column(size_t i) const
    {
        YAMA_ASSERT_CRIT(i < columns_count, "yama::matrix3x3_t column index overflow");
        return data() + rows_count * i;
    }

    value_type& m(size_t row, size_t col)
    {
        return column(col)[row];
    }

    constexpr const value_type& m(size_t row, size_t col) const
    {
        return column(col)[row];
    }

    value_type& operator()(size_t row, size_t col)
    {
        return m(row, col);
    }

    constexpr const value_type& operator()(size_t row, size_t col) const
    {
        return m(row, col);
    }

    template <size_t D>
    typename dim<D>::template vector_t<value_type>& column_vector(size_t col, size_t offset = 0)
    {
        static_assert(D <= columns_count, "yama::matrix3x3_t column_vector out of range");
        YAMA_ASSERT_BAD(D + offset <= rows_count, "yama::matrix3x3_t column_vector reaching end of column");
        return dim<D>::template vector_t<value_type>::attach_to_ptr(column(col) + offset);
    }

    template <size_t D>
    const typename dim<D>::template vector_t<value_type>& column_vector(size_t col, size_t offset = 0) const
    {
        static_assert(D <= columns_count, "yama::matrix3x3_t column_vector out of range");
        YAMA_ASSERT_BAD(D + offset <= rows_count, "yama::matrix3x3_t column_vector reaching end of column");
        return dim<D>::template vector_t<value_type>::attach_to_ptr(column(col) + offset);
    }

    vector3_t<value_type>& column_vector(size_t col)
    {
        return vector3_t<value_type>::attach_to_ptr(column(col));
    }

    const vector3_t<value_type>& column_vector(size_t col) const
    {
        return vector3_t<value_type>::attach_to_ptr(column(col));
    }

    template <size_t D>
    typename dim<D>::template vector_t<value_type> row_vector(size_t row, size_t offset = 0) const
    {
        static_assert(D < rows_count, "yama::matrix3x3_t row_vector out of range");
        YAMA_ASSERT_BAD(D + offset <= columns_count, "yama::matrix3x3_t row_vector reaching end of column");
        typename dim<D>::template vector_t<value_type> ret;
        for (size_t i = 0; i < D; ++i)
        {
            ret.at(i) = column(offset + i)[row];
        }
        return ret;
    }

    vector3_t<value_type> row_vector(size_t row) const
    {
        return vector3_t<value_type>::coord(m(row, 0), m(row, 1), m(row, 2));
    }

    template <size_t D>
    typename dim<D>::template vector_t<value_type> main_diagonal(size_t offset = 0) const
    {
        static_assert(D < rows_count, "yama::matrix3x3_t row_vector out of range");
        YAMA_ASSERT_BAD(D + offset <= columns_count, "yama::matrix3x3_t row_vector reaching end of column");
        typename dim<D>::template vector_t<value_type> ret;
        for (size_t i = 0; i < D; ++i)
        {
            ret.at(i) = m(offset+i, offset+i);
        }
        return ret;
    }

    vector3_t<value_type> main_diagonal() const
    {
        return vector3_t<value_type>::coord(m(0, 0), m(1, 1), m(2, 2));
    }

    ///////////////////////////
    // cast

    value_type* as_ptr()
    {
        return data();
    }

    const value_type* as_ptr() const
    {
        return data();
    }

    template <typename S>
    matrix3x3_t<S> as_matrix3x3_t() const
    {
        return matrix3x3_t<S>::columns(
            S(m00), S(m10), S(m20),
            S(m01), S(m11), S(m21),
            S(m02), S(m12), S(m22)
        );
    }

    ///////////////////////////
    // std

    iterator begin()
    {
        return data();
    }

    iterator end()
    {
        return data() + value_count;
    }

    const_iterator begin() const
    {
        return data();
    }

    const_iterator end() const
    {
        return data() + value_count;
    }

    value_type& front()
    {
        return at(0);
    }

    value_type& back()
    {
        return at(value_count - 1);
    }

    constexpr const value_type& front() const
    {
        return at(0);
    }

    constexpr const value_type& back() const
    {
        return at(value_count - 1);
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    const_reverse_iterator crbegin() const { return rbegin(); }
    const_reverse_iterator crend() const { return rend(); }

    ///////////////////////////////////////////////////////////////////////////
    // arithmetic

    constexpr const matrix3x3_t& operator+() const
    {
        return *this;
    }

    constexpr matrix3x3_t operator-() const
    {
        return columns(
            -m00, -m10, -m20,
            -m01, -m11, -m21,
            -m02, -m12, -m22
        );
    }

    matrix3x3_t& operator+=(const matrix3x3_t& b)
    {
        m00 += b.m00; m10 += b.m10; m20 += b.m20;
        m01 += b.m01; m11 += b.m11; m21 += b.m21;
        m02 += b.m02; m12 += b.m12; m22 += b.m22;
        return *this;
    }

    matrix3x3_t& operator-=(const matrix3x3_t& b)
    {
        m00 -= b.m00; m10 -= b.m10; m20 -= b.m20;
        m01 -= b.m01; m11 -= b.m11; m21 -= b.m21;
        m02 -= b.m02; m12 -= b.m12; m22 -= b.m22;
        return *this;
    }

    matrix3x3_t& operator*=(const value_type& s)
    {
        m00 *= s; m10 *= s; m20 *= s;
        m01 *= s; m11 *= s; m21 *= s;
        m02 *= s; m12 *= s; m22 *= s;
        return *this;
    }

    matrix3x3_t& operator/=(const value_type& s)
    {
        YAMA_ASSERT_WARN(s != 0, "yama::matrix3x3_t division by zero");
        m00 /= s; m10 /= s; m20 /= s;
        m01 /= s; m11 /= s; m21 /= s;
        m02 /= s; m12 /= s; m22 /= s;
        return *this;
    }

    matrix3x3_t& operator*=(const matrix3x3_t& b)
    {
        auto c00 = m00 * b.m00 + m01 * b.m10 + m02 * b.m20;
        auto c10 = m10 * b.m00 + m11 * b.m10 + m12 * b.m20;
        auto c20 = m20 * b.m00 + m21 * b.m10 + m22 * b.m20;
        auto c01 = m00 * b.m01 + m01 * b.m11 + m02 * b.m21;
        auto c11 = m10 * b.m01 + m11 * b.m11 + m12 * b.m21;
        auto c21 = m20 * b.m01 + m21 * b.m11 + m22 * b.m21;
        auto c02 = m00 * b.m02 + m01 * b.m12 + m02 * b.m22;
        auto c12 = m10 * b.m02 + m11 * b.m12 + m12 * b.m22;
        auto c22 = m20 * b.m02 + m21 * b.m12 + m22 * b.m22;

        m00 = c00; m10 = c10; m20 = c20;
        m01 = c01; m11 = c11; m21 = c21;
        m02 = c02; m12 = c12; m22 = c22;

        return *this;
    }

    matrix3x3_t& mul(const matrix3x3_t& b)
    {
        m00 *= b.m00; m10 *= b.m10; m20 *= b.m20;
        m01 *= b.m01; m11 *= b.m11; m21 *= b.m21;
        m02 *= b.m02; m12 *= b.m12; m22 *= b.m22;
        return *this;
    }

    matrix3x3_t& div(const matrix3x3_t& b)
    {
        m00 /= b.m00; m10 /= b.m10; m20 /= b.m20;
        m01 /= b.m01; m11 /= b.m11; m21 /= b.m21;
        m02 /= b.m02; m12 /= b.m12; m22 /= b.m22;
        return *this;
    }

    matrix3x3_t& transpose()
    {
        std::swap(m10, m01);
        std::swap(m20, m02);
        std::swap(m21, m12);
        return *this;
    }

    value_type determinant() const
    {
        return
          -(m02*m11*m20)+ m01*m12*m20 + m02*m10*m21 -
            m00*m12*m21 - m01*m10*m22 + m00*m11*m22;
    }

    // returns determinant
    value_type inverse()
    {
        auto det = determinant();

        auto c00 = -m12*m21 + m11*m22;
        auto c10 =  m12*m20 - m10*m22;
        auto c20 = -m11*m20 + m10*m21;
        auto c01 =  m02*m21 - m01*m22;
        auto c11 = -m02*m20 + m00*m22;
        auto c21 =  m01*m20 - m00*m21;
        auto c02 = -m02*m11 + m01*m12;
        auto c12 =  m02*m10 - m00*m12;
        auto c22 = -m01*m10 + m00*m11;

        m00 = c00 / det; m10 = c10 / det; m20 = c20 / det;
        m01 = c01 / det; m11 = c11 / det; m21 = c21 / det;
        m02 = c02 / det; m12 = c12 / det; m22 = c22 / det;

        return det;
    }
};

template <typename T>
bool operator==(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return
        a.m00 == b.m00 && a.m10 == b.m10 && a.m20 == b.m20 &&
        a.m01 == b.m01 && a.m11 == b.m11 && a.m21 == b.m21 &&
        a.m02 == b.m02 && a.m12 == b.m12 && a.m22 == b.m22;
}

template <typename T>
bool operator!=(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return
        a.m00 != b.m00 || a.m10 != b.m10 || a.m20 != b.m20 ||
        a.m01 != b.m01 || a.m11 != b.m11 || a.m21 != b.m21 ||
        a.m02 != b.m02 || a.m12 != b.m12 || a.m22 != b.m22;
}

template <typename T>
bool close(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b, const T& epsilon = constants_t<T>::EPSILON())
{
    return
        close(a.m00, b.m00, epsilon) && close(a.m10, b.m10, epsilon) && close(a.m20, b.m20, epsilon) &&
        close(a.m01, b.m01, epsilon) && close(a.m11, b.m11, epsilon) && close(a.m21, b.m21, epsilon) &&
        close(a.m02, b.m02, epsilon) && close(a.m12, b.m12, epsilon) && close(a.m22, b.m22, epsilon);
}

template <typename T>
matrix3x3_t<T> operator+(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        a.m00 + b.m00, a.m10 + b.m10, a.m20 + b.m20,
        a.m01 + b.m01, a.m11 + b.m11, a.m21 + b.m21,
        a.m02 + b.m02, a.m12 + b.m12, a.m22 + b.m22
    );
}

template <typename T>
matrix3x3_t<T> operator-(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        a.m00 - b.m00, a.m10 - b.m10, a.m20 - b.m20,
        a.m01 - b.m01, a.m11 - b.m11, a.m21 - b.m21,
        a.m02 - b.m02, a.m12 - b.m12, a.m22 - b.m22
    );
}

template <typename T>
matrix3x3_t<T> operator*(const matrix3x3_t<T>& a, const T& s)
{
    return matrix3x3_t<T>::columns(
        a.m00 * s, a.m10 * s, a.m20 * s,
        a.m01 * s, a.m11 * s, a.m21 * s,
        a.m02 * s, a.m12 * s, a.m22 * s
    );
}

template <typename T>
matrix3x3_t<T> operator*(const T& s, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        s * b.m00, s * b.m10, s * b.m20,
        s * b.m01, s * b.m11, s * b.m21,
        s * b.m02, s * b.m12, s * b.m22
    );
}

template <typename T>
matrix3x3_t<T> operator/(const matrix3x3_t<T>& a, const T& s)
{
    return matrix3x3_t<T>::columns(
        a.m00 / s, a.m10 / s, a.m20 / s,
        a.m01 / s, a.m11 / s, a.m21 / s,
        a.m02 / s, a.m12 / s, a.m22 / s
    );
}

template <typename T>
matrix3x3_t<T> operator/(const T& s, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        s / b.m00, s / b.m10, s / b.m20,
        s / b.m01, s / b.m11, s / b.m21,
        s / b.m02, s / b.m12, s / b.m22
    );
}

template <typename T>
matrix3x3_t<T> operator*(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20,
        a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20,
        a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20,
        a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21,
        a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21,
        a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21,
        a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22,
        a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22,
        a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22
    );
}

template <typename T>
matrix3x3_t<T> abs(const matrix3x3_t<T>& a)
{
    return matrix3x3_t<T>::columns(
        std::abs(a.m00), std::abs(a.m10), std::abs(a.m20),
        std::abs(a.m01), std::abs(a.m11), std::abs(a.m21),
        std::abs(a.m02), std::abs(a.m12), std::abs(a.m22)
    );
}

template <typename T>
matrix3x3_t<T> mul(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        a.m00 * b.m00, a.m10 * b.m10, a.m20 * b.m20,
        a.m01 * b.m01, a.m11 * b.m11, a.m21 * b.m21,
        a.m02 * b.m02, a.m12 * b.m12, a.m22 * b.m22
    );
}

template <typename T>
matrix3x3_t<T> div(const matrix3x3_t<T>& a, const matrix3x3_t<T>& b)
{
    return matrix3x3_t<T>::columns(
        a.m00 / b.m00, a.m10 / b.m10, a.m20 / b.m20,
        a.m01 / b.m01, a.m11 / b.m11, a.m21 / b.m21,
        a.m02 / b.m02, a.m12 / b.m12, a.m22 / b.m22
    );
}

template <typename T>
bool isfinite(const matrix3x3_t<T>& a)
{
    return std::isfinite(a.m00) && std::isfinite(a.m10) && std::isfinite(a.m20) &&
           std::isfinite(a.m01) && std::isfinite(a.m11) && std::isfinite(a.m21) &&
           std::isfinite(a.m02) && std::isfinite(a.m12) && std::isfinite(a.m22);
}

template <typename T>
matrix3x3_t<T> inverse(const matrix3x3_t<T>& a, T& out_determinant)
{
    out_determinant = a.determinant();

    return matrix3x3_t<T>::columns(
        (-a.m12*a.m21 + a.m11*a.m22) / out_determinant,
        ( a.m12*a.m20 - a.m10*a.m22) / out_determinant,
        (-a.m11*a.m20 + a.m10*a.m21) / out_determinant,
        ( a.m02*a.m21 - a.m01*a.m22) / out_determinant,
        (-a.m02*a.m20 + a.m00*a.m22) / out_determinant,
        ( a.m01*a.m20 - a.m00*a.m21) / out_determinant,
        (-a.m02*a.m11 + a.m01*a.m12) / out_determinant,
        ( a.m02*a.m10 - a.m00*a.m12) / out_determinant,
        (-a.m01*a.m10 + a.m00*a.m11) / out_determinant
    );
}

template <typename T>
matrix3x3_t<T> inverse(const matrix3x3_t<T>& a)
{
    T det;
    return inverse(a, det);
}

// type traits
template <typename T>
struct is_yama<matrix3x3_t<T>> : public std::true_type {};

template <typename T>
struct is_matrix<matrix3x3_t<T>> : public std::true_type {};

// shorthand
#if !defined(YAMA_NO_SHORTHAND)

typedef matrix3x3_t<preferred_type> matrix3x3;
typedef matrix3x3 matrix3;

#endif

}
