/*
 * Copyright (C) 2015-2018 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "ScopedArguments.h"

#include "GenericArgumentsInlines.h"
#include "JSCInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ScopedArguments);

const ClassInfo ScopedArguments::s_info = { "Arguments", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(ScopedArguments) };

ScopedArguments::ScopedArguments(VM& vm, Structure* structure, WriteBarrier<Unknown>* storage)
    : GenericArguments(vm, structure)
    , m_storage(vm, this, storage)
{
    ASSERT(!storageHeader(storage).overrodeThings);
}

void ScopedArguments::finishCreation(VM& vm, JSFunction* callee, ScopedArgumentsTable* table, JSLexicalEnvironment* scope)
{
    Base::finishCreation(vm);
    m_callee.set(vm, this, callee);
    m_table.set(vm, this, table);
    m_scope.set(vm, this, scope);
}

ScopedArguments* ScopedArguments::createUninitialized(VM& vm, Structure* structure, JSFunction* callee, ScopedArgumentsTable* table, JSLexicalEnvironment* scope, unsigned totalLength)
{
    unsigned overflowLength;
    if (totalLength > table->length())
        overflowLength = totalLength - table->length();
    else
        overflowLength = 0;
    
    void* rawStoragePtr = vm.jsValueGigacageAuxiliarySpace.allocateNonVirtual(
        vm, storageSize(overflowLength), nullptr, AllocationFailureMode::Assert);
    WriteBarrier<Unknown>* storage = static_cast<WriteBarrier<Unknown>*>(rawStoragePtr) + 1;
    storageHeader(storage).overrodeThings = false;
    storageHeader(storage).totalLength = totalLength;
    
    ScopedArguments* result = new (
        NotNull,
        allocateCell<ScopedArguments>(vm.heap))
        ScopedArguments(vm, structure, storage);
    result->finishCreation(vm, callee, table, scope);
    return result;
}

ScopedArguments* ScopedArguments::create(VM& vm, Structure* structure, JSFunction* callee, ScopedArgumentsTable* table, JSLexicalEnvironment* scope, unsigned totalLength)
{
    ScopedArguments* result =
        createUninitialized(vm, structure, callee, table, scope, totalLength);

    unsigned namedLength = table->length();
    for (unsigned i = namedLength; i < totalLength; ++i)
        result->overflowStorage()[i - namedLength].clear();
    
    return result;
}

ScopedArguments* ScopedArguments::createByCopying(JSGlobalObject* globalObject, CallFrame* callFrame, ScopedArgumentsTable* table, JSLexicalEnvironment* scope)
{
    return createByCopyingFrom(
        globalObject->vm(), globalObject->scopedArgumentsStructure(),
        callFrame->registers() + CallFrame::argumentOffset(0), callFrame->argumentCount(),
        jsCast<JSFunction*>(callFrame->jsCallee()), table, scope);
}

ScopedArguments* ScopedArguments::createByCopyingFrom(VM& vm, Structure* structure, Register* argumentsStart, unsigned totalLength, JSFunction* callee, ScopedArgumentsTable* table, JSLexicalEnvironment* scope)
{
    ScopedArguments* result =
        createUninitialized(vm, structure, callee, table, scope, totalLength);
    
    unsigned namedLength = table->length();
    for (unsigned i = namedLength; i < totalLength; ++i)
        result->overflowStorage()[i - namedLength].set(vm, result, argumentsStart[i].jsValue());
    
    return result;
}

void ScopedArguments::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    ScopedArguments* thisObject = static_cast<ScopedArguments*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);

    visitor.append(thisObject->m_callee);
    visitor.append(thisObject->m_table);
    visitor.append(thisObject->m_scope);
    
    visitor.markAuxiliary(&thisObject->storageHeader());
    
    if (thisObject->storageHeader().totalLength > thisObject->m_table->length()) {
        visitor.appendValues(
            thisObject->overflowStorage(), thisObject->storageHeader().totalLength - thisObject->m_table->length());
    }
}

Structure* ScopedArguments::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ScopedArgumentsType, StructureFlags), info());
}

void ScopedArguments::overrideThings(VM& vm)
{
    RELEASE_ASSERT(!storageHeader().overrodeThings);
    
    putDirect(vm, vm.propertyNames->length, jsNumber(m_table->length()), static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirect(vm, vm.propertyNames->callee, m_callee.get(), static_cast<unsigned>(PropertyAttribute::DontEnum));
    putDirect(vm, vm.propertyNames->iteratorSymbol, globalObject(vm)->arrayProtoValuesFunction(), static_cast<unsigned>(PropertyAttribute::DontEnum));
    
    storageHeader().overrodeThings = true;
}

void ScopedArguments::overrideThingsIfNecessary(VM& vm)
{
    if (!storageHeader().overrodeThings)
        overrideThings(vm);
}

void ScopedArguments::unmapArgument(VM& vm, uint32_t i)
{
    ASSERT_WITH_SECURITY_IMPLICATION(i < storageHeader().totalLength);
    unsigned namedLength = m_table->length();
    if (i < namedLength)
        m_table.set(vm, this, m_table->set(vm, i, ScopeOffset()));
    else
        overflowStorage()[i - namedLength].clear();
}

void ScopedArguments::copyToArguments(JSGlobalObject* globalObject, CallFrame* callFrame, VirtualRegister firstElementDest, unsigned offset, unsigned length)
{
    GenericArguments::copyToArguments(globalObject, callFrame, firstElementDest, offset, length);
}

} // namespace JSC

