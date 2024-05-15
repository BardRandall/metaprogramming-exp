#pragma once
#include <cassert>
#include <compare>
#include <concepts>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>


namespace detail {
  class ILoggerStorage {
   public:
    virtual ILoggerStorage* MakeCopy() = 0;
    virtual void Log(unsigned int) = 0;
    virtual ~ILoggerStorage() = default;
  };

  template <typename Logger>
  class LoggerStorage : public ILoggerStorage {
   public:
    template <typename F>
    LoggerStorage(F&& f) : logger_(std::forward<F>(f)) {
      
    }

    ILoggerStorage* MakeCopy() override {
      if constexpr(std::is_copy_constructible_v<Logger>) {
        return new LoggerStorage(logger_);
      }
      return nullptr;
    }

    void Log(unsigned int count) override {
      logger_(count);
    }

    ~LoggerStorage() override = default;

   private:
    Logger logger_;
  };

  template <typename F>
  LoggerStorage(F&&) -> LoggerStorage<std::remove_reference_t<F>>;
}


template <class T>
class Spy {
  class SpyProxy {
   public:
    SpyProxy(Spy& spy) : spy_(spy) {
      ++spy_.current_usage_;
      ++spy_.proxy_count_;
    }

    T* operator->() {
      return &spy_.value_;
    }

    ~SpyProxy() {
      if (--spy_.proxy_count_ == 0) {
        if (spy_.logger_) {
          spy_.logger_->Log(spy_.current_usage_);
        }
        spy_.current_usage_ = 0;
      }
    }

   private:
    Spy& spy_;
  };

public:
  Spy() = default;

  explicit Spy(T value) : value_(std::move(value)) {

  }

  Spy(const Spy& other) requires(std::copy_constructible<T>)
    : value_(other.value_) {
    if (other.logger_) {
      logger_ = other.logger_->MakeCopy();
    }
  }

  Spy& operator=(const Spy& other) requires(std::copyable<T>) {
    Spy temp = other;
    *this = std::move(temp);
    return *this;
  }

  Spy(Spy&& other) requires(std::move_constructible<T>)
    : value_(std::move(other.value_)) {
    std::swap(logger_, other.logger_);
  }

  Spy& operator=(Spy&& other) requires(std::movable<T>) {
    Spy temp = std::move(other);
    std::swap(value_, temp.value_);
    std::swap(logger_, temp.logger_);
    return *this;
  }

  T& operator*() {
    return value_;
  }

  const T& operator*() const {
    return value_;
  }

  SpyProxy operator->() {
    return SpyProxy(*this);
  }

  auto operator<=>(const Spy& other) const
    requires std::three_way_comparable<T> {
    return value_ <=> other.value_;
  }

  bool operator==(const Spy& other) const
    requires std::equality_comparable<T> {
    return value_ == other.value_;
  }

  // Resets logger
  void setLogger() {
    if (logger_) {
      delete logger_;
    }
    logger_ = nullptr;
  }

  template <std::invocable<unsigned int> Logger> requires (
    (!std::copy_constructible<T> || std::copy_constructible<std::remove_reference_t<Logger>>) &&
    (!std::copyable<T> || std::copyable<std::remove_reference_t<Logger>>) &&
    std::is_nothrow_destructible_v<Logger>
  )
  void setLogger(Logger&& logger) {
    if (logger_) {
      delete logger_;
    }
    logger_ = new detail::LoggerStorage(std::forward<Logger>(logger));
  }

  ~Spy() {
    if (logger_) {
      delete logger_;
    }
  }

private:

  T value_;
  detail::ILoggerStorage* logger_ = nullptr;
  unsigned int current_usage_ = 0;
  std::size_t proxy_count_ = 0;
};