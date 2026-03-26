#ifndef MY_PROMISE_H
#define MY_PROMISE_H
#include<memory>
#include<thread>
#include<mutex>
#include<exception>
#include<stdexcept>
#include<optional>
#include<variant>
#include<atomic>
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using std::move;
using std::mutex;
using std::condition_variable;
using std::lock_guard;
using std::unique_lock;
using std::exception_ptr;
using std::rethrow_exception;
using std::runtime_error;
using std::optional;
using std::variant;

template<class... Ts>
struct overload : Ts... { using Ts::operator()...; }; 

namespace mpcs {
template<class T> class MyPromise;

template<class T>
struct SharedState {
  std::mutex mtx;
  std::condition_variable cv;
  optional<variant<T, exception_ptr>> value;
};

template<typename T>
class MyFuture {
public:
  MyFuture(MyFuture const &) = delete; // Injected class name
  MyFuture(MyFuture &&) = default;

  T get() {
    unique_lock lck{sharedState->mtx};
    sharedState->cv.wait(lck, 
		[&] {return sharedState->value.has_value(); });
    return std::visit(overload{
        [](T t) { return t; },
        [](exception_ptr ep) -> T { rethrow_exception(ep);} // Have to match return types even though exception is thrown
      }, *sharedState->value);
  }
private:
  friend class MyPromise<T>;
  MyFuture(shared_ptr<SharedState<T>> &sharedState) 
	  : sharedState(sharedState) {}
  shared_ptr<SharedState<T>> sharedState;
};

template<typename T>
class MyPromise
{
public:
  MyPromise() : sharedState{make_shared<SharedState<T>>()} {}

  void set_value(const T &value) {
	{ 
	  lock_guard lck(sharedState->mtx);
      sharedState->value = value;
	}
    sharedState->cv.notify_one();
  }

  void set_exception(exception_ptr exc) {
	{ 
	  lock_guard lck(sharedState->mtx);
      sharedState->value = exc;
	}
    sharedState->cv.notify_one();
  }

  MyFuture<T> get_future() {
    return sharedState;
  }
private:
  shared_ptr<SharedState<T>> sharedState; 
};
}
#endif
