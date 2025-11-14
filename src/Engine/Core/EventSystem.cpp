//
// Created by 2401Lucas on 2025-10-30.
//

#include "EventSystem.h"
#include <algorithm>
#include <stdexcept>

EventSystem::EventSystem()
    : m_nextHandleId(1) {
}

EventSystem::~EventSystem() {
    Clear();
}

EventHandle EventSystem::Subscribe(const std::string &eventType, EventCallback callback) {
    if (!callback) {
        throw std::invalid_argument("Cannot subscribe with null callback");
    }

    // Create new listener
    EventListener listener;
    listener.id = m_nextHandleId++;
    listener.callback = std::move(callback);

    // Add to listeners for this event type
    m_listeners[eventType].push_back(std::move(listener));

    // Return handle
    EventHandle handle;
    handle.id = listener.id;
    return handle;
}

void EventSystem::Unsubscribe(EventHandle handle) {
    if (!handle.IsValid()) {
        return;
    }

    // Search through all event types to find this handle
    for (auto &[eventType, listeners]: m_listeners) {
        auto it = std::find_if(listeners.begin(), listeners.end(),
                               [handle](const EventListener &listener) {
                                   return listener.id == handle.id;
                               });

        if (it != listeners.end()) {
            listeners.erase(it);
            return;
        }
    }
}

void EventSystem::UnsubscribeAll(const std::string &eventType) {
    auto it = m_listeners.find(eventType);
    if (it != m_listeners.end()) {
        it->second.clear();
    }
}

void EventSystem::Emit(const std::string &eventType, const EventData &data) {
#ifdef DEBUG_EVENTS
    printf(eventType.c_str());
#endif

    auto it = m_listeners.find(eventType);
    if (it == m_listeners.end()) {
        // No listeners for this event type
        return;
    }

    // Call all listeners for this event
    // Swap with an empty vector to avoid issues if callbacks modify the subscription list
    auto listenersCopy = it->second;
    for (const auto &listener: listenersCopy) {
        if (listener.callback) {
            listener.callback(data);
        }
    }
}

void EventSystem::EmitQueued(const std::string &eventType, const EventData &data) {
    QueuedEvent event;
    event.type = eventType;
    event.data = data;
    m_queuedEvents.push_back(std::move(event));
}

void EventSystem::ProcessQueued() {
    // Swap with an empty vector to avoid issues if callbacks queue more events
    std::vector<QueuedEvent> eventsToProcess;
    eventsToProcess.swap(m_queuedEvents);

    for (const auto &event: eventsToProcess) {
        Emit(event.type, event.data);
    }
}

void EventSystem::Clear() {
    m_listeners.clear();
    m_queuedEvents.clear();
}

size_t EventSystem::GetListenerCount(const std::string &eventType) const {
    auto it = m_listeners.find(eventType);
    if (it != m_listeners.end()) {
        return it->second.size();
    }
    return 0;
}
