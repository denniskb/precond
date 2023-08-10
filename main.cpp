#include <cassert>
#include <functional>
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
    object o{nullptr};
    const object co{nullptr};

    any<object>{o};
    any<object>{co};
    any<object>{object(nullptr)};

    any<const object>{co};
    any<const object>{o};
    any<const object>{object(nullptr)};

    any<const object&>{o};
    any<const object&>{co};
    any<const object&>{object(nullptr)};

    any<object&>{o};
    // any<object&>{co};
    // any<object&>{object(nullptr)};

    any<object&&>{object(nullptr)};
    // any<object&&>{o};
    // any<object&&>{co};
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
  // object&            |      ✓      |         ✗         |          ✗         |       ✓      |       ✓
  // object&&           |      ✓      |         ✗         |          ✗         |       ✗      |       ✓
  //
  // clang-format on
  {
    any<object> a{nullptr};
    any<const object> ca{nullptr};
    object o{nullptr};
    any<const object&> car{o};
    any<object&> ar{o};
    any<object&&> arr{object{nullptr}};

    // clang-format off
    { object o = a; }
    { object o = ca; }
    { object o = car; }
    { object o = ar; }
    { object o = arr; }

	{ const object o = a; }
    { const object o = ca; }
    { const object o = car; }
    { const object o = ar; }
    { const object o = arr; }

	{ const object& o = a; }
    { const object& o = ca; }
    { const object& o = car; }
    { const object& o = ar; }
    { const object& o = arr; }

	{ object& o = a; }
    //{ object& o = ca; }
    //{ object& o = car; }
    { object& o = ar; }
    { object& o = arr; }

	{ object&& o = std::move(a); }
    //{ object&& o = std::move(ca); }
    //{ object&& o = std::move(car); }
    //{ object&& o = std::move(ar); }
    { object&& o = std::move(arr); }
    // clang-format on
  }

  // checking functionality
  {
    using namespace pre::throw_on_fail;
    assert_nothrow([] { positive<int>{5}; });
    assert_throw([] { positive<int>{-5}; });

    assert_nothrow([] { not_zero<int>{5}; });
    assert_throw([] { not_zero<int>{0}; });

    assert_throw([] { not_null<void*>{nullptr}; });
    assert_nothrow([] { not_null<int*>{reinterpret_cast<int*>(1)}; });

    assert_nothrow([] { not_empty<std::vector<int>>{std::vector<int>(5)}; });
    assert_throw([] { not_empty<std::vector<int>>{std::vector<int>()}; });

    assert_nothrow([] { sorted<std::vector<int>>{{1, 2}}; });
    assert_throw([] { sorted<std::vector<int>>{{2, 1}}; });
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

  // Treat condition like it was a parameter
  {
    int x = 5;
    any<int&> a = x;
    a = 10;
    assert(x == 10);
  }
  {
    std::vector<int> v(2, 1);
    any<std::vector<int>&> a{v};
    a[1] = 2;
    assert(v[0] == 1);
    assert(v[1] == 2);
  }
  {
    auto void_func = [] {};
    any<void (*)()> a{void_func};
    a();
  }
  {
    auto ret_func = [] { return 5; };
    any<int (*)()> a{ret_func};
    assert(a() == 5);
  }
  {
    int x = 5;
    auto capture = [&] { x++; };
    any<std::function<void()>> a{capture};
    a();
    assert(x == 6);
  }
  {
    auto pfunc = [] {};
    pre::assert_on_fail::not_null<void (*)()>{pfunc}();
  }
  {
    int x = 5;
    any<int*> a{&x};
    *a = 10;
    assert(x == 10);
  }
  {
    int x = 5;
    auto fun = [&](int newval) { x = newval; };
    any<decltype(fun)>{fun}(10);
    assert(x == 10);
  }

  return 0;
}