#pragma once

/// C++ port of GPUI's app module.
/// Provides the central application state, entity management, and event system.

#include <gpui/action.h>
#include <gpui/collections.h>
#include <gpui/executor.h>
#include <gpui/platform.h>
#include <gpui/util.h>
#include <any>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace gpui {

// Forward declarations
class Window;
class App;
template <typename T> class Entity;
template <typename T> class Context;

// ─── Entity System ─────────────────────────────────────────────────────────

/// An opaque handle to any entity, without type information.
struct AnyEntity {
    uint64_t id = 0;
    std::type_index type = typeid(void);

    bool operator==(const AnyEntity& other) const { return id == other.id; }
    bool operator!=(const AnyEntity& other) const { return id != other.id; }
};

/// A type-safe handle to a managed entity.
/// Entities are application state objects managed by the GPUI framework.
template <typename T>
class Entity {
public:
    Entity() = default;
    explicit Entity(uint64_t id) : id_(id) {}

    /// Get the entity's unique ID.
    uint64_t id() const { return id_; }

    /// Read the entity's state.
    const T& read(const App& cx) const;

    /// Update the entity's state.
    template <typename F>
    auto update(App& cx, F&& callback) -> decltype(callback(std::declval<T&>(), std::declval<Context<T>&>()));

    /// Convert to an untyped entity handle.
    AnyEntity to_any() const {
        return AnyEntity{id_, std::type_index(typeid(T))};
    }

    bool operator==(const Entity& other) const { return id_ == other.id_; }
    bool operator!=(const Entity& other) const { return id_ != other.id_; }

    /// Check if this handle is valid (non-zero ID).
    bool is_valid() const { return id_ != 0; }
    explicit operator bool() const { return is_valid(); }

private:
    uint64_t id_ = 0;
};

// ─── Subscription ──────────────────────────────────────────────────────────

/// A handle to an active subscription (observer, event listener, etc.).
/// The subscription is automatically cancelled when this handle is dropped.
class Subscription {
public:
    Subscription() = default;
    explicit Subscription(std::function<void()> cancel)
        : cancel_(std::make_shared<CancelGuard>(std::move(cancel))) {}

    /// Explicitly cancel the subscription.
    void cancel() {
        if (cancel_) cancel_->cancel();
    }

    /// Release ownership without cancelling (the subscription lives forever).
    void detach() { cancel_.reset(); }

    bool is_active() const { return cancel_ && cancel_->active; }

private:
    struct CancelGuard {
        std::function<void()> on_cancel;
        bool active = true;

        explicit CancelGuard(std::function<void()> cb) : on_cancel(std::move(cb)) {}
        ~CancelGuard() { cancel(); }
        void cancel() {
            if (active && on_cancel) {
                active = false;
                on_cancel();
            }
        }
    };
    std::shared_ptr<CancelGuard> cancel_;
};

// ─── Context ───────────────────────────────────────────────────────────────

/// A context for interacting with a specific entity.
/// Provides access to the app state plus the ability to notify observers.
template <typename T>
class Context {
public:
    Context(App& app, uint64_t entity_id)
        : app_(app), entity_id_(entity_id) {}

    /// Get the app context.
    App& app() { return app_; }
    const App& app() const { return app_; }

    /// Notify observers that this entity has changed.
    void notify();

    /// Emit an event from this entity.
    template <typename Event>
    void emit(Event event);

    /// Observe another entity for changes.
    template <typename U>
    Subscription observe(Entity<U> entity, std::function<void(T&, Entity<U>, Context<T>&)> callback);

    /// Subscribe to events from another entity.
    template <typename U, typename Event>
    Subscription subscribe(Entity<U> entity, std::function<void(T&, const Event&, Context<T>&)> callback);

private:
    App& app_;
    uint64_t entity_id_;
};

// ─── Window Handle ─────────────────────────────────────────────────────────

/// An opaque handle to a window.
struct WindowHandle {
    uint64_t id = 0;

