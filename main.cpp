#include <cassert>
#include <vector>

#include "pre.h"

#define myassert(x) \
  if (!(x)) throw 1;

void assert_throw(auto func) {
  bool threw = false;
  try {
    func();
  } catch (...) {
    threw = true;
  }
  assert(threw);
}

void assert_nothrow(auto func) {
  bool threw = false;
  try {
    func();
  } catch (...) {
    threw = true;
  }
  assert(!threw);
}

template <class T>
using any = pre::cond<T, [](auto&&) {}>;

template <class T>
using positive = pre::cond<T, [](auto x) { myassert(x > 0); }>;

template <class T>
using not_zero = pre::cond<T, [](auto x) { myassert(x != 0); }>;

template <class T>
using not_null = pre::cond<T, [](auto* p) { myassert(p); }>;

template <class T>
using not_empty = pre::cond<T, [](auto&& cont) { myassert(!cont.empty()); }>;

struct object {
  int ncopies = 0;
  bool* destroyed = nullptr;
  object(bool* destroyed_ = nullptr) : destroyed{destroyed_} {};
  object(const object&) { ncopies++; }
  object& operator=(const object&) {
    ncopies++;
    return *this;
  }
  object(object&&) = default;
  object& operator=(object&&) = default;
  ~object() {
    if (destroyed) *destroyed = true;
  }
};

template <int I>
struct constexpr_int {
  constexpr operator int() const { return I; }
  constexpr constexpr_int<I + 1> operator++() const { return {}; }
};

template <int I>
void constexpr_for(auto func) {
  auto loop = [](auto& self, auto index, auto func) {
    func(index);
    if constexpr (index + 1 < I) self(self, ++index, func);
  };
  if constexpr (I > 0) loop(loop, constexpr_int<0>{}, func);
}

template <class T>
constexpr bool is_movable =
    std::is_object_v<T> && std::is_move_constructible_v<T>;

