// Upstream available here: https://github.com/Zubax/ramen

/// ______  ___ ___  ________ _   _
/// | ___ `/ _ `|  `/  |  ___| ` | |
/// | |_/ / /_` ` .  . | |__ |  `| |
/// |    /|  _  | |`/| |  __|| . ` |
/// | |` `| | | | |  | | |___| |`  |
/// `_| `_`_| |_|_|  |_|____/`_| `_/
///
/// RAMEN (Real-time Actor-based Message Exchange Network) is a very compact single-header C++20+ dependency-free
/// library that implements message-passing/flow-based programming for hard real-time mission-critical embedded systems.
/// It is designed to be very low-overhead, efficient, and easy to use. Please refer to the README for the docs.
///
/// The library includes the core functionality (the message passing ports) along with a few utilities for
/// practical message-passing programs.
///
/// The notation to define control flow, data flow, and relation between components is described here.
///
/// In diagrams, arrows represent the control flow direction: the pointed-to item is invoked by the pointing item.
/// Actors accept data inputs from the left and output data to the right.
/// A control input (the invoked item) is called a behavior, and a control output is called an event.
/// Overall this results in four possible combinations of input/output ports,
/// which can be used to arrange two dataflow models -- push (eager) and pull (lazy):
///
///     Port kind       Control     Data    Alias
///     --------------------------------------------
///      in-behavior    in          in      Pushable
///     out-event       out         out     Pusher
///     out-behavior    in          out     Pullable
///      in-event       out         in      Puller
///
/// Pull-model ports pair naturally with other pull-model ports, and the same for push-model ports.
/// To bridge push and pull models together, we use two basic components:
/// Latch (with behaviors on either side) and Lift (with events on either side and an additional trigger behavior);
/// they are defined below.
///
/// The following diagramming notation is adopted:
///
///                                +--------+
///   (input behavior) pushable -->|        |--> pusher (output event)
///                                | Actor  |
///      (input event) puller   <--|        |<-- pullable (output behavior)
///                                +--------+
///
/// The type of exchanged data can be arbitrary and it is possible to exchange more than one object per port.
/// Behaviors and Events can be linked together arbitrarily using operator>>; remember that the arrows point in the
/// direction of the control flow, not data flow. Within an actor, a behavior may trigger some events;
/// an event of an actor can trigger behaviors in other actors that it is linked to.
/// In some scenarios, the direction of the control flow does not matter on a bigger scale,
/// in which case one can use operator^ to link behaviors and events together irrespective of the control direction.
///
/// Events can be linked with behaviors and other events into topics. Given multiple events on a topic,
/// the resulting behavior is that any of the linked events will cause all linked behaviors to be triggered in
/// the order of linking. Topics that do not contain behaviors have no effect (events do not affect each other).
/// Behaviors cannot be linked directly without events.
///
/// Actors are usually implemented as structs with all data fields public. Public data does not hinder
/// encapsulation because actors are unable to affect or even see each other's data directly, as all interation is
/// done through message passing; this is the essence of the actor model. Hence, the encapsulation mechanisms provided
/// by C++ become redundant. This point only holds for pure actors, however; there may exist mixed classes that work
/// both as actors and as regular objects, in which case this recommendation may not apply
/// (whether it's a good practice to define such mixed classes is another matter).
///
/// Remember that recursive dependencies are common in actor networks, especially when they are used to implement
/// control systems. When implementing an actor, keep in mind that triggering any event to push or pull data can
/// cause the control flow to loop back to the current actor through a possibly very long chain of interactions.
/// Proper design should prevent the possibility of descending into an infinite recursion and also serving data
/// from an actor whose internal state is inconsistent. To avoid this class of errors, state updates should
/// always be performed in a transactional manner: first, all inputs are read, then the state is updated, and only
/// then the outputs are written. This is a general rule of thumb for designing such systems and is not specific
/// to this library. Improper design can cause an infinite recursion with a subsequent stack overflow and a segfault.
///
/// Pushable and Pusher accept const references to the passed data. It is not possible to pass non-const
/// references nor rvalue references because the data may be passed through a chain of actors, and it is necessary
/// to guarantee that each will receive the exact same data regardless of the order in which the actors are invoked.
///
/// Puller and Pullable accept mutable references to the data because the data is returned via out-parameters.
/// This is necessary to support the use case when the output type can only be assigned inside a port behavior,
/// but not constructed.
/// One practical case where this situation arises is returning Eigen::MatrixRef<> with at least one dynamic dimension.
///
/// Defining ports of highly specialized types is possible but rarely useful because specialized types impair
/// composability. For example, suppose there is a specialized configuration struct for some actor. The actor could
/// accept the configuration via an in-port and it would work, but the utility of this choice is limited because
/// in order to make use of this port, the other actor would need to have access to its specific type, at which point
/// the message passing aspect becomes redundant, as it would be easier to just pass/alter the configuration struct
/// directly (e.g., by mutating the state of the first actor). Instead, if the first actor were to accept configuration
/// via more granular in-ports of more generic types, like vectors, matrices, or whatever is common in the application,
/// then the composability of the solution would not be compromised.
///
/// Message passing enables a new approach to policy-based design. It is possible to ship predefined policies with
/// an actor by defining several behaviors, each implementing its own policy while sharing the same type.
/// The client will then choose which particular behavior (policy) to use at the time when the network is linked.
///
/// ===================================================================================================================
///
/// Author: Pavel Kirienko <pavel.kirienko@zubax.com>
///
/// MIT License
///
/// Copyright (c) Zubax Robotics
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
/// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
/// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
/// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or substantial portions
/// of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
/// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
/// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
/// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// NOTES
///
/// This derived version has been adapted by LLMs for use with C++17 in AVR8 microcontrollers with avr-libstdcpp.
/// Models used: Claude 4.0 Sonnet and Gemini 2.5 Pro Preview.
///
/// The following notes are LLM-generated:
///
/// Under avr-libstdcpp (avr-gcc 7+ with -std=c++17 or newer), you can typically use:
///   Standard headers: <array>, <algorithm>, <tuple>, <type_traits>, <cstddef>, <cstdint>, <utility>, <limits>
///   Possibly <cmath> (may require linking math.cc or similar for full support).
///   Common type_traits: std::is_invocable_v, std::enable_if_t, std::decay_t, std::is_same_v,
///                       std::is_default_constructible_v, std::is_move_constructible_v,
///                       std::is_nothrow_move_constructible_v, std::is_nothrow_move_assignable_v,
///                       std::conditional_t, std::conjunction_v, std::index_sequence, std::make_index_sequence.
///   Avoid heavy headers like <vector>, <map>, <string>, <iostream>, <sstream>, <functional> (for std::function)
///   if you need minimal code size and RAM usage. RAMEN's Function class is an alternative to std::function.
///
/// Summary of differences with upstream version:
///
/// The AVR-compatible port of ramen.hpp introduces several changes to enhance compatibility
/// with more constrained environments like AVR microcontrollers, typically targeting C++14/C++17
/// standards and prioritizing code size and resource usage.
///
/// Key differences include:
///
/// 1.  **C++ Standard and Feature Usage:**
///     *   The original `ramen.hpp` is designed for C++20+ and utilizes features like
///         C++20 concepts (`requires` clauses) and attributes like `[[no_unique_address]]`.
///     *   The AVR port is adapted for older C++ standards (likely C++14 or C++17).
///         -   `requires` clauses are generally replaced with `std::enable_if_t` for SFINAE.
///         -   Attributes like `[[no_unique_address]]` are removed as they might not be
///             supported or have the intended effect with older compilers or for embedded targets.
///
/// 2.  **`Function` Class:**
///     *   **Constructors & Assignment:** The AVR port uses `std::enable_if_t` for SFINAE in
///         constructors and assignment operators instead of C++20 concepts.
///     *   **Default Constructor:** The AVR port's `Function` class has a default constructor
///         (`Function() noexcept = default;`), making it default-constructible (but potentially in an
///         uninitialized/invalid state until assigned). The original `Function` was not
///         default-constructible to ensure it was always valid upon construction.
///     *   **State Management in Move Operations:** The AVR port explicitly nullifies `call_`,
///         `dtor_`, and `move_` pointers in the moved-from `Function` object to prevent
///         double-frees or calls on invalid objects.
///     *   **`is_valid_target_v`:** The AVR port consistently uses `is_valid_target_v` (a `static constexpr bool`)
///         for SFINAE checks, which is common in pre-C++20 SFINAE.
///     *   **`explicit operator bool()`:** Added to check if the `Function` is initialized.
///     *   **Error Handling:** The `operator()` in the AVR port includes an `assert(call_ != nullptr)`
///         to catch calls on uninitialized or moved-from functions.
///
/// 3.  **`ListNode` Class:**
///     *   **Constructor SFINAE:** The AVR port introduces a more complex SFINAE mechanism
///         (`detail::is_valid_listnode_forwarding_constructor_args`) for the variadic
///         constructor of `ListNode` to correctly handle forwarding arguments to the
///         underlying `T`'s constructor while avoiding ambiguity with copy/move constructors.
///         This was a specific fix noted in the port's version update (`RAMEN_VERSION_MINOR 4 // Updated for ListNode SFINAE fix`).
///     *   **Move Semantics:** The move constructor and move assignment operator in the AVR port
///         more explicitly handle the transfer of `prev_` and `next_` pointers and ensure the
///         moved-from node is properly detached. The `noexcept` specification is also conditional
///         based on the `T`'s move constructibility/assignability.
///     *   **`clusterize` Method:** The `key_func` in the AVR port now takes `const T&` instead of
///         `const ListNode&`. The logic for finding the head of the list before clusterization
///         is refined (`this->head()`).
///     *   **Const Correctness:** Added `const` overloads for `next()`, `head()`, and `tail()`.
///
/// 4.  **`Behavior` Class:**
///     *   **Constructor SFINAE:** Uses `std::enable_if_t` similar to `Function`.
///     *   **Direct Invocation:** The comment in the AVR port for `operator()` clarifies that
///         direct invocation calls the behavior's function but does *not* automatically
///         broadcast to other linked nodes via an `Event`. The original implied that
///         direct invocation might trigger the entire topic (which it did via a temporary Event).
///         The AVR port's direct invocation simply calls `fun_(args...)`.
///
/// 5.  **`Event` Class:**
///     *   **`operator>>` (Linking):** The AVR port's `clusterize` call within `operator>>`
///         gets the head of the list explicitly: `static_cast<detail::ListNode<...>*>(this)->head()->template clusterize<2>(...)`.
///         It also includes a comment about the potential performance cost of `clusterize` on AVR
///         and suggests conditional removal if complex event chaining isn't used.
///     *   **Invocation `operator()`:** Iterates using `const detail::ListNode<...>*` and
///         `static_cast` to call `trigger` on `Triggerable` objects.
///
/// 6.  **`Footprint` and Default Footprints:**
///     *   The `default_behavior_footprint` is `sizeof(void*) * 2` in the AVR port,
///         potentially smaller than the original's `std::max({sizeof(void(std::tuple<>::*)()), sizeof(void*), sizeof(void (*)())})`
///         which aimed to accommodate member function pointers. This change likely aims to save memory.
///     *   Utility components like `PushUnary`, `PullUnary`, `PullNary` in the AVR port often
///         cap the `Footprint` passed to their internal `Pushable`/`Pullable` members, e.g.,
///         `Footprint<(fp < 64) ? fp : 64>`. This is a hard cap to limit memory usage on AVR.
///
/// 7.  **Utility Components (`Latch`, `Lift`, `PushUnary`, `PullUnary`, `PullNary`):**
///     *   Constructors consistently use `std::enable_if_t` for SFINAE.
///     *   `Lift::trigger`: The AVR port adds checks `if (in)` and `if (out)` before attempting
///         to call the puller or pusher, making it safer if they are not linked.
///     *   `PullUnary` and `PullNary` in the AVR port have slightly different internal structures for
///         `apply_inputs`, `apply_function` (using `std::index_sequence_for`) and helper functions.
///         The `value` member in the AVR port's `PullNary` is not `[[no_unique_address]]`.
///
/// 8.  **`Puller<T>` Specialization:**
///     *   The fancy helpers `operator*` and `operator->` use `std::enable_if_t` to ensure `T`
///         is default-constructible, making the SFINAE explicit.
///
/// 9.  **`Finalizer` Class:**
///     *   The default footprint is `sizeof(void*) * 4`, potentially smaller than the original's `sizeof(void*) * 10`.
///     *   Constructor uses `std::enable_if_t`.
///     *   Move operations (`Finalizer(Finalizer&&)` and `operator=(Finalizer&&)`) more explicitly
///         assign a no-op lambda (`[]{})`) to the moved-from `act_` member to ensure it's disarmed.
///     *   The destructor and move assignment now check `if (act_)` before calling `act_()`,
///         accommodating the default-constructed state or a disarmed state.
///
/// 10. **General Style and Minor Changes:**
///     *   The AVR port often uses `_v` suffixes for type traits (e.g., `std::is_invocable_r_v`)
///         which are C++17 helpers. The original used them too, but the port's reliance is
///         consistent with targeting C++17.
///     *   `alignof(std::max_align_t)` is used as the default alignment for `Function`, same as original.
///     *   `static_assert` for alignment in `Function` checks `(alignment > 0)`.
///
/// In summary, the AVR port focuses on broader compiler compatibility (especially for older C++ standards),
/// memory footprint reduction (smaller default footprints, footprint caps), and robust SFINAE using
/// `std::enable_if_t`. It also makes some minor adjustments to logic and error handling for
/// resource-constrained environments.
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <array>
#include <tuple>
#include <utility>
#include <cassert>
#include <cstddef>
#include <algorithm> // For std::min/max if used, though not directly by RAMEN core
#include <type_traits>

