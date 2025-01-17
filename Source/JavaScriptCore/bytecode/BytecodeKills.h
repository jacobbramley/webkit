/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2019 Arm Ltd. All rights reserved.
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

#include "CodeBlock.h"
#include <wtf/UniqueArray.h>
#include <wtf/PointerMacro.h>

namespace JSC {

class BytecodeLivenessAnalysis;

class BytecodeKills {
    WTF_MAKE_FAST_ALLOCATED;
public:
    BytecodeKills()
        : m_codeBlock(nullptr)
    {
    }
    
private:
    friend class BytecodeLivenessAnalysis;

    class KillSet {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        KillSet()
            : m_word(0)
        {
        }

        ~KillSet()
        {
            if (hasVector())
                delete vector();
        }
        
        void add(unsigned local)
        {
            if (isEmpty()) {
                setOneItem(local);
                return;
            }
            if (hasOneItem()) {
                ASSERT(oneItem() != local);
                Vector<unsigned>* vector = new Vector<unsigned>();
                vector->append(oneItem());
                vector->append(local);
                setVector(vector);
                return;
            }
            ASSERT(!vector()->contains(local));
            vector()->append(local);
        }
        
        template<typename Functor>
        void forEachLocal(const Functor& functor)
        {
            if (isEmpty())
                return;
            if (hasOneItem()) {
                functor(oneItem());
                return;
            }
            for (unsigned local : *vector())
                functor(local);
        }
        
        bool contains(unsigned expectedLocal)
        {
            if (isEmpty())
                return false;
            if (hasOneItem())
                return oneItem() == expectedLocal;
            for (unsigned local : *vector()) {
                if (local == expectedLocal)
                    return true;
            }
            return false;
        }
        
    private:
        bool isEmpty() const
        {
            return !m_word;
        }
        
        bool hasOneItem() const
        {
            return WTF::Pointer::getLowBits<1u>(m_word);
        }
        
        unsigned oneItem() const
        {
            return uintptr_t(m_word) >> 1;
        }
        
        void setOneItem(unsigned value)
        {
            m_word = (value << 1) | 1;
        }
        
        bool hasVector() const
        {
            return !isEmpty() && !hasOneItem();
        }
        
        Vector<unsigned>* vector()
        {
            return bitwise_cast<Vector<unsigned>*>(m_word);
        }
        
        void setVector(Vector<unsigned>* value)
        {
            m_word = bitwise_cast<uintptr_t>(value);
        }
        
        uintptr_t m_word;
    };
    
    CodeBlock* m_codeBlock;
    UniqueArray<KillSet> m_killSets;
};

} // namespace JSC