int main() {
  // clang-format off
  //
  // What wrapper type is constructible from what parameter type?
  //
  // WRAPPER TYPE       |                       PARAMETER TYPE
  //                    | const object | const objec& | object& | object&&
  // ---------------------------------------------------------------------
  // pre<object>        |      ✓       |       ✓      |    ✓    |    ✓ 
  // pre<const ojbect>  |      ✓       |       ✓      |    ✓    |    ✓
  // pre<const object&> |      ✓       |       ✓      |    ✓    |    ✓
  // pre<object&>       |      ✗       |       ✗      |    ✓    |    ✗
  // pre<object&&>      |      ✗       |       ✗      |    ✗    |    ✓
  //
  // clang-format on
  {
    // clang-format off
    constexpr bool table[] = {
		true , true , true , true, 
		true , true , true , true,
		true , true , true , true, 
		true, false, true , false, 
		true, false, false, true};
    // clang-format on

    using rows_t = std::tuple<any<object>, any<const object>,
                              any<const object&>, any<object&>, any<object&&>>;

    using cols_t = std::tuple<const object, const object&, object&, object&&>;

    constexpr_for<5>([](auto i_row) {
      constexpr_for<0>([&](auto i_col) {
        constexpr int index = i_row * 4 + i_col;
        constexpr bool constructible =
            std::is_constructible_v<std::tuple_element_t<i_row, rows_t>,
                                    std::tuple_element_t<i_col, cols_t>>;

        static_assert(constructible == table[index]);
      });
    });
  }

  // clang-format off
  //
  // What object type is constructible from what wrapper type?
  //
  // OBJECT TYPE        |                                      WRAPPER TYPE
  //                    | any<object> | any<const object> | any<const object&> | any<object&> | any<object&&>
  // --------------------------------------------------------------------------------------------------------
  // object             |      ✓      |         ✓         |          ✓         |       ✓      |       ✓ 
  // const object       |      ✓      |         ✓         |          ✓         |       ✓      |       ✓ 
  // const object&      |      ✓      |         ✓         |          ✓         |       ✓      |       ✓
  // object&            |      ✓      |         ✗         |          ✗         |       ✓      |       ✗
  // object&&           |      ✗      |         ✗         |          ✗         |       ✗      |       ✓
  //
  // clang-format on
  {
    // clang-format off
    constexpr bool table[] = {
		true , true , true , true , true,
		true , true , true , true , true,
		true , true , true , true , true,
		true , false, false, true , false, 
		false, false, false, false, true};
    // clang-format on

    using rows_t =
        std::tuple<object, const object, const object&, object&, object&&>;

    using cols_t = std::tuple<any<object>, any<const object>,
                              any<const object&>, any<object&>, any<object&&>>;

    constexpr_for<0>([](auto i_row) {
      constexpr_for<5>([&](auto i_col) {
        constexpr int index = i_row * 5 + i_col;
        constexpr bool constructible =
            std::is_constructible_v<std::tuple_element_t<i_row, rows_t>,
                                    std::tuple_element_t<i_col, cols_t>>;
        static_assert(constructible == table[index]);
      });
    });
  }

  {
    // What wrapper types can be moved from?
    static_assert(is_movable<any<object>>);
    static_assert(is_movable<any<object&>>);
    static_assert(is_movable<any<object&&>>);
  }

  // checking functionality
  {
    assert_nothrow([] { positive<int>{5}; });
    assert_throw([] { positive<int>{-5}; });

    assert_nothrow([] { not_zero<int>{5}; });
    assert_throw([] { not_zero<int>{0}; });

    assert_nothrow([] { not_empty<std::vector<int>>{std::vector<int>(5)}; });
    assert_throw([] { not_empty<std::vector<int>>{std::vector<int>()}; });
  }

  // reference semantics
  {
    int x = 5;
    any<int> a{x};
    assert(a == 5);
  }

  {
    int x = 5;
    any<int&> a{x};
    a--;
    assert(x == 4);

    x = 6;
    assert(a == 6);
  }

  {
    int x = 5;
    any<const int&> a{x};
    x++;
    assert(a == 6);
  }

  {
    any<int> a{5};
    int& x = a;
    x++;
    assert(a == 6);
  }

  {
    int x = 5;
    any<int*> a{&x};
    int* y = a;
    (*y)++;
    assert(x == 6);
  }

  // 0-copy
  {
    object o;
    any<object&> a{o};
    assert(a.value.ncopies == 0);
  }

  {
    object o;
    any<const object&> a{o};
    assert(a.value.ncopies == 0);
  }

  {
    any<object> a{{}};
    assert(a.value.ncopies == 0);
  }

  {
    any<const object&> a{{}};
    assert(a.value.ncopies == 0);
  }

  // Lifetime extension
  {
    bool destroyed = false;
    any<object&&> a{{&destroyed}};  // DANGLING REFERENCE, for tests only!!!
    assert(destroyed);
  }

  {
    bool destroyed = false;
    auto fun = [&](any<object&&> a) { assert(!destroyed); };

    fun(object{&destroyed});
    assert(destroyed);

    destroyed = false;
    {
      object o{&destroyed};
      fun(std::move(o));
    }
    assert(destroyed);
  }

  // Member access operator
  {
    any<std::vector<int>> a{std::vector<int>(5)};
    assert(a->size() == 5);
  }

  // In-place construction
  {
    any<std::vector<int>> a{5};
    assert(a->size() == 5);
    assert(a->at(0) == 0);
  }
  {
    any<std::vector<int>> a{5, 1};
    assert(a->size() == 5);
    assert(a->at(0) == 1);
  }

  // Nested function calls with conditions
  {
    auto inner = [](any<object> a) {};
    auto outer = [&](any<object> a) { inner(a); };
    outer(object{});
  }
  {
    auto inner = [](any<object&> a) {
      assert(a.value.ncopies == 0);
      a.value.ncopies = 5;
    };
    auto outer = [&](any<object&> a) { inner(a); };
    object o;
    outer(o);
    assert(o.ncopies == 5);
  }
  {
    auto inner = [](any<object&&> a) { assert(a.value.ncopies == 0); };
    auto outer = [&](any<object> a) { inner(std::move(a)); };
    outer(object{});
  }

  return 0;
}