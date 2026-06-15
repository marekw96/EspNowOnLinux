#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <vector>
#include <functional>
#include <algorithm>

class event_dispatcher;

class subscription_guard {
public:
    subscription_guard() = default;

    subscription_guard(event_dispatcher& dispatcher, uint64_t id);

    ~subscription_guard();

    // Move only
    subscription_guard(const subscription_guard&) = delete;
    subscription_guard& operator=(const subscription_guard&) = delete;

    subscription_guard(subscription_guard&& other) noexcept;

    subscription_guard& operator=(subscription_guard&& other) noexcept;

    void reset();

    uint64_t get_id() const { return id; }

private:
    event_dispatcher* dispatcher = nullptr;
    uint64_t id = 0;
};

class event_dispatcher {
public:
    using subscription_id = uint64_t;

private:
    struct handlers_base {
        virtual ~handlers_base() = default;
        virtual void remove(subscription_id id) = 0;
    };

    template<typename Event>
    struct handlers : handlers_base {
        using argument_type = typename Event::argument_type;
        std::vector<std::pair<subscription_id, std::function<void(argument_type)>>> handlers;

        void remove(subscription_id id) override {
            std::erase_if(handlers, [id](const auto& pair) {
                return pair.first == id;
            });
        }
    };

public:
    template<typename Event, typename Func>
    [[nodiscard]] subscription_guard subscribe(Func&& func) {
        using List = handlers<Event>;

        auto key = std::type_index(typeid(Event));

        if (!subscribers.contains(key)) {
            subscribers[key] = std::make_unique<List>();
        }

        auto* list = static_cast<List*>(subscribers[key].get());

        subscription_id id = ++next_id;
        list->handlers.emplace_back(id, std::forward<Func>(func));
        return subscription_guard(*this, id);
    }

    void unsubscribe(subscription_id id);

    template<typename Event>
    void publish(typename Event::argument_type argument) {
        auto key = std::type_index(typeid(Event));

        auto it = subscribers.find(key);
        if (it == subscribers.end()) {
            return;
        }

        auto* list = static_cast<handlers<Event>*>(it->second.get());

        for (auto& handler : list->handlers) {
            handler.second(argument);
        }
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<handlers_base>> subscribers;
    subscription_id next_id = 0;
};