#define RAMEN_VERSION_MAJOR 0
#define RAMEN_VERSION_MINOR 4 // Updated for ListNode SFINAE fix

namespace ramen
{

// Forward declarations
template <typename>
class Callable;

template <typename, std::size_t footprint, std::size_t alignment = alignof(std::max_align_t)>
class Function;

// Definition for Footprint
template <std::size_t N>
struct Footprint { };

template <typename, std::size_t footprint, bool movable = false>
struct Behavior;

template <typename>
struct Event;

constexpr std::size_t default_behavior_footprint = sizeof(void*) * 2;

namespace detail
{
    // Helper for SFINAE on ListNode's variadic constructor
    // Primary template: general case, not enabled.
    template <typename ThisListNodeType, typename... Args>
    struct is_valid_listnode_forwarding_constructor_args : std::false_type {};

    // Specialization for 0 arguments (for T's default constructor)
    template <typename ThisListNodeType>
    struct is_valid_listnode_forwarding_constructor_args<ThisListNodeType> : std::true_type {};

    // Specialization for 1 argument - enable if it's not ThisListNodeType itself
    template <typename ThisListNodeType, typename SingleArg>
    struct is_valid_listnode_forwarding_constructor_args<ThisListNodeType, SingleArg>
        : std::bool_constant<!std::is_same_v<ThisListNodeType, std::decay_t<SingleArg>>> {};

