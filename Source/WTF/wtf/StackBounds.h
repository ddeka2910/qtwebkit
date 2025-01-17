/*
 * Copyright (C) 2010-2017 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#pragma once

#include <algorithm>
#include <wtf/StackPointer.h>
#include <wtf/ThreadingPrimitives.h>

namespace WTF {

class StackBounds {
    WTF_MAKE_FAST_ALLOCATED;

    // This 64k number was picked because a sampling of stack usage differences
    // between consecutive entries into one of the Interpreter::execute...()
    // functions was seen to be as high as 27k. Hence, 64k is chosen as a
    // conservative availability value that is not too large but comfortably
    // exceeds 27k with some buffer for error.
    static constexpr size_t s_defaultAvailabilityDelta = 64 * 1024;

public:
    enum class StackDirection { Upward, Downward };

    static constexpr StackBounds emptyBounds() { return StackBounds(); }

#if HAVE(STACK_BOUNDS_FOR_NEW_THREAD)
    // This function is only effective for newly created threads. In some platform, it returns a bogus value for the main thread.
    static StackBounds newThreadStackBounds(PlatformThreadHandle);
#endif
    static StackBounds currentThreadStackBounds()
    {
        auto result = currentThreadStackBoundsInternal();
        result.checkConsistency();
        return result;
    }

    void* origin() const
    {
        ASSERT(m_origin);
        return m_origin;
    }

    void* end() const
    {
        ASSERT(m_bound);
        return m_bound;
    }
    
    size_t size() const
    {
        if (isGrowingDownward())
            return static_cast<char*>(m_origin) - static_cast<char*>(m_bound);
        return static_cast<char*>(m_bound) - static_cast<char*>(m_origin);
    }

    bool isEmpty() const { return !m_origin; }

    bool contains(void* p) const
    {
        if (isEmpty())
            return false;
        if (isGrowingDownward())
            return (m_origin >= p) && (p > m_bound);
        return (m_bound > p) && (p >= m_origin);
    }

    void* recursionLimit(size_t minAvailableDelta = s_defaultAvailabilityDelta) const
    {
        checkConsistency();
        if (isGrowingDownward())
            return static_cast<char*>(m_bound) + minAvailableDelta;
        return static_cast<char*>(m_bound) - minAvailableDelta;
    }

    void* recursionLimit(char* startOfUserStack, size_t maxUserStack, size_t reservedZoneSize) const
    {
        checkConsistency();
        if (maxUserStack < reservedZoneSize)
            reservedZoneSize = maxUserStack;
        size_t maxUserStackWithReservedZone = maxUserStack - reservedZoneSize;

        if (isGrowingDownward()) {
            char* endOfStackWithReservedZone = reinterpret_cast<char*>(m_bound) + reservedZoneSize;
            if (startOfUserStack < endOfStackWithReservedZone)
                return endOfStackWithReservedZone;
            size_t availableUserStack = startOfUserStack - endOfStackWithReservedZone;
            if (maxUserStackWithReservedZone > availableUserStack)
                maxUserStackWithReservedZone = availableUserStack;
            return startOfUserStack - maxUserStackWithReservedZone;
        }

        char* endOfStackWithReservedZone = reinterpret_cast<char*>(m_bound) - reservedZoneSize;
        if (startOfUserStack > endOfStackWithReservedZone)
            return endOfStackWithReservedZone;
        size_t availableUserStack = endOfStackWithReservedZone - startOfUserStack;
        if (maxUserStackWithReservedZone > availableUserStack)
            maxUserStackWithReservedZone = availableUserStack;
        return startOfUserStack + maxUserStackWithReservedZone;
    }

    bool isGrowingDownward() const
    {
        ASSERT(m_origin && m_bound);
        return m_bound <= m_origin;
    }

private:
    StackBounds(void* origin, void* end)
        : m_origin(origin)
        , m_bound(end)
    {
    }

    constexpr StackBounds()
        : m_origin(nullptr)
        , m_bound(nullptr)
    {
    }

    static StackDirection stackDirection();

    WTF_EXPORT_PRIVATE static StackBounds currentThreadStackBoundsInternal();

    void checkConsistency() const
    {
#if !ASSERT_DISABLED
        void* currentPosition = currentStackPointer();
        ASSERT(m_origin != m_bound);
        ASSERT(isGrowingDownward()
            ? (currentPosition < m_origin && currentPosition > m_bound)
            : (currentPosition > m_origin && currentPosition < m_bound));
#endif
    }

    void* m_origin;
    void* m_bound;

    friend class StackStats;
};

} // namespace WTF

using WTF::StackBounds;
