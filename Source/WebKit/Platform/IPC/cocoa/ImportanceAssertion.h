/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if PLATFORM(MAC) || (PLATFORM(QT) && USE(MACH_PORTS))

#include <mach/message.h>

namespace IPC {

class ImportanceAssertion {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(ImportanceAssertion);

public:
    explicit ImportanceAssertion(mach_msg_header_t* header)
        : m_voucher(0)
    {
        if (MACH_MSGH_BITS_HAS_VOUCHER(header->msgh_bits)) {
            m_voucher = header->msgh_voucher_port;
            header->msgh_voucher_port = MACH_VOUCHER_NULL;
            header->msgh_bits &= ~(MACH_MSGH_BITS_VOUCHER_MASK | MACH_MSGH_BITS_RAISEIMP);
        }
    }

    ~ImportanceAssertion()
    {
        if (m_voucher) {
            kern_return_t kr = mach_voucher_deallocate(m_voucher);
            ASSERT_UNUSED(kr, !kr);
        }
    }

private:
    mach_voucher_t m_voucher;
};

}

#endif // PLATFORM(MAC) || (PLATFORM(QT) && USE(MACH_PORTS))
