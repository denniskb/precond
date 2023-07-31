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

#include <cstddef>
#include <type_traits>
#include <utility>

namespace pre {

namespace detail {
template <class T, class... Ts>
struct first {
  using type = T;
};
template <class... Ts>
using first_t = typename first<Ts...>::type;
}  // namespace detail

template <class T, auto Check>
struct cond {
  T value;

  template <class... Args>
  constexpr cond(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>&& noexcept(Check(value)))
    requires((sizeof...(Args) > 1) ||
             sizeof...(Args) == 1 &&
                 !std::is_same_v<std::decay_t<T>,
                                 std::decay_t<detail::first_t<Args...>>>)
      : value(std::forward<Args>(args)...) {
    Check(value);
  }
  constexpr cond(T value_) noexcept(
      std::is_nothrow_constructible_v<T, T>&& noexcept(Check(value_)))
      : value{std::forward<T>(value_)} {
    Check(value);
  }

  cond(const cond&) = delete;
  cond(cond&&) = delete;
  cond& operator=(const cond&) = delete;
  cond& operator=(cond&&) = delete;

  constexpr operator T&() & noexcept { return value; }
  constexpr operator T&&() && noexcept { return std::move(value); }
  constexpr auto* operator->() noexcept { return &value; }

  template <class U>
  constexpr cond& operator=(U&& u)
    requires(std::is_assignable_v<T, U>)
  {
    value = std::forward<U>(u);
    return *this;
  }

  constexpr auto& operator[](std::size_t i)
    requires(requires { value[0]; })
  {
    return value[i];
  }

  constexpr decltype(auto) operator()()
    requires(requires { value(); })
  {
    return value();
  }
};
}  // namespace pre