    // Specialization for more than 1 argument (for T's variadic constructor)
    template <typename ThisListNodeType, typename Arg1, typename Arg2, typename... RestArgs>
    struct is_valid_listnode_forwarding_constructor_args<ThisListNodeType, Arg1, Arg2, RestArgs...> : std::true_type {};

} // namespace detail

// ====================================================================================================================

template <typename R, typename... A>
class Callable<R(A...)>
{
public:
    template <typename F>
    static constexpr bool is_compatible_v = std::is_invocable_r_v<R, F, A...>;

    virtual R operator()(A... args) const = 0;

    Callable() noexcept                      = default;
    Callable(const Callable&)                = default;
    Callable(Callable&&) noexcept            = default;
    Callable& operator=(const Callable&)     = default;
    Callable& operator=(Callable&&) noexcept = default;

protected:
    ~Callable() noexcept = default;
};

namespace detail
{
// Helper to perform the core validation checks
template <typename Signature, std::size_t footprint, std::size_t alignment, typename Target>
struct IsValidTargetHelper : std::conjunction< // std::conjunction needs C++17, or use custom for C++14
    std::bool_constant<Callable<Signature>::template is_compatible_v<Target>>,
    std::bool_constant<(sizeof(Target) <= footprint)>,
    std::bool_constant<(alignof(Target) <= alignment)>,
    std::is_move_constructible<Target>
> {};

// Main IsValidTarget struct - by default, it uses the helper
template <typename Signature, std::size_t footprint, std::size_t alignment, typename Target>
struct IsValidTarget : IsValidTargetHelper<Signature, footprint, alignment, Target> {};

// Specialization to explicitly forbid Function-to-Function assignments
template <typename X_Sig, typename Y_Sig, std::size_t x_fp, std::size_t x_al, std::size_t y_fp, std::size_t y_al>
struct IsValidTarget<X_Sig, x_fp, x_al, Function<Y_Sig, y_fp, y_al>> : std::false_type {};

} // namespace detail

template <typename R, typename... A, std::size_t footprint, std::size_t alignment>
class Function<R(A...), footprint, alignment> final : public Callable<R(A...)>
{
    template <typename, std::size_t, std::size_t>
    friend class Function;

