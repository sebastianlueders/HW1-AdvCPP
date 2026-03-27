// Requires C++20 (atomic_flag::wait / notify_one added in C++20).

#include <memory>
#include <thread>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <optional>
#include <variant>
#include <atomic>
#include <iostream>

using std::shared_ptr;
using std::make_shared;
using std::move;
using std::exception_ptr;
using std::rethrow_exception;
using std::runtime_error;
using std::optional;
using std::variant;

// Helper for std::visit; allows us to pull all inherited operator() overloads into the same scope
// Allows us to write the following std::visit:
/*
std::visit(overload{
    [](T t) { ... }, // If the variant currently holds T
    [](exception_ptr ep) { ... }  // If the variant is an exception_ptr
}, someVariant);
*/
template<class... Ts>
struct overload : Ts... { using Ts::operator()...; };

// To avoid naming collisions
namespace mpcs 
{

// Tell the compiler there will be a class named MyPromise<T> later on
template<class T> class MyPromise;

// ---------- SharedState --------------------------------------------------
// atomic_flag starts in the "cleared" (false) state. Changes to true once the output value is saved in value.
// Invariant: flag is set (true) iff value has been written.
template<class T>
struct SharedState {
    std::atomic_flag ready;                       // future waits on flag; promise sets it when the result is ready
    optional<variant<T, exception_ptr>> value;    // Holds the return value for the operation
};

// MyFuture template class for types T
template<typename T>
class MyFuture {
public:
    MyFuture(MyFuture const &) = delete; // cannot copy a MyFuture: MyFuture<int> f2 = f1;   // not allowed
    MyFuture(MyFuture &&)      = default; // Transferring ownership of a future is allowed 

    T get() {
        // Block until the flag is set (true).
        // memory_order_acquire pairs with the release in set_value/set_exception,
        // guaranteeing the write to `value` is visible here.
        sharedState->ready.wait(false, std::memory_order_acquire); // wait while ready is false

        return std::visit(overload{
            [](T t)              { return t; },
            [](exception_ptr ep) -> T { rethrow_exception(ep); }
        }, *sharedState->value);
    }

private:
    friend class MyPromise<T>; // Gives MyFuture access to MyPromise private constructor
    MyFuture(shared_ptr<SharedState<T>> &ss) : sharedState(ss) {} // Takes a shared pointer to shared state and stores it in future
    shared_ptr<SharedState<T>> sharedState; // Stores a shared pointer to SharedState<T>
};

// MyPromise template class for types T
template<typename T>
class MyPromise {
public:
    MyPromise() : sharedState{make_shared<SharedState<T>>()} {} // Constructor for MyPromise that creates dynamic SharedState

    void set_value(const T &value) {
        sharedState->value = value;
        // Release store: all writes above are visible to the acquire load in get().
        sharedState->ready.test_and_set(std::memory_order_release); // atomically set ready to true
        sharedState->ready.notify_one(); // Wakes one thread waiting on get(), if any
    }

    void set_exception(exception_ptr exc) {
        sharedState->value = exc;
        sharedState->ready.test_and_set(std::memory_order_release);
        sharedState->ready.notify_one();
    }

    MyFuture<T> get_future() {
        return sharedState;
    }

private:
    shared_ptr<SharedState<T>> sharedState;
};

} // namespace mpcs

// Test
int main() {
    // Normal value case
    {
        mpcs::MyPromise<int> p;
        auto f = p.get_future();

        std::thread t([&p] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            p.set_value(42);
        });

        std::cout << "got: " << f.get() << "\n"; // prints 42
        t.join();
    }

    // Exception case
    {
        mpcs::MyPromise<int> p;
        auto f = p.get_future();

        std::thread t([&p] {
            p.set_exception(std::make_exception_ptr(runtime_error("oops")));
        });

        try {
            f.get();
        } catch (const runtime_error &e) {
            std::cout << "caught: " << e.what() << "\n"; // prints "oops"
        }
        t.join();
    }
}