    bool operator==(const WindowHandle& other) const { return id == other.id; }
    bool operator!=(const WindowHandle& other) const { return id != other.id; }
};

// ─── Global State ──────────────────────────────────────────────────────────

/// Type-erased global state storage.
using GlobalId = std::type_index;

// ─── Event Emitter ─────────────────────────────────────────────────────────

/// Concept for types that can emit events of type E.
template <typename T, typename E>
concept EventEmitter = requires {
    typename T::Events;  // The type must declare an Events type alias
};

// ─── App ───────────────────────────────────────────────────────────────────

/// The central application state object.
/// Manages entities, windows, globals, subscriptions, and the event loop.
class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // ─── Entity Management ─────────────────────────────────────────────

    /// Create a new entity with the given initial state.
    template <typename T, typename F>
    Entity<T> new_entity(F&& initializer) {
        uint64_t id = next_entity_id_++;
        auto state = std::make_shared<T>();
        Context<T> ctx(*this, id);
        initializer(*state, ctx);
        entities_[id] = EntitySlot{std::type_index(typeid(T)), std::move(state)};
        return Entity<T>(id);
    }

    /// Create a new entity with a default-constructed value.
    template <typename T>
    Entity<T> new_entity(T value) {
        uint64_t id = next_entity_id_++;
        entities_[id] = EntitySlot{
            std::type_index(typeid(T)),
            std::make_shared<T>(std::move(value))
        };
        return Entity<T>(id);
    }

    /// Read an entity's state.
    template <typename T>
    const T& read_entity(Entity<T> entity) const {
        auto it = entities_.find(entity.id());
        assert(it != entities_.end() && "Entity not found");
        return *std::static_pointer_cast<T>(it->second.data);
    }

    /// Update an entity's state mutably.
    template <typename T, typename F>
    auto update_entity(Entity<T> entity, F&& callback)
        -> decltype(callback(std::declval<T&>(), std::declval<Context<T>&>())) {
        auto it = entities_.find(entity.id());
        assert(it != entities_.end() && "Entity not found");
        auto& state = *std::static_pointer_cast<T>(it->second.data);
        Context<T> ctx(*this, entity.id());
        return callback(state, ctx);
    }

    /// Release an entity, removing it from the app.
    void release_entity(uint64_t id) {
        entities_.erase(id);
        observers_.erase(id);
        // Remove all subscriptions for this entity
        for (auto it = subscribers_.begin(); it != subscribers_.end();) {
            if (it->first.entity_id == id) {
                it = subscribers_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // ─── Global State ──────────────────────────────────────────────────

    /// Set a global value of type T.
    template <typename T>
    void set_global(T value) {
        globals_[std::type_index(typeid(T))] = std::make_any<T>(std::move(value));
    }

    /// Get a reference to a global value.
    template <typename T>
    T& global() {
        auto it = globals_.find(std::type_index(typeid(T)));
        assert(it != globals_.end() && "Global not found");
        return std::any_cast<T&>(it->second);
    }

    /// Get a const reference to a global value.
    template <typename T>
    const T& global() const {
        auto it = globals_.find(std::type_index(typeid(T)));
        assert(it != globals_.end() && "Global not found");
        return std::any_cast<const T&>(it->second);
    }

    /// Check if a global of type T exists.
    template <typename T>
    bool has_global() const {
        return globals_.count(std::type_index(typeid(T))) > 0;
    }

    /// Get or insert a default global value.
    template <typename T>
    T& default_global() {
        auto key = std::type_index(typeid(T));
        auto it = globals_.find(key);
        if (it == globals_.end()) {
            globals_[key] = std::make_any<T>(T{});
            it = globals_.find(key);
        }
        return std::any_cast<T&>(it->second);
    }

    // ─── Observation ───────────────────────────────────────────────────

    /// Observe an entity for changes.
    Subscription observe_entity(uint64_t entity_id, std::function<void()> callback) {
        auto& observers = observers_[entity_id];
        auto id = next_observer_id_++;
        observers[id] = std::move(callback);
        return Subscription([this, entity_id, id]() {
            auto it = observers_.find(entity_id);
            if (it != observers_.end()) {
                it->second.erase(id);
            }
        });
    }

    /// Notify all observers of an entity.
    void notify_entity(uint64_t entity_id) {
        auto it = observers_.find(entity_id);
        if (it != observers_.end()) {
            // Copy to avoid iterator invalidation
            auto callbacks = it->second;
            for (auto& [_, cb] : callbacks) {
                cb();
            }
        }
    }

    // ─── Event Subscription ────────────────────────────────────────────

    /// Subscribe to events of type Event from an entity.
    template <typename Event>
    Subscription subscribe_entity(uint64_t entity_id,
                                   std::function<void(const Event&)> callback) {
        auto key = SubscriberKey{entity_id, std::type_index(typeid(Event))};
        auto id = next_observer_id_++;
        subscribers_[key][id] = [cb = std::move(callback)](const std::any& event) {
            cb(std::any_cast<const Event&>(event));
        };
        return Subscription([this, key, id]() {
            auto it = subscribers_.find(key);
            if (it != subscribers_.end()) {
                it->second.erase(id);
            }
        });
    }

    /// Emit an event from an entity.
    template <typename Event>
    void emit_event(uint64_t entity_id, Event event) {
        auto key = SubscriberKey{entity_id, std::type_index(typeid(Event))};
        auto it = subscribers_.find(key);
        if (it != subscribers_.end()) {
            auto callbacks = it->second;
            std::any boxed = std::move(event);
            for (auto& [_, cb] : callbacks) {
                cb(boxed);
            }
        }
    }

    // ─── Action Dispatch ───────────────────────────────────────────────

    /// Register a global action handler.
    template <typename A>
    Subscription on_action(std::function<void(const A&)> handler) {
        auto key = std::type_index(typeid(A));
        auto id = next_observer_id_++;
        action_handlers_[key][id] = [h = std::move(handler)](const Action& action) {
            h(static_cast<const A&>(action));
        };
        return Subscription([this, key, id]() {
            auto it = action_handlers_.find(key);
            if (it != action_handlers_.end()) {
                it->second.erase(id);
            }
        });
    }

    /// Dispatch an action to registered handlers.
    void dispatch_action(const Action& action) {
        auto it = action_handlers_.find(action.type_id());
        if (it != action_handlers_.end()) {
            auto handlers = it->second;
            for (auto& [_, handler] : handlers) {
                handler(action);
            }
        }
    }

    // ─── Window Management ─────────────────────────────────────────────

    /// Open a new window with the given options and root view.
    WindowHandle open_window(const WindowOptions& options,
                             std::function<void(Window&, App&)> build_root);

    /// Update a window's content.
    void update_window(WindowHandle handle, std::function<void(Window&, App&)> callback);

    /// Get all open windows.
    const std::vector<WindowHandle>& windows() const { return window_handles_; }

    /// Close a window.
    void close_window(WindowHandle handle);

    // ─── Executor ──────────────────────────────────────────────────────

    /// Get the executor for spawning async tasks.
    Executor& executor() { return executor_; }

    /// Spawn a background task.
    template <typename F, typename R = std::invoke_result_t<F>>
    TaskHandle<R> spawn(F&& func) {
        return executor_.spawn(std::forward<F>(func));
    }

    // ─── Platform ──────────────────────────────────────────────────────

    /// Get the platform interface.
    Platform& platform() { return *platform_; }

    /// Quit the application.
    void quit() { platform_->quit(); }

    // ─── Deferred Effects ──────────────────────────────────────────────

    /// Schedule a callback to run after the current update cycle.
    void defer(std::function<void(App&)> callback) {
        deferred_.push_back(std::move(callback));
    }

    /// Flush all deferred effects.
    void flush_effects();

private:
    struct EntitySlot {
        std::type_index type = typeid(void);
        std::shared_ptr<void> data;
    };

    struct SubscriberKey {
        uint64_t entity_id;
        std::type_index event_type;

        bool operator==(const SubscriberKey& other) const {
            return entity_id == other.entity_id && event_type == other.event_type;
        }
    };

    struct SubscriberKeyHash {
        std::size_t operator()(const SubscriberKey& k) const {
            return std::hash<uint64_t>{}(k.entity_id) ^
                   (std::hash<std::type_index>{}(k.event_type) << 32);
        }
    };

    // Entity storage
    uint64_t next_entity_id_ = 1;
    std::unordered_map<uint64_t, EntitySlot> entities_;

    // Observers (entity change notifications)
    uint64_t next_observer_id_ = 1;
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, std::function<void()>>> observers_;

    // Event subscribers
    std::unordered_map<SubscriberKey,
                       std::unordered_map<uint64_t, std::function<void(const std::any&)>>,
                       SubscriberKeyHash> subscribers_;

    // Action handlers
    std::unordered_map<std::type_index,
                       std::unordered_map<uint64_t, std::function<void(const Action&)>>> action_handlers_;

    // Global state
    std::unordered_map<std::type_index, std::any> globals_;

    // Windows
    std::vector<WindowHandle> window_handles_;
    std::unordered_map<uint64_t, std::unique_ptr<Window>> windows_;
    uint64_t next_window_id_ = 1;

    // Deferred effects
    std::vector<std::function<void(App&)>> deferred_;

    // Platform and executor
    std::unique_ptr<Platform> platform_;
    Executor executor_;
};

// ─── Entity method implementations ────────────────────────────────────────

template <typename T>
const T& Entity<T>::read(const App& cx) const {
    return cx.read_entity(*this);
}

template <typename T>
template <typename F>
auto Entity<T>::update(App& cx, F&& callback)
    -> decltype(callback(std::declval<T&>(), std::declval<Context<T>&>())) {
    return cx.update_entity(*this, std::forward<F>(callback));
}

// ─── Context method implementations ───────────────────────────────────────

template <typename T>
void Context<T>::notify() {
    app_.notify_entity(entity_id_);
}

template <typename T>
template <typename Event>
void Context<T>::emit(Event event) {
    app_.emit_event(entity_id_, std::move(event));
}

template <typename T>
template <typename U>
Subscription Context<T>::observe(Entity<U> entity,
                                  std::function<void(T&, Entity<U>, Context<T>&)> callback) {
    auto self_id = entity_id_;
    return app_.observe_entity(entity.id(), [this, self_id, entity, cb = std::move(callback)]() {
        app_.update_entity(Entity<T>(self_id), [&](T& self, Context<T>& ctx) {
            cb(self, entity, ctx);
        });
    });
}

template <typename T>
template <typename U, typename Event>
Subscription Context<T>::subscribe(Entity<U> entity,
                                    std::function<void(T&, const Event&, Context<T>&)> callback) {
    auto self_id = entity_id_;
    return app_.subscribe_entity<Event>(entity.id(),
        [this, self_id, cb = std::move(callback)](const Event& event) {
            app_.update_entity(Entity<T>(self_id), [&](T& self, Context<T>& ctx) {
                cb(self, event, ctx);
            });
        });
}

// ─── Application Builder ──────────────────────────────────────────────────

/// Builder for configuring and launching a GPUI application.
class Application {
public:
    Application() : app_(std::make_shared<App>()) {}

    /// Run the application with a setup callback.
    void run(std::function<void(App&)> on_ready) {
        app_->platform().run([this, cb = std::move(on_ready)]() {
            cb(*app_);
        });
    }

    /// Set the application to run in headless mode (no windows).
    Application& headless() {
        headless_ = true;
        return *this;
    }

    /// Access the app before running.
    App& app() { return *app_; }

private:
    std::shared_ptr<App> app_;
    bool headless_ = false;
};

} // namespace gpui
