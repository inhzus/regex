//
// Copyright [2020] <inhzus>
//

#include <unistd.h>

#include <iostream>
#include <string>

#include "regex/graph.h"

template <typename T>
class FixedQueue {
 public:
  FixedQueue(size_t n) : idx_(0), size_(0) { data_.resize(n); }
  void Push(T &&val) {
    if (data_.empty()) return;
    ++size_;
    if (size_ > data_.size()) {
      Pop();
    }
    data_[(idx_ + size_) % data_.size()] = std::move(val);
  }
  T Pop() {
    if (size_ < 1) throw;
    T ret(std::move(data_[idx_]));
    idx_ = (idx_ + 1) % data_.size();
    --size_;
    return ret;
  }
  [[nodiscard]] bool Empty() const { return idx_ == size_; }
  [[nodiscard]] size_t Size() const { return size_; }

 private:
  std::vector<T> data_;
  size_t idx_;
  size_t size_;
};

void help_msg() {
  printf(
      "Usage: ... | rep [OPTIONS] PATTERNS\n"
      "Search PATTERNS from STDIN.\n"
      "Example: cat ~/.vimrc | rep \"^set\"\n");
}

void error_if(bool condition, const char *msg) {
  if (condition) {
    fprintf(stderr, "rep error: %s\n", msg);
    help_msg();
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  int opt;
  int kAfter = 0;
  int kBefore = 0;
  std::string exp;
  while ((opt = getopt(argc, argv, "-A:B:h")) != -1) {
    switch (opt) {
      case 'A': {
        kAfter = atoi(optarg);
        error_if(kAfter < 0, "-A arg should be positive");
        break;
      }
      case 'B': {
        kBefore = atoi(optarg);
        error_if(kBefore < 0, "-B arg should be positive");
        break;
      }
      case 'h': {
        help_msg();
        return 0;
      }
      case '?': {
        return 1;
      }
      default: {
        exp = std::string(optarg);
        break;
      }
    }
  }
  error_if(exp.empty(), "PATTERNS arg missing");
  int after = 0;
  FixedQueue<std::string> queue(kBefore);
  auto pattern = regex::Graph::Compile(exp);
  std::string line;
  while (getline(std::cin, line)) {
    std::string s = pattern.Sub("\033[31m\\0\033[0m", line);
    if (s != line) {
      while (!queue.Empty()) {
        std::string t = queue.Pop();
        printf("%s\n", t.c_str());
      }
      printf("%s\n", s.c_str());
      after = kAfter;
    } else if (after > 0) {
      printf("%s\n", s.c_str());
      --after;
    } else {
      queue.Push(std::move(line));
    }
  }
  return 0;
}
