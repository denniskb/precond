# pre::cond Explicit <span style="color:maroon;">Pre</span><span style="color:navy;">cond</span>itions for Function Arguments
Instead of relying on comments, make your preconditions an explicit part of your function signatures (and check them at the same time):

```c++
int safe_divide(int x, pre::not_zero<int> y) {
  return x / y;
}

float safe_mean(pre::not_empy<const std::vector<float>&> v) {
  return std::reduce(v->begin(), v->end()) / v->size();
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
```

<sup>1</sup>This was a conscious decision. There are too many preferences as to how to handle violated preconditions (assert/throw/log error/print stacktrace/terminate, debug/release, ...).

**3.** `pre::cond` itself **should always be accepted by non-const value**. Instead, use the template arguments to qualify your parameters. You should use the exact same qualifiers as you would if you were accepting your parameters directly:

```c++
void func(const std::vector<object>& v)
                        |
                        v
void func(mycond<const std::vector<object>&> v)
```

**4.** `pre::cond` has the same lifetime and value/reference semantics as a naked parameter with the same qualifiers would. Its interface is almost the same:

```c++
float safe_sqrt(pre::positive<float> x) {
  return std::sqrt(x);
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

- Access the stored value/reference: `mycond.value`.
- Implicit conversion to `T`: `auto& a = mycond;`
- Member access operator: `mycond->...()`

## TODO
- In the case where `pre::cond` wraps a single parameter, add common operators such as `[]`, ... (if the wrapped type supports them). This would allow you to do `param[i]` with `some_cond<some_list> param{...}`
- Nested function calls with conditions