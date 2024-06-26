// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <cassert>
#include <concepts>
#include <memory>
#include <ranges>
#include <span>
#include <utility>

#include <range_algorithm_support.hpp>
using namespace std;

template <class Range>
concept can_empty = requires(Range& r) { ranges::empty(r); };
template <class Range>
concept can_data = requires(Range& r) { ranges::data(r); };
template <class Range>
concept can_size = requires(Range& r) { ranges::size(r); };

struct instantiator {
    template <ranges::range R>
    static constexpr void call() {
        using ranges::ref_view, ranges::begin, ranges::end, ranges::forward_range;
        int input[3] = {0, 1, 2};

        { // traits
            static_assert(ranges::input_range<R> || ranges::output_range<R, const int&>);
            static_assert(ranges::enable_borrowed_range<ref_view<R>>);
        }

        { // constructors and assignment operators
            static_assert(!constructible_from<ref_view<R>, R>);

            R wrapped_input{input};
            ref_view<R> same_range{wrapped_input};
            static_assert(is_nothrow_constructible_v<ref_view<R>, R&>);

            auto copy_constructed = same_range;
            if constexpr (forward_range<R>) {
                assert(copy_constructed.begin().peek() == begin(input));
            }
            assert(copy_constructed.end().peek() == end(input));

            int other_data[3] = {4, 5, 6};
            R wrapped_other{other_data};
            ref_view<R> copy_assigned{wrapped_other};
            copy_assigned = copy_constructed;
            if constexpr (forward_range<R>) {
                assert(copy_assigned.begin().peek() == begin(input));
            }
            assert(copy_assigned.end().peek() == end(input));

            [[maybe_unused]] auto move_constructed = std::move(copy_assigned);
            if constexpr (forward_range<R>) {
                assert(move_constructed.begin().peek() == begin(input));
            }
            assert(move_constructed.end().peek() == end(input));

            same_range = std::move(copy_constructed);
            if constexpr (forward_range<R>) {
                assert(same_range.begin().peek() == begin(input));
            }
            assert(same_range.end().peek() == end(input));
        }

        { // access
            R wrapped_input{input};
            ref_view<R> test_view{wrapped_input};
            same_as<R> auto& base_range = as_const(test_view).base();
            assert(addressof(base_range) == addressof(wrapped_input));

            static_assert(noexcept(as_const(test_view).base()));
        }

        { // iterators
            R wrapped_input{input};
            ref_view<R> test_view{wrapped_input};
            const same_as<ranges::iterator_t<R>> auto first = as_const(test_view).begin();
            assert(first.peek() == input);
            static_assert(noexcept(as_const(test_view).begin()) == noexcept(wrapped_input.begin()));

            const same_as<ranges::sentinel_t<R>> auto last = as_const(test_view).end();
            assert(last.peek() == end(input));
            static_assert(noexcept(as_const(test_view).end()) == noexcept(wrapped_input.end()));
        }

#if _HAS_CXX23
        if constexpr (ranges::input_range<R>) { // const iterators (from view_interface)
            R wrapped_input{input};
            ref_view<R> test_view{wrapped_input};
            const same_as<ranges::const_iterator_t<R>> auto cfirst = as_const(test_view).cbegin();
            if constexpr (_Is_specialization_v<remove_const_t<decltype(cfirst)>, basic_const_iterator>) {
                assert(cfirst.base().peek() == begin(input));
            } else {
                assert(cfirst.peek() == begin(input));
            }

            const same_as<ranges::const_sentinel_t<R>> auto clast = as_const(test_view).cend();
            if constexpr (_Is_specialization_v<remove_const_t<decltype(clast)>, basic_const_iterator>) {
                assert(clast.base().peek() == end(input));
            } else {
                assert(clast.peek() == end(input));
            }
        }
#endif // _HAS_CXX23

        { // state
            static_assert(can_size<ref_view<R>> == ranges::sized_range<R>);
            if constexpr (ranges::sized_range<R>) {
                R wrapped_input{input};
                ref_view<R> test_view{wrapped_input};

                const same_as<ranges::range_size_t<R>> auto ref_size = as_const(test_view).size();
                assert(ref_size == size(wrapped_input));

                static_assert(noexcept(as_const(test_view).size()) == noexcept(wrapped_input.size()));
            }

            static_assert(can_data<ref_view<R>> == ranges::contiguous_range<R>);
            if constexpr (ranges::contiguous_range<R>) {
                R wrapped_input{input};
                ref_view<R> test_view{wrapped_input};

                const same_as<int*> auto ref_data = as_const(test_view).data();
                assert(ref_data == input);

                static_assert(noexcept(as_const(test_view).data()) == noexcept(wrapped_input.data()));
            }

            static_assert(can_empty<ref_view<R>> == can_empty<R>);
            if constexpr (can_empty<R>) {
                R wrapped_input{input};
                ref_view<R> test_view{wrapped_input};

                const same_as<bool> auto ref_empty = as_const(test_view).empty();
                assert(!ref_empty);

                static_assert(noexcept(as_const(test_view).empty()) == noexcept(ranges::empty(wrapped_input)));

                R empty_range{span<int, 0>{}};
                ref_view<R> empty_view{empty_range};
                assert(empty_view.empty());
            }
        }

        { // CTAD
            span<const int, 3> spanInput{input};
            ref_view span_view{spanInput};
            static_assert(same_as<decltype(span_view), ref_view<span<const int, 3>>>);
        }
    }
};

int main() {
    static_assert((test_inout<instantiator, int>(), true));
    test_inout<instantiator, int>();
}
