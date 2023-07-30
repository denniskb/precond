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
using any = pre::cond<[](auto&&) {}, T>;

template <class T>
using positive = pre::cond<[](auto x) { myassert(x > 0); }, T>;

template <class T>
using not_zero = pre::cond<[](auto x) { myassert(x != 0); }, T>;

template <class T>
using not_null = pre::cond<[](auto* p) { myassert(p); }, T>;

template <class T>
using not_empty = pre::cond<[](auto&& cont) { myassert(!cont.empty()); }, T>;

template <class... Ts>
using same_size =
    pre::cond<[](auto&&... conts) { myassert(conts.size() == ...); }, Ts...>;

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
    std::is_object_v<T> && std::is_move_constructible_v<T> &&
    std::is_assignable_v<T&, T> && std::is_swappable_v<T>;

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
		false, false, true , false, 
		false, false, false, true};
    // clang-format on

    using rows_t = std::tuple<any<object>, any<const object>,
                              any<const object&>, any<object&>, any<object&&>>;

    using cols_t = std::tuple<const object, const object&, object&, object&&>;

    constexpr_for<5>([](auto i_row) {
      constexpr_for<4>([&](auto i_col) {
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

    constexpr_for<5>([](auto i_row) {
      constexpr_for<5>([&](auto i_col) {
        constexpr int index = i_row * 5 + i_col;
        constexpr bool constructible =
            std::is_constructible_v<std::tuple_element_t<i_row, rows_t>,
                                    std::tuple_element_t<i_col, cols_t>>;
        static_assert(constructible == table[index]);
      });
    });
  }

  {  // What wrapper types can be moved from?
    static_assert(is_movable<any<object>>);
    static_assert(!is_movable<any<const object>>);
    static_assert(!is_movable<any<const object&>>);
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

    assert_nothrow([] {
      same_size<std::vector<int>, std::vector<object>>{std::vector<int>(5),
                                                       std::vector<object>(5)};
    });
    assert_throw([] {
      same_size<std::vector<int>, std::vector<object>>{std::vector<int>(3),
                                                       std::vector<object>(5)};
    });
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
    a.get<0>()--;
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

  // 0-copy
  {
    object o;
    any<object&> a{o};
    assert(a.get<0>().ncopies == 0);
  }

  {
    object o;
    any<const object&> a{o};
    assert(a.get<0>().ncopies == 0);
  }

  {
    any<object> a{{}};
    assert(a.get<0>().ncopies == 0);
  }

  {
    any<const object&> a{{}};
    assert(a.get<0>().ncopies == 0);
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

    fun({&destroyed});
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

  // Structured binding
  {
    std::vector<int> x(5);
    std::vector<int> y(5);
    same_size<std::vector<int>&, std::vector<int>&> cond{x, y};

    auto& [a, b] = *cond;
    assert(a.size() == b.size() && a.size() == 5);
    a.clear();
    assert(x.size() == 0);
  }

  // Nested conditions
  {
    // TODO
  }

  return 0;
}