    static_assert((alignment > 0) && ((alignment & (alignment - 1)) == 0), "alignment must be a power of 2");

public:
    template <typename F, typename T = std::decay_t<F>>
    static constexpr bool is_valid_target_v = detail::IsValidTarget<R(A...), footprint, alignment, T>::value;

    template <typename F>
    Function(F&& fun, std::enable_if_t<is_valid_target_v<F>, int> = 0) noexcept
    {
        construct(std::forward<F>(fun));
    }
    
    Function() noexcept = default;

    Function(const Function& that) = delete;
    Function(Function&& that) noexcept : call_(that.call_), dtor_(that.dtor_), move_(that.move_)
    {
        if (move_ != nullptr)
        {
            move_(fun_.data(), that.fun_.data());
            that.call_ = nullptr;
            that.dtor_ = nullptr;
            that.move_ = nullptr;
        }
    }

    template <std::size_t foot2, std::size_t align2>
    Function(Function<R(A...), foot2, align2>&& that,
             std::enable_if_t<(foot2 <= footprint) && (align2 <= alignment), int> = 0) noexcept :
        call_(that.call_),
        dtor_(that.dtor_),
        move_(that.move_)
    {
        if (move_ != nullptr)
        {
            move_(fun_.data(), that.fun_.data());
            that.call_ = nullptr;
            that.dtor_ = nullptr;
            that.move_ = nullptr;
        }
    }

    Function& operator=(const Function& that) = delete;
    Function& operator=(Function&& that) noexcept
    {
        if (this != &that)
        {
            assign(std::move(that));
        }
        return *this;
    }

    template <std::size_t foot2, std::size_t align2>
    std::enable_if_t<(foot2 <= footprint) && (align2 <= alignment), Function&>
    operator=(Function<R(A...), foot2, align2>&& that) noexcept
    {
        assign(std::move(that));
        return *this;
    }

    template <typename F>
    std::enable_if_t<is_valid_target_v<F>, Function&>
    operator=(F&& fun) noexcept
    {
        if (dtor_ != nullptr) destroy();
        construct(std::forward<F>(fun));
        return *this;
    }

    explicit operator bool() const noexcept { return call_ != nullptr; }

    R operator()(A... args) const override {
        assert(call_ != nullptr && "Function not initialized or moved from");
        return call_(fun_.data(), args...);
    }

    ~Function() noexcept { if (dtor_ != nullptr) destroy(); }

private:
    template <typename F, typename T_target = std::decay_t<F>>
    std::enable_if_t<detail::IsValidTarget<R(A...), footprint, alignment, T_target>::value>
    construct(F&& fun) noexcept
    {
        call_ = [](void* const ptr, A... args) -> R { return (*static_cast<T_target*>(ptr))(args...); };
        dtor_ = [](void* const ptr) noexcept { static_cast<T_target*>(ptr)->~T_target(); };
        move_ = [](void* const dst, void* const src) noexcept { new (dst) T_target(std::move(*static_cast<T_target*>(src))); };
        static_assert((sizeof(T_target) <= footprint) && (alignof(T_target) <= alignment), "Function target too large");
        new (fun_.data()) T_target(std::forward<F>(fun));
    }

