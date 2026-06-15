#include "event_dispatcher.hpp"

subscription_guard::subscription_guard(event_dispatcher& dispatcher, uint64_t id)
    : dispatcher(&dispatcher), id(id) {}

subscription_guard::~subscription_guard() {
    reset();
}

subscription_guard::subscription_guard(subscription_guard&& other) noexcept
    : dispatcher(other.dispatcher), id(other.id) {
    other.dispatcher = nullptr;
    other.id = 0;
}

subscription_guard& subscription_guard::operator=(subscription_guard&& other) noexcept {
    if (this != &other) {
        reset();
        dispatcher = other.dispatcher;
        id = other.id;
        other.dispatcher = nullptr;
        other.id = 0;
    }
    return *this;
}

void subscription_guard::reset() {
    if (dispatcher && id != 0) {
        dispatcher->unsubscribe(id);
        dispatcher = nullptr;
        id = 0;
    }
}

void event_dispatcher::unsubscribe(subscription_id id) {
    for (auto& [key, list] : subscribers) {
        list->remove(id);
    }
}
