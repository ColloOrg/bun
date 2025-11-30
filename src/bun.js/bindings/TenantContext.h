#pragma once

#include <wtf/HashMap.h>
#include <wtf/Lock.h>
#include <wtf/text/WTFString.h>
#include <atomic>
#include <cstdint>

namespace Bun {

// ════════════════════════════════════════════════════════════════
// TenantContext - Minimal Multi-Tenant Isolation
//
// Holds per-tenant state:
//   - ID and name
//   - Rate limits (timers, fetches)
//   - Isolated environment variables
//   - Lifecycle state
//
// Thread-safe: Uses atomic operations for counters
// ════════════════════════════════════════════════════════════════

class TenantContext {
public:
    uint64_t id;
    WTF::String name;

    // Rate Limits
    struct Limits {
        uint32_t maxTimers = 1000;
        uint32_t maxConcurrentFetches = 100;
    } limits;

    // Atomic counters
    std::atomic<uint32_t> timerCount{0};
    std::atomic<uint32_t> fetchCount{0};

    // Lifecycle state
    enum class State : uint8_t {
        Active = 0,
        Draining = 1,
        Terminated = 2
    };
    std::atomic<State> state{State::Active};

    // Constructor
    explicit TenantContext(uint64_t tenantId, WTF::String tenantName = WTF::String())
        : id(tenantId)
        , name(WTFMove(tenantName))
    {
    }

    // State management
    bool isActive() const {
        return state.load(std::memory_order_acquire) == State::Active;
    }

    void startDraining() {
        State expected = State::Active;
        state.compare_exchange_strong(expected, State::Draining, std::memory_order_acq_rel);
    }

    void terminate() {
        State expected = State::Active;
        if (!state.compare_exchange_strong(expected, State::Terminated, std::memory_order_acq_rel)) {
            expected = State::Draining;
            state.compare_exchange_strong(expected, State::Terminated, std::memory_order_acq_rel);
        }
    }

    // Atomic rate limit operations (TOCTOU-safe)
    ALWAYS_INLINE bool tryIncrementTimerCount() {
        uint32_t current = timerCount.load(std::memory_order_relaxed);
        do {
            if (current >= limits.maxTimers)
                return false;
        } while (!timerCount.compare_exchange_weak(current, current + 1,
            std::memory_order_relaxed, std::memory_order_relaxed));
        return true;
    }

    ALWAYS_INLINE bool tryIncrementFetchCount() {
        uint32_t current = fetchCount.load(std::memory_order_relaxed);
        do {
            if (current >= limits.maxConcurrentFetches)
                return false;
        } while (!fetchCount.compare_exchange_weak(current, current + 1,
            std::memory_order_relaxed, std::memory_order_relaxed));
        return true;
    }

    ALWAYS_INLINE void decrementTimerCount() {
        uint32_t current = timerCount.load(std::memory_order_relaxed);
        while (current > 0 && !timerCount.compare_exchange_weak(current, current - 1,
            std::memory_order_relaxed, std::memory_order_relaxed));
    }

    ALWAYS_INLINE void decrementFetchCount() {
        uint32_t current = fetchCount.load(std::memory_order_relaxed);
        while (current > 0 && !fetchCount.compare_exchange_weak(current, current - 1,
            std::memory_order_relaxed, std::memory_order_relaxed));
    }

    // Environment variables (thread-safe)
    WTF::String getEnv(const WTF::String& key) const {
        Locker locker { m_envLock };
        auto it = m_env.find(key);
        if (it != m_env.end())
            return it->value;
        return WTF::String();
    }

    void setEnv(const WTF::String& key, const WTF::String& value) {
        Locker locker { m_envLock };
        m_env.set(key, value);
    }

    bool hasEnv(const WTF::String& key) const {
        Locker locker { m_envLock };
        return m_env.contains(key);
    }

private:
    mutable Lock m_envLock;
    WTF::HashMap<WTF::String, WTF::String> m_env WTF_GUARDED_BY_LOCK(m_envLock);
};

} // namespace Bun

// ════════════════════════════════════════════════════════════════
// C API for Zig FFI
// ════════════════════════════════════════════════════════════════

extern "C" {

// Lifecycle
Bun::TenantContext* TenantContext__create(uint64_t id);
void TenantContext__destroy(Bun::TenantContext* ctx);

// State
bool TenantContext__isActive(Bun::TenantContext* ctx);
void TenantContext__startDraining(Bun::TenantContext* ctx);
void TenantContext__terminate(Bun::TenantContext* ctx);

// Rate limiting
bool TenantContext__tryIncrementTimerCount(Bun::TenantContext* ctx);
bool TenantContext__tryIncrementFetchCount(Bun::TenantContext* ctx);
void TenantContext__decrementTimerCount(Bun::TenantContext* ctx);
void TenantContext__decrementFetchCount(Bun::TenantContext* ctx);

// Environment
void TenantContext__setEnv(Bun::TenantContext* ctx, const char* key, size_t keyLen, const char* value, size_t valueLen);
size_t TenantContext__getEnv(Bun::TenantContext* ctx, const char* key, size_t keyLen, char* outBuffer, size_t bufferSize);
bool TenantContext__hasEnv(Bun::TenantContext* ctx, const char* key, size_t keyLen);

}
