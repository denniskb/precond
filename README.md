# pre::cond Explicit <span style="color:maroon;">Pre</span><span style="color:navy;">cond</span>itions for Function Arguments
Instead of relying on comments, make your preconditions an explicit part of your function signatures (and check them at the same time):

```c++
int safe_divide(int x, pre::not_zero<int> y) {
  return x / y;
}

float safe_mean(pre::not_empy<const std::vector<float>&> v) {
  return std::reduce(v->begin(), v->end()) / v->size();
}

void plot(pre::same_size<std::span<const int>, std::span<const float>> xy) {
  auto [x, y] = *xy;
  ...
}
```

## Requirements
- C++20 compiler

## Setup
**1.** Copy `pre.h` into your project.

**2.** `pre::cond` does not come with any pre-defined conditions<sup>1</sup>. You must define your own. It's quick & easy! Example:

```c++
#include "pre.h"

#include <cassert>

template <class T>
using positive = pre::cond<[](auto x) { assert(x > 0); }, T>;

template <class T>
using not_zero = pre::cond<[](auto x) { assert(x != 0); }, T>;

template <class T>
using not_null = pre::cond<[](auto* p) { assert(p); }, T>;

template <class T>
using not_empty = pre::cond<[](auto&& cont) { assert(!cont.empty()); }, T>;

template <class... Ts>
using same_size = pre::cond<[](auto&&... conts) { assert(conts.size() == ...); }, Ts...>;
```

<sup>1</sup>This was a conscious decision. There are too many preferences as to how to handle violated preconditions (assert/throw/log error/print stacktrace/terminate, debug/release, ...).

**3.** `pre::cond` itself **should always be accepted by non-const value**. Instead, use the template arguments to qualify your parameters. You should use the exact same qualifiers as you would if you were accepting your parameters directly:

```c++
void func(const std::vector<object>& v)
                        |
                        v
void func(mycond<const std::vector<object>& v)
```

**4.** `pre::cond` has the same lifetime and value/reference semantics as a naked parameter with the same qualifiers would. Its interface is almost the same:

```c++
float safe_sqrt(pre::positive<float> x) {
  return std::sqrt(x);
}

float dot_product(pre::same_size<const std::vector<float>&,
                                 const std::vector<float>&> ab) {
  const auto& [a, b] = *ab;
  // alternatively:
  // const auto& a = ab.get<0>();
  // const auto& b = ab.get<1>();
  
  float result = 0;
  for (int i = 0; i < a.size(); i++)
    result += a[i] * b[i];
    
  return result;
}

auto bin_search(pre::sorted<span<const int>> list) {
  if (list->size() == 0)
    return list->begin();
    
  ...
}

auto take_ownership(pre::not_null<std::unique_ptr<...>&&> p) {
  // p is valid for the duration of this function
}
auto p = std::make_unique(...);
take_ownership(std::move(p)); // p is destructed after ';'
```

## API
Given `pre::cond<...> mycond{...};`:

- Return a reference to the underlying parameter tuple: `mycond.params` or `*mycond`.
- Return a reference to the i-th parameter: `mycond.get<i>()`
- Implicit conversion to first parameter: `auto& a = mycond;` (requires size==1).
- Member access operator: `mycond->...()` (requires size==1).

## TODO
- In the case where `pre::cond` wraps a single parameter, add common operators such as `[]`, ... (if the wrapped type supports them). This would allow you to do `param[i]` with `some_cond<some_list> param{...}`
- Nested conditions:

```c++
void finite_differences(
  pre::same_size<
    pre::monotonic<span<float>>,        // x, must be strictly increasing
    span<float>                         // f(x), must have same size as x
  > x_fx, span<float> out_derivatives);
```