    void destroy() noexcept
    {
        assert(dtor_ != nullptr);
        dtor_(fun_.data());
        dtor_ = nullptr;
        call_ = nullptr;
        move_ = nullptr;
    }

    template <std::size_t foot2, std::size_t align2>
    std::enable_if_t<(foot2 <= footprint) && (align2 <= alignment)>
    assign(Function<R(A...), foot2, align2>&& that) noexcept
    {
        if (dtor_ != nullptr) destroy();
        call_ = that.call_;
        dtor_ = that.dtor_;
        move_ = that.move_;
        if (move_ != nullptr)
        {
            move_(fun_.data(), that.fun_.data());
            that.call_ = nullptr;
            that.dtor_ = nullptr;
            that.move_ = nullptr;
        }
    }

    alignas(alignment) mutable std::array<unsigned char, footprint> fun_;
    R (*call_)(void*, A...)              = nullptr;
    void (*dtor_)(void*) noexcept        = nullptr;
    void (*move_)(void*, void*) noexcept = nullptr;
};

// ====================================================================================================================
namespace detail
{

template <typename T>
class ListNode : public T
{
public:
    template <typename... Args>
    ListNode(Args&&... args,
             std::enable_if_t<
                 detail::is_valid_listnode_forwarding_constructor_args<ListNode<T>, Args...>::value,
                 int
             > = 0)
        : T(std::forward<Args>(args)...) {}

    ListNode(const ListNode&) = delete;
    ListNode(ListNode&& that) noexcept(std::is_nothrow_move_constructible_v<T>)
        : T(std::move(static_cast<T&>(that)))
    {
        prev_ = that.prev_;
        next_ = that.next_;
        if (prev_ != nullptr) { prev_->next_ = this; }
        if (next_ != nullptr) { next_->prev_ = this; }
        that.prev_ = nullptr;
        that.next_ = nullptr;
    }
    ListNode& operator=(const ListNode&) = delete;
    ListNode& operator=(ListNode&& that) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (this != &that)
        {
            static_cast<T&>(*this) = std::move(static_cast<T&>(that));
            remove(); // remove this from its current list before taking over that's links
            prev_ = that.prev_;
            next_ = that.next_;
            if (prev_ != nullptr) { prev_->next_ = this; }
            if (next_ != nullptr) { next_->prev_ = this; }
            that.prev_ = nullptr;
            that.next_ = nullptr;
        }
        return *this;
    }

    constexpr bool      linked() const noexcept { return (prev_ != nullptr) || (next_ != nullptr); }
    constexpr const ListNode* next() const noexcept { return next_; } // Const version
    ListNode* next() noexcept { return next_; } // Non-const version

    ListNode* head() noexcept
    {
        ListNode* p = this;
        while (p->prev_ != nullptr) { p = p->prev_; }
        return p;
    }
    const ListNode* head() const noexcept // Const version
    {
        const ListNode* p = this;
        while (p->prev_ != nullptr) { p = p->prev_; }
        return p;
    }

    ListNode* tail() noexcept
    {
        ListNode* p = this;
        while (p->next_ != nullptr) { p = p->next_; }
        return p;
    }
     const ListNode* tail() const noexcept // Const version
    {
        const ListNode* p = this;
        while (p->next_ != nullptr) { p = p->next_; }
        return p;
    }


    void merge(ListNode* const that) noexcept
    {
        if (that != nullptr && that != this)
        {
            ListNode* const that_head = that->head();
            ListNode* const this_head_ptr = this->head(); // Use different name
            if (this_head_ptr != that_head)
            {
                ListNode* const this_tail_ptr = this->tail(); // Use different name
                this_tail_ptr->next_          = that_head;
                that_head->prev_          = this_tail_ptr;
            }
        }
    }

    template <std::size_t N_Clusters, typename K_Func>
    std::enable_if_t<std::is_invocable_r_v<std::size_t, K_Func, const T&>>
    clusterize(const K_Func& key_func) noexcept
    {
        std::array<std::pair<ListNode*, ListNode*>, N_Clusters> clusters{};
        ListNode* current_list_head = this->head(); // Start from the actual head of the list this node is in
        ListNode* p = current_list_head;
        while (p != nullptr)
        {
            ListNode* const current_node = p;
            p = p->next_;
            current_node->remove();
            std::size_t cluster_index = key_func(static_cast<const T&>(*current_node));
            assert(cluster_index < N_Clusters && "Cluster index out of bounds");

            std::pair<ListNode*, ListNode*>& c = clusters[cluster_index];
            if (c.first == nullptr)
            {
                c.first  = current_node;
                c.second = current_node;
            }
            else
            {
                c.second->next_ = current_node;
                current_node->prev_ = c.second;
                c.second = current_node;
            }
        }
        // Reconstruct the list globally from clusters. This assumes all nodes were part of the initial list.
        // The list this node was part of is now empty, and the nodes are in clusters.
        // We need to link the clusters together.
        ListNode* new_global_head = nullptr;
        ListNode* new_global_tail = nullptr;
        for (const auto& c : clusters)
        {
            if (c.first != nullptr)
            {
                if (new_global_head == nullptr)
                {
                    new_global_head = c.first;
                    new_global_tail = c.second;
                }
                else
                {
                    new_global_tail->next_ = c.first;
                    c.first->prev_  = new_global_tail;
                    new_global_tail = c.second;
                }
            }
        }
        // After this, the list structure containing the original 'this' node is rebuilt.
        // 'this' node's prev/next pointers are now updated according to its new position.
    }

