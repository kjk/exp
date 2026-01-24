#pragma once

/// C++ port of GPUI's action system.
/// Actions are named commands that can be bound to keys and dispatched through the UI tree.

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace gpui {

/// Type ID for actions, using C++ RTTI.
using ActionTypeId = std::type_index;

/// Base class for all actions.
/// Actions are value types that represent user commands (e.g., "Save", "Undo").
class Action {
public:
    virtual ~Action() = default;

    /// Get the unique type ID for this action.
    virtual ActionTypeId type_id() const = 0;

    /// Get the human-readable name of this action.
    virtual const std::string& name() const = 0;

    /// Clone this action.
    virtual std::unique_ptr<Action> clone() const = 0;
};

/// A typed action with associated data.
/// Use GPUI_ACTION macro to define actions easily.
template <typename T>
class TypedAction : public Action {
public:
    T data;

    TypedAction() = default;
    explicit TypedAction(T data) : data(std::move(data)) {}

    ActionTypeId type_id() const override {
        return std::type_index(typeid(T));
    }

    const std::string& name() const override {
        static std::string name = typeid(T).name();
        return name;
    }

    std::unique_ptr<Action> clone() const override {
        return std::make_unique<TypedAction<T>>(data);
    }
};

/// A simple action with no data (just a command name).
/// Define with: struct MyAction {};
template <typename Tag>
class SimpleAction : public Action {
public:
    ActionTypeId type_id() const override {
        return std::type_index(typeid(Tag));
    }

    const std::string& name() const override {
        static std::string name = typeid(Tag).name();
        return name;
    }

    std::unique_ptr<Action> clone() const override {
        return std::make_unique<SimpleAction<Tag>>();
    }
};

/// Registry for action constructors (used for deserialization from key bindings).
class ActionRegistry {
public:
    using Factory = std::function<std::unique_ptr<Action>()>;

    static ActionRegistry& instance() {
        static ActionRegistry registry;
        return registry;
    }

    /// Register an action type with its name.
    template <typename T>
    void register_action(const std::string& name) {
        factories_[name] = []() -> std::unique_ptr<Action> {
            return std::make_unique<T>();
        };
        type_names_[std::type_index(typeid(T))] = name;
    }

    /// Create an action by name.
    std::unique_ptr<Action> create(const std::string& name) const {
        auto it = factories_.find(name);
        if (it != factories_.end()) {
            return it->second();
        }
        return nullptr;
    }

    /// Get the registered name for an action type.
    std::optional<std::string> name_for(ActionTypeId type) const {
        auto it = type_names_.find(type);
        if (it != type_names_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    ActionRegistry() = default;
    std::unordered_map<std::string, Factory> factories_;
    std::unordered_map<ActionTypeId, std::string> type_names_;
};

/// Macro to define and register a simple action.
#define GPUI_ACTION(Name) \
    struct Name##Tag {}; \
    using Name = gpui::SimpleAction<Name##Tag>

/// Macro to register an action at static initialization time.
#define GPUI_REGISTER_ACTION(ActionType, name_str) \
    namespace { \
    struct ActionType##Registrar { \
        ActionType##Registrar() { \
            gpui::ActionRegistry::instance().register_action<ActionType>(name_str); \
        } \
    } action_##ActionType##_registrar; \
    }

} // namespace gpui
