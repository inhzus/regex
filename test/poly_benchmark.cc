//
// Copyright [2020] <inhzus>
//

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>
#include <utility>
#include <variant>  // NOLINT

constexpr int kIterateCount = 10000;

namespace a {
// Implementation 1
struct Edge {
  enum Type { Char, Store, NamedStore, Epsilon };

  Edge() : type(Epsilon) {} // NOLINT
  explicit Edge(char ch) : type(Char), ch({ch}) {}  // NOLINT
  explicit Edge(int idx) : type(Store), store{idx} {}  // NOLINT
  Edge(int idx, std::string name) : // NOLINT
      type(NamedStore), named_store{idx, std::move(name)} {}

  Type type;
  union {
    struct {
      char val;
    } ch;
    struct {
      int idx;
    } store;
    struct {
      int idx;
      std::string name;
    } named_store;
  };
};  // Implementation 1
}  // namespace a

namespace b {
// Implementation 2
struct Edge {
  enum Type { Char, Store, NamedStore, Epsilon };

  [[nodiscard]] virtual Type type() const = 0;
};
struct EpsilonEdge : Edge {
  [[nodiscard]] Type type() const override { return Epsilon; }
};
struct CharEdge : Edge {
  [[nodiscard]] Type type() const override { return Char; }
  explicit CharEdge(char ch) : ch(ch) {}
  char ch;
};
struct NamedStoreEdge : Edge {
  [[nodiscard]] Type type() const override { return NamedStore; }

  NamedStoreEdge(int idx, std::string name) : idx(idx), name(std::move(name)) {}

  int idx;
  std::string name;
};
struct StoreEdge : Edge {
  [[nodiscard]] Type type() const override { return Store; }

  explicit StoreEdge(int idx) : idx(idx) {}

  int idx;
};  // implementation 2

}  // namespace b

namespace c {

// Implementation 3
struct Edge {
  template<typename>
  struct tag {};
  template<typename T, typename V>
  struct index_of_variant;
  template<typename T, typename... Ts>
  struct index_of_variant<T, std::variant<Ts...>>
      : std::integral_constant<
          size_t, std::variant<tag<Ts>...>(tag<T>()).index()> {
  };
  struct Epsilon {};
  struct Store { int idx; };
  struct NamedStore {
    int idx;
    std::string name;
  };

  Edge() : data(Epsilon{}) {}
  explicit Edge(char ch) : data(ch) {}
  explicit Edge(int idx) : data(Store{idx}) {}
  Edge(int idx, std::string name) :
      data(NamedStore{idx, std::move(name)}) {}

  template<typename T>
  static constexpr size_t Index() {
    return index_of_variant<
        T, std::variant<char, Epsilon, Store, NamedStore>>();
  }

  std::variant<char, Epsilon, Store, NamedStore> data;
};  // Implementation 3

}  // namespace c

TEST_CASE("union struct benchmark") {
  REQUIRE(48 == sizeof(a::Edge));
  std::vector<a::Edge *> edges{
      new a::Edge(),
      new a::Edge('a'),
      new a::Edge(1),
      new a::Edge(2, "name")
  };
  // @formatter:off
  BENCHMARK("union struct") {
    int sum = 0;
    for (int i = 0; i < kIterateCount; ++i) {
      for (auto *edge : edges) {
        switch (edge->type) {
          case a::Edge::Char: {
            sum += edge->ch.val;
            break;
          }
          case a::Edge::Epsilon: {
            sum += 1;
            break;
          }
          case a::Edge::Store: {
            sum += edge->store.idx;
            break;
          }
          case a::Edge::NamedStore: {
            sum += edge->named_store.idx;
            sum += edge->named_store.name.size();
            break;
          }
        }
      }
    }
    return sum;
  };
  // @formatter: on
}

TEST_CASE("polymorphism struct") {
  REQUIRE(8 == sizeof(b::Edge));
  using b::Edge, b::StoreEdge, b::CharEdge, b::EpsilonEdge, b::NamedStoreEdge;
  std::vector<Edge *> edges{
      new EpsilonEdge(),
      new CharEdge('a'),
      new StoreEdge(1),
      new NamedStoreEdge(2, "name")
  };
  // @formatter:off
  BENCHMARK("polymorphism struct") {
    int sum = 0;
    for (int i = 0; i < kIterateCount; ++i) {
      for (auto *edge : edges) {
        switch (edge->type()) {
          case Edge::Epsilon: {
           [[maybe_unused]] auto e = dynamic_cast<EpsilonEdge *>(edge);
           sum += 1;
           break;
          }
          case Edge::Char: {
           auto e = dynamic_cast<CharEdge *>(edge);
           sum += e->ch;
           break;
          }
          case Edge::Store: {
           auto e = dynamic_cast<StoreEdge *>(edge);
           sum += e->idx;
           break;
          }
          case Edge::NamedStore: {
           auto e = dynamic_cast<NamedStoreEdge *>(edge);
           sum += e->idx;
           sum += e->name.size();
           break;
          }
        }
      }
    }
    return sum;
  };
  // @formatter:on
}

TEST_CASE("variant struct") {
  using c::Edge;
  REQUIRE(48 == sizeof(Edge));
  std::vector<Edge *> edges{
      new Edge(),
      new Edge('a'),
      new Edge(1),
      new Edge(2, "name")
  };
  // @formatter:off
  BENCHMARK("variant struct") {
    int sum = 0;
    for (int i = 0; i < kIterateCount; ++i) {
      for (auto *edge : edges) {
        switch (edge->data.index()) {
          case Edge::Index<char>(): {
            sum += std::get<char>(edge->data);
            break;
          }
          case Edge::Index<Edge::Epsilon>(): {
            sum += 1;
            break;
          }
          case Edge::Index<Edge::Store>(): {
            sum += std::get<Edge::Store>(edge->data).idx;
            break;
          }
          case Edge::Index<Edge::NamedStore>(): {
            auto &store = std::get<Edge::NamedStore>(edge->data);
            sum += store.idx;
            sum += store.name.size();
            break;
          }
        }
      }
    }
    return sum;
  };
  // @formatter:on
}