    void remove() noexcept
    {
        if (prev_ != nullptr) { prev_->next_ = next_; }
        if (next_ != nullptr) { next_->prev_ = prev_; }
        prev_ = nullptr;
        next_ = nullptr;
    }

protected:
    ~ListNode() noexcept { remove(); }

private:
    ListNode* prev_ = nullptr;
    ListNode* next_ = nullptr;
};

template <typename>
class Triggerable;

template <typename R, typename... A>
class Triggerable<R(A...)>
{
public:
    virtual R trigger(A... args) const = 0;
    virtual std::size_t key() const noexcept = 0;

    Triggerable() noexcept                         = default;
    Triggerable(const Triggerable&)                = default;
    Triggerable(Triggerable&&) noexcept            = default;
    Triggerable& operator=(const Triggerable&)     = default;
    Triggerable& operator=(Triggerable&&) noexcept = default;

protected:
    ~Triggerable() noexcept = default;
};

template <typename T_PortContent>
struct Port : protected ListNode<T_PortContent>
{
    using ListNode<T_PortContent>::linked;

    explicit constexpr operator bool() const noexcept { return this->linked(); }

    Port()                           = default;
    Port(const Port&)                = delete;
    Port(Port&&) noexcept(std::is_nothrow_move_constructible_v<ListNode<T_PortContent>>) = default;
    Port& operator=(const Port&)     = delete;
    Port& operator=(Port&&) noexcept(std::is_nothrow_move_assignable_v<ListNode<T_PortContent>>) = default;

protected:
    ~Port() noexcept = default;
};

template <bool Movable>
struct EnableCopyMove;

template <> struct EnableCopyMove<true> {
    EnableCopyMove() noexcept = default; EnableCopyMove(EnableCopyMove&&) noexcept = default; EnableCopyMove(const EnableCopyMove&) = default;
    EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default; EnableCopyMove& operator=(const EnableCopyMove&) = default;
    ~EnableCopyMove() noexcept = default;
};
template <> struct EnableCopyMove<false> {
    EnableCopyMove() noexcept = default; EnableCopyMove(EnableCopyMove&&) noexcept = delete; EnableCopyMove(const EnableCopyMove&) = delete;
    EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete; EnableCopyMove& operator=(const EnableCopyMove&) = delete;
    ~EnableCopyMove() noexcept = default;
};

} // namespace detail

// ====================================================================================================================

template <typename R, typename... A, std::size_t footprint_val, bool movable>
struct Behavior<R(A...), footprint_val, movable> : public detail::Port<detail::Triggerable<R(A...)>>,
                                                 private detail::EnableCopyMove<movable>
{
    using Signature = R(A...);
    using BasePort = detail::Port<detail::Triggerable<Signature>>;
    using FunType = Function<Signature, footprint_val>;

    template <typename F>
    Behavior(F&& fun,
             std::enable_if_t<
                 FunType::template is_valid_target_v<F> &&
                 !std::is_same_v<std::decay_t<F>, Behavior>, int> = 0) :
        fun_(std::forward<F>(fun)) {}

    Behavior(const Behavior&)                = delete;
    Behavior(Behavior&&) noexcept            = default;
    Behavior& operator=(const Behavior&)     = delete;
    Behavior& operator=(Behavior&&) noexcept = default;

    virtual ~Behavior() noexcept = default;

    // Calling a Behavior directly invokes its function but does NOT automatically
    // broadcast to other nodes linked via an Event.
    // To achieve broadcast/chaining, invoke the Event to which this Behavior is linked.
    R operator()(A... args)
    {
        return fun_(args...);
    }

private:
    R trigger(A... args) const final { return fun_(args...); }
    std::size_t key() const noexcept final { return 1; }
    FunType fun_;
};

template <typename... A>
struct Event<void(A...)> : public detail::Port<detail::Triggerable<void(A...)>>
{
    using Signature = void(A...);
    using BasePort = detail::Port<detail::Triggerable<Signature>>;

    using BasePort::operator bool;

    Event()                   = default;
    Event(const Event&)                = delete;
    Event(Event&&)            = default;
    Event& operator=(const Event&)     = delete;
    Event& operator=(Event&&) noexcept = default;

    Event& operator>>(BasePort& that) noexcept
    {
        this->merge(&that);
        // The clusterize call reorders the local sublist so that all key()==0 (Events)
        // come before key()==1 (Behaviors). This ensures a defined order if events are chained.
        // On AVR, this can be expensive. If you only ever do `Event >> Behavior` (no complex Event->Event
        // chains mixed with Behaviors where processing order of same-level events matters),
        // you could consider removing it for performance if the merge order is sufficient.
        static_cast<detail::ListNode<detail::Triggerable<Signature>>*>(this)->head()
            ->template clusterize<2>([](const detail::Triggerable<Signature>& x) { return x.key(); });
        return *this;
    }

