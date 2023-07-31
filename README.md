# pre::cond Explicit <span style="color:maroon;">Pre</span><span style="color:navy;">cond</span>itions for Function Arguments
Instead of relying on comments, make your preconditions an explicit part of your function signatures (and check them at the same time):

```c++
int safe_divide(int x, pre::not_zero<int> y) {
  return x / y;
}

float safe_mean(pre::not_empy<const std::vector<float>&> v) {
  return std::reduce(v->begin(), v->end()) / v->size();
}

float safe_modf(float num, pre::not_null<float*> iptr) {...}
```

## Requirements
- C++20 compiler

## Setup
**1.** Copy `pre.h` into your project.

**2.** `pre::cond` comes with the following default preconditions:

- `not_zero`
- `positive`
- `not_null`
- `not_empty`
- `sorted`

They are located in the namespaces `pre::assert_on_fail` and `pre::throw_on_fail`. The first set `assert()`'s their conditions, the latter throws a `std::logic_error` if a condition is violated. You could use either namespace:

```c++
void func() {
  using namespace pre::throw_on_fail;
  
  auto lambda = [](positive<int> x) {...};
  ...
}
```

or create an alias:

```c++
namespace check = pre::assert_on_fail;

void func(check::not_zero<int> x) {...}
```

You can also define your own preconditions, it's quick & easy! Example:

```c++
#include "pre.h"

#include <cassert>

template <class T>
using not_zero = pre::cond<T, [](auto i) { assert(i != T{0}); }>;

template <class T>
using positive = pre::cond<T, [](auto i) { assert(i > T{0}); }>;

template <class T>
using not_null = pre::cond<T, [](auto* p) { assert(p != nullptr); }>;

template <class T>
using not_empty = pre::cond<T, [](auto&& cont) { assert(!cont.empty()); }>;
```

**3.** `pre::cond` itself **should always be accepted by non-const value**. Instead, use the template arguments to qualify your parameters. You should use the exact same qualifiers as you would if you were accepting your parameters directly:

```c++
void func(const std::vector<object>& v)
                        |
                        v
void func(mycond<const std::vector<object>&> v)
```

**4.** `pre::cond` has the same lifetime and value/reference semantics and the same interface as a naked parameter would:

```c++
float safe_sqrt(pre::positive<float> x) {
  return std::sqrt(x);
}

auto bin_search(pre::sorted<span<const int>> list) {
  if (list->empty())
    return list->end();
  
  while (...) {
    ...
    auto x = list[mid];
    ...
  }
}

auto take_ownership(pre::not_null<std::unique_ptr<...>&&> p) {
  // p is valid for the duration of this function
}
auto p = std::make_unique(...);
take_ownership(std::move(p)); // p is destructed after ';'

float modf(float num, pre::not_null<float*> iptr) {
  ...
  *iptr = ...;
  return ...;
}

void glutIdleFunc(pre::not_null<void(*)()> callback) {
  ...
  callback();
  ...
}
```

## API
Given `pre::cond<...> mycond{...};`:

- Access the stored value/reference: `mycond.value`
- Implicit conversion to `T`: `x = mycond;` (equivalent to `x = mycond.value;`)
- Member access operator: `mycond->...` (equivalent to `mycond.value. ...`)
- Assignment: `mycond = ...;` (equivalent to `mycond.value = ...`)
- Subscript operator: `mycond[i]` (equivalent to `mycond.value[i]`)
- Function call operator: `mycond(...)` (equivalent to `mycond.value(...)`)