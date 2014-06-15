/*
@copyright Louis Dionne 2014
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#include <boost/hana/comparable.hpp>
#include <boost/hana/core.hpp>
#include <boost/hana/detail/static_assert.hpp>
#include <boost/hana/functional.hpp>
#include <boost/hana/functor.hpp>
#include <boost/hana/integral.hpp>
#include <boost/hana/list.hpp>
using namespace boost::hana;


struct Matrix;

template <typename Storage, typename = operators::enable>
struct matrix_type {
    using hana_datatype = Matrix;

    Storage rows_;
    constexpr auto columns() const
    { return length(head(rows_)); }

    constexpr auto rows() const
    { return length(rows_); }

    constexpr auto size() const
    { return rows() * columns(); }

    template <typename I, typename J>
    constexpr auto at(I i, J j) const
    { return boost::hana::at(j, boost::hana::at(i, rows_)); }
};

auto transpose = [](auto m) {
    auto new_storage = unpack(zip, m.rows_);
    return matrix_type<decltype(new_storage)>{new_storage};
};

auto elementwise = [](auto f) {
    return [=](auto ...matrices) {
        auto new_storage = zip_with(partial(zip_with, f), matrices.rows_...);
        return matrix_type<decltype(new_storage)>{new_storage};
    };
};

template <typename S1, typename S2>
constexpr auto operator+(matrix_type<S1> m1, matrix_type<S2> m2)
{ return elementwise(_+_)(m1, m2); }

template <typename S1, typename S2>
constexpr auto operator-(matrix_type<S1> m1, matrix_type<S2> m2)
{ return elementwise(_-_)(m1, m2); }

template <typename S1, typename S2>
constexpr auto operator*(matrix_type<S1> m1, matrix_type<S2> m2)
{ return elementwise(_*_)(m1, m2); }

template <typename S1, typename S2>
constexpr auto operator/(matrix_type<S1> m1, matrix_type<S2> m2)
{ return elementwise(_/_)(m1, m2); }


auto row = [](auto ...entries) {
    return list(entries...);
};

auto matrix = [](auto ...rows) {
    auto storage = list(rows...);
    auto all_same_length = all([=](auto row) {
        return length(row) == length(head(storage));
    }, tail(storage));
    static_assert(all_same_length,
    "");

    return matrix_type<decltype(storage)>{storage};
};

auto vector = on(matrix, row);

namespace boost { namespace hana {
    template <>
    struct Functor<Matrix> : defaults<Functor>::with<Matrix> {
        template <typename F, typename M>
        static constexpr auto fmap_impl(F f, M mat) {
            auto new_storage = fmap(partial(fmap, f), mat.rows_);
            return matrix_type<decltype(new_storage)>{new_storage};
        }
    };

    template <>
    struct Comparable<Matrix, Matrix> : defaults<Comparable>::with<Matrix, Matrix> {
        template <typename M1, typename M2>
        static constexpr auto equal_impl(M1 m1, M2 m2) {
            return m1.rows() == m2.rows() &&
                   m1.columns() == m2.columns() &&
                   all_of(zip_with(_==_, m1.rows_, m2.rows_));
        }
    };
}}


void test_sizes() {
    auto m = matrix(
        row(1, '2', 3),
        row('4', char_<'5'>, 6)
    );
    BOOST_HANA_STATIC_ASSERT(m.size() == 6);
    BOOST_HANA_STATIC_ASSERT(m.columns() == 3);
    BOOST_HANA_STATIC_ASSERT(m.rows() == 2);
}

void test_at() {
    auto m = matrix(
        row(1, '2', 3),
        row('4', char_<'5'>, 6),
        row(int_<7>, '8', 9.3)
    );
    BOOST_HANA_STATIC_ASSERT(m.at(int_<0>, int_<0>) == 1);
    BOOST_HANA_STATIC_ASSERT(m.at(int_<0>, int_<1>) == '2');
    BOOST_HANA_STATIC_ASSERT(m.at(int_<0>, int_<2>) == 3);

    BOOST_HANA_STATIC_ASSERT(m.at(int_<1>, int_<0>) == '4');
    BOOST_HANA_STATIC_ASSERT(m.at(int_<1>, int_<1>) == char_<'5'>);
    BOOST_HANA_STATIC_ASSERT(m.at(int_<1>, int_<2>) == 6);

    BOOST_HANA_STATIC_ASSERT(m.at(int_<2>, int_<0>) == int_<7>);
    BOOST_HANA_STATIC_ASSERT(m.at(int_<2>, int_<1>) == '8');
    BOOST_HANA_STATIC_ASSERT(m.at(int_<2>, int_<2>) == 9.3);
}

void test_comparable() {
    BOOST_HANA_STATIC_ASSERT(matrix(row(1, 2)) == matrix(row(1, 2)));
    BOOST_HANA_STATIC_ASSERT(matrix(row(1, 2)) != matrix(row(1, 5)));

    BOOST_HANA_STATIC_ASSERT(matrix(row(1, 2), row(3, 4)) == matrix(row(1, 2), row(3, 4)));
    BOOST_HANA_STATIC_ASSERT(matrix(row(1, 2), row(3, 4)) != matrix(row(1, 2), row(0, 4)));
    BOOST_HANA_STATIC_ASSERT(matrix(row(1, 2), row(3, 4)) != matrix(row(0, 2), row(3, 4)));

    BOOST_HANA_STATIC_ASSERT(matrix(row(1), row(2)) != matrix(row(3, 4), row(5, 6)));
    BOOST_HANA_STATIC_ASSERT(matrix(row(1), row(2)) != matrix(row(3, 4)));
}

void test_functor() {
    auto m = matrix(
        row(1, int_<2>, 3),
        row(int_<4>, 5, 6),
        row(7, 8, int_<9>)
    );
    BOOST_HANA_STATIC_ASSERT(fmap(_ + int_<1>, m) ==
        matrix(
            row(2, int_<3>, 4),
            row(int_<5>, 6, 7),
            row(8, 9, int_<10>)
        )
    );
}

void test_operators() {
    auto m = matrix(row(1, 2), row(3, 4));
    BOOST_HANA_STATIC_ASSERT(m + m == matrix(row(2, 4), row(6, 8)));
    BOOST_HANA_STATIC_ASSERT(m - m == matrix(row(0, 0), row(0, 0)));
}

void test_vector() {
    auto v = vector(1, '2', int_<3>, 4.2f);
    BOOST_HANA_STATIC_ASSERT(v.size() == 4);
    BOOST_HANA_STATIC_ASSERT(v.rows() == 4);
    BOOST_HANA_STATIC_ASSERT(v.columns() == 1);
}

void test_transpose() {
    auto m = matrix(
        row(1, 2.2, '3'),
        row(4, '5', 6)
    );
    BOOST_HANA_STATIC_ASSERT(transpose(m) ==
        matrix(
            row(1, 4),
            row(2.2, '5'),
            row('3', 6)
        )
    );
}

int main() {
    test_sizes();
    test_at();
    test_comparable();
    test_functor();
    test_operators();
    test_vector();
    test_transpose();
}