    void operator()(A... args) const
    {
        // Iterate through the linked list starting from the item *after* this Event node.
        const detail::ListNode<detail::Triggerable<Signature>>* p = this->next();
        while (p != nullptr)
        {
            static_cast<const detail::Triggerable<Signature>*>(p)->trigger(args...);
            p = p->next();
        }
    }

    void detach() noexcept { this->remove(); }
    virtual ~Event() noexcept = default;

private:
    void trigger(A...) const final {} // Event node itself does nothing when triggered
    std::size_t key() const noexcept final { return 0; }

    friend Event& operator^(Event& le, BasePort& ri) noexcept { return le >> ri; }
    friend Event& operator^(BasePort& le, Event& ri) noexcept { return ri >> le; }
    friend Event& operator^(Event& le, Event& ri) noexcept { return le >> ri; }
};

template <typename R, typename... A> struct Event<R(A...)>;

// ====================================================================================================================

template <typename... T> struct Pushable final : public Behavior<void(const T&...), default_behavior_footprint> { using Behavior<void(const T&...), default_behavior_footprint>::Behavior; };
template <typename... T, std::size_t fp> struct Pushable<Footprint<fp>, T...> final : public Behavior<void(const T&...), fp> { using Behavior<void(const T&...), fp>::Behavior; };
template <> struct Pushable<void> final : public Behavior<void(), default_behavior_footprint> { using Behavior<void(), default_behavior_footprint>::Behavior; };
template <std::size_t fp> struct Pushable<Footprint<fp>, void> final : public Behavior<void(), fp> { using Behavior<void(), fp>::Behavior; };

template <typename... T> struct Pusher final : public Event<void(const T&...)> {};
template <> struct Pusher<void> final : public Event<void()> {};

template <typename... T> struct Pullable final : public Behavior<void(T&...), default_behavior_footprint> { using Behavior<void(T&...), default_behavior_footprint>::Behavior; };
template <typename... T, std::size_t fp> struct Pullable<Footprint<fp>, T...> final : public Behavior<void(T&...), fp> { using Behavior<void(T&...), fp>::Behavior; };

namespace detail { template <typename T_val> struct PullerArrowProxy { T_val value{}; T_val* operator->() noexcept { return &value; } }; }

template <typename... T> struct Puller final : public Event<void(T&...)> {};
template <typename T_param> struct Puller<T_param> final : public Event<void(T_param&)> {
    template <typename U = T_param> std::enable_if_t<std::is_default_constructible_v<U>, T_param> operator*() const { T_param out{}; this->operator()(out); return out; }
    template <typename U = T_param> auto operator->() const -> std::enable_if_t<std::is_default_constructible_v<U>, detail::PullerArrowProxy<T_param>> { detail::PullerArrowProxy<T_param> out; this->operator()(out.value); return out; }
};
template <> struct Puller<void> final : public Event<void()> {};

// ====================================================================================================================

template <typename T_val, typename In_type = T_val, typename Out_type = T_val> struct Latch {
    T_val value{}; Pushable<In_type> in = [this](const In_type& val) { value = static_cast<T_val>(val); }; Pullable<Out_type> out = [this](Out_type& val) { val = static_cast<Out_type>(value); };
};
template <typename T_val, typename Out_type = T_val> struct Lift {
    T_val value{}; Puller<T_val> in{}; Pusher<Out_type> out{}; Pushable<> trigger = [this] { if (in) in(value); if (out) out(static_cast<Out_type>(value)); };
};

// ====================================================================================================================

template <typename Out_type, typename... In_types> struct PushUnary;
template <std::size_t fp, typename Out_type, typename... In_types> struct PushUnary<Footprint<fp>, Out_type, In_types...> {
    template <typename F> PushUnary(F fun, std::enable_if_t<std::is_invocable_r_v<Out_type, F, In_types...>, int> = 0) :
        in([this, fun_ = std::move(fun)](const In_types&... val) { if constexpr (std::is_same_v<Out_type, void>) { fun_(val...); if (out) out(); } else { if (out) out(fun_(val...)); } }) {}
    Pushable<Footprint<(fp < 64) ? fp : 64>, In_types...> in; Pusher<Out_type> out;
};
template <typename Out_type, typename... In_types> struct PushUnary : public PushUnary<Footprint<default_behavior_footprint>, Out_type, In_types...> { using PushUnary<Footprint<default_behavior_footprint>, Out_type, In_types...>::PushUnary; };

