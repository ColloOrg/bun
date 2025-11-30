#include "root.h"
#include "TenantContext.h"
#include <wtf/text/CString.h>

extern "C" {

Bun::TenantContext* TenantContext__create(uint64_t id) {
    return new Bun::TenantContext(id);
}

void TenantContext__destroy(Bun::TenantContext* ctx) {
    delete ctx;
}

bool TenantContext__isActive(Bun::TenantContext* ctx) {
    return ctx ? ctx->isActive() : false;
}

void TenantContext__startDraining(Bun::TenantContext* ctx) {
    if (ctx) ctx->startDraining();
}

void TenantContext__terminate(Bun::TenantContext* ctx) {
    if (ctx) ctx->terminate();
}

bool TenantContext__tryIncrementTimerCount(Bun::TenantContext* ctx) {
    return ctx ? ctx->tryIncrementTimerCount() : true;
}

bool TenantContext__tryIncrementFetchCount(Bun::TenantContext* ctx) {
    return ctx ? ctx->tryIncrementFetchCount() : true;
}

void TenantContext__decrementTimerCount(Bun::TenantContext* ctx) {
    if (ctx) ctx->decrementTimerCount();
}

void TenantContext__decrementFetchCount(Bun::TenantContext* ctx) {
    if (ctx) ctx->decrementFetchCount();
}

void TenantContext__setEnv(Bun::TenantContext* ctx, const char* key, size_t keyLen, const char* value, size_t valueLen) {
    if (!ctx || !key || !value) return;

    WTF::String keyStr = WTF::String::fromUTF8(std::span<const char>(key, keyLen));
    WTF::String valueStr = WTF::String::fromUTF8(std::span<const char>(value, valueLen));

    if (keyStr.isNull() || valueStr.isNull()) return;

    ctx->setEnv(keyStr, valueStr);
}

size_t TenantContext__getEnv(Bun::TenantContext* ctx, const char* key, size_t keyLen, char* outBuffer, size_t bufferSize) {
    if (!ctx || !key) return SIZE_MAX;

    WTF::String keyStr = WTF::String::fromUTF8(std::span<const char>(key, keyLen));
    if (keyStr.isNull()) return SIZE_MAX;

    if (!ctx->hasEnv(keyStr))
        return 0;

    WTF::String value = ctx->getEnv(keyStr);
    if (value.isEmpty())
        return 0;

    CString utf8 = value.utf8();
    size_t len = utf8.length();

    if (outBuffer && bufferSize > len) {
        memcpy(outBuffer, utf8.data(), len);
        outBuffer[len] = '\0';
    }

    return len;
}

bool TenantContext__hasEnv(Bun::TenantContext* ctx, const char* key, size_t keyLen) {
    if (!ctx || !key) return false;

    WTF::String keyStr = WTF::String::fromUTF8(std::span<const char>(key, keyLen));
    if (keyStr.isNull()) return false;

    return ctx->hasEnv(keyStr);
}

} // extern "C"
