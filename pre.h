/*
Copyright 2023 Dennis Bautembach

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace pre {
namespace detail {
// If T is a reference type, do nothing, otherwise add an lvalue reference.
template <class T>
using add_reference_t = std::conditional_t<std::is_reference_v<T>, T, T&>;
}  // namespace detail

template <auto Check, class... Ts>
struct cond {
  std::tuple<Ts...> params;

  // Construct precondition from one or more params, implicitly so in the case
  // of one input.
  // Call Check with all params.
  constexpr cond(Ts... params_) : params{std::forward<Ts>(params_)...} {
    std::apply(Check, params);
  }

  // Helpers
  template <int I>
  using ith_t = std::tuple_element_t<I, decltype(params)>;
  template <int I>
  using ith_return_t = detail::add_reference_t<ith_t<I>>;

  // Return the i-th input
  template <int I>
  constexpr ith_return_t<I> get() {
    return std::forward<ith_return_t<I>>(std::get<I>(params));
  }
  // Implicitly convert to the first input (requires sizeof...(Ts)==0)
  constexpr operator ith_return_t<0>()
    requires(sizeof...(Ts) == 1)
  {
    return get<0>();
  }
  // Member access operator for the first input (requires sizeof...(Ts)==0)
  constexpr auto* operator->()
    requires(sizeof...(Ts) == 1)
  {
    return &get<0>();
  }
  // Return the underlying tuple
  constexpr auto& operator*() { return params; }
};
}  // namespace pre