template <typename Out_type, typename... In_types> struct PullUnary;
template <std::size_t fp, typename Out_type, typename... In_types> struct PullUnary<Footprint<fp>, Out_type, In_types...> {
    template <typename F> PullUnary(F&& fun, std::enable_if_t<std::is_invocable_r_v<Out_type, F, In_types...> && (sizeof...(In_types) > 0), int> = 0) : PullUnary(std::forward<F>(fun), In_types{}...) {}
    template <typename F> PullUnary(F fun, In_types... initial_values, std::enable_if_t<std::is_invocable_r_v<Out_type, F, In_types...>, int> = 0) :
        value(std::move(initial_values)...), out([this, fun_ = std::move(fun)](Out_type& val_out) { if (in) apply_inputs(std::index_sequence_for<In_types...>{}); val_out = apply_function(fun_, std::index_sequence_for<In_types...>{}); }) {}
private: template <std::size_t... Is> void apply_inputs(std::index_sequence<Is...>) { if (in) in(std::get<Is>(value)...); }
    template <typename F_func, std::size_t... Is> Out_type apply_function(F_func& func, std::index_sequence<Is...>) { return func(std::get<Is>(value)...); }
public: std::tuple<In_types...> value{}; Puller<In_types...> in{}; Pullable<Footprint<(fp < 64) ? fp : 64>, Out_type> out;
};
template <std::size_t fp, typename Out_type, typename In_single> struct PullUnary<Footprint<fp>, Out_type, In_single> {
    template <typename F> PullUnary(F fun, In_single initial_value = {}, std::enable_if_t<std::is_invocable_r_v<Out_type, F, In_single>, int> = 0) :
        value(std::move(initial_value)), out([this, fun_ = std::move(fun)](Out_type& val_out) { if (in) in(value); val_out = fun_(value); }) {}
    In_single value; Puller<In_single> in{}; Pullable<Footprint<(fp < 64) ? fp : 64>, Out_type> out;
};
template <typename Out_type, typename... In_types> struct PullUnary : public PullUnary<Footprint<default_behavior_footprint>, Out_type, In_types...> { using PullUnary<Footprint<default_behavior_footprint>, Out_type, In_types...>::PullUnary; };

template <typename Out_type, typename... In_types> struct PullNary;
template <std::size_t fp, typename Out_type, typename... In_types> struct PullNary<Footprint<fp>, Out_type, In_types...> {
    template <typename F> PullNary(F&& fun, std::enable_if_t<std::is_invocable_r_v<Out_type, F, In_types...> && (sizeof...(In_types) > 0), int> = 0) : PullNary(std::forward<F>(fun), In_types{}...) {}
    template <typename F> PullNary(F fun, In_types... initial_values, std::enable_if_t<std::is_invocable_r_v<Out_type, F, In_types...>, int> = 0) :
        value(std::move(initial_values)...), out([this, fun_ = std::move(fun)](Out_type& val_out) { apply_to_pullers(std::index_sequence_for<In_types...>{}); val_out = apply_function(fun_, std::index_sequence_for<In_types...>{}); }) {}
private: template <std::size_t... Is> void apply_to_pullers(std::index_sequence<Is...>) { bool dummy[] = {(std::get<Is>(in) ? (std::get<Is>(in)(std::get<Is>(value)), true) : false)...}; (void)dummy; }
    template <typename F_func, std::size_t... Is> Out_type apply_function(F_func& func, std::index_sequence<Is...>) { return func(std::get<Is>(value)...); }
public: std::tuple<In_types...> value; std::tuple<Puller<In_types>...> in{}; Pullable<Footprint<(fp < 64) ? fp : 64>, Out_type> out;
};
template <typename Out_type, typename... In_types> struct PullNary : public PullNary<Footprint<default_behavior_footprint>, Out_type, In_types...> { using PullNary<Footprint<default_behavior_footprint>, Out_type, In_types...>::PullNary; };

// ====================================================================================================================

template <typename To, typename From> struct PushCast final : public PushUnary<Footprint<sizeof(void*)>, To, From> { PushCast() : PushUnary<Footprint<sizeof(void*)>, To, From>([](const From& val) { return static_cast<To>(val); }) {} };
template <typename To, typename From> struct PullCast final : public PullUnary<Footprint<sizeof(void*)>, To, From> { PullCast() : PullUnary<Footprint<sizeof(void*)>, To, From>([](const From& val) { return static_cast<To>(val); }) {} };

// ====================================================================================================================

struct Ctor final { template <typename F> Ctor(F&& fun, std::enable_if_t<std::is_invocable_r_v<void, F>, int> = 0) { std::forward<F>(fun)(); } };

template <std::size_t footprint_val = sizeof(void*) * 4> class Finalizer final {
public: using Fun = Function<void(), footprint_val>;
    Finalizer() noexcept : Finalizer([] {}) {}
    template <typename F> Finalizer(F&& action, std::enable_if_t<Fun::template is_valid_target_v<F>, int> = 0) noexcept : act_(std::forward<F>(action)) {}
    Finalizer(const Finalizer&) = delete;
    Finalizer(Finalizer&& that) noexcept : act_(std::move(that.act_)) { that.act_ = []{}; }
    Finalizer& operator=(const Finalizer&) = delete;
    Finalizer& operator=(Finalizer&& that) noexcept { if (this != &that) { if (act_) act_(); act_ = std::move(that.act_); that.act_ = []{}; } return *this; }
    ~Finalizer() noexcept { if (act_) act_(); }
    void disarm() noexcept { act_ = []{}; }
private: Fun act_;
};

} // namespace ramen
