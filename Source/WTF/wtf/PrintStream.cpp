/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2019,2022 Arm Ltd. All rights reserved.
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
#include <wtf/PrintStream.h>

#include <stdio.h>
#include <wtf/text/CString.h>
#include <wtf/text/UniquedStringImpl.h>
#include <wtf/text/WTFString.h>

#if __has_feature(capabilities)
#include <cheriintrin.h>
#endif

namespace WTF {

PrintStream::PrintStream() { }
PrintStream::~PrintStream() { } // Force the vtable to be in this module

void PrintStream::printf(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
}

void PrintStream::printfVariableFormat(const char* format, ...)
{
    ALLOW_NONLITERAL_FORMAT_BEGIN
    IGNORE_GCC_WARNINGS_BEGIN("suggest-attribute=format")
    va_list argList;
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
    IGNORE_GCC_WARNINGS_END
    ALLOW_NONLITERAL_FORMAT_END
}

void PrintStream::flush()
{
}

PrintStream& PrintStream::begin()
{
    return *this;
}

void PrintStream::end()
{
}

void printInternal(PrintStream& out, const char* string)
{
    out.printf("%s", string);
}

static void printExpectedCStringHelper(PrintStream& out, const char* type, Expected<CString, UTF8ConversionError> expectedCString)
{
    if (UNLIKELY(!expectedCString)) {
        if (expectedCString.error() == UTF8ConversionError::OutOfMemory)
            out.print("(Out of memory while converting ", type, " to utf8)");
        else
            out.print("(failed to convert ", type, " to utf8)");
        return;
    }
    out.print(expectedCString.value());
}

void printInternal(PrintStream& out, const StringView& string)
{
    printExpectedCStringHelper(out, "StringView", string.tryGetUtf8());
}

void printInternal(PrintStream& out, const CString& string)
{
    out.print(string.data());
}

void printInternal(PrintStream& out, const String& string)
{
    printExpectedCStringHelper(out, "String", string.tryGetUtf8());
}

void printInternal(PrintStream& out, const StringImpl* string)
{
    if (!string) {
        out.print("(null StringImpl*)");
        return;
    }
    printExpectedCStringHelper(out, "StringImpl*", string->tryGetUtf8());
}

void printInternal(PrintStream& out, bool value)
{
    out.print(boolForPrinting(value));
}

void printInternal(PrintStream& out, int value)
{
    out.printf("%d", value);
}

void printInternal(PrintStream& out, unsigned value)
{
    out.printf("%u", value);
}

void printInternal(PrintStream& out, signed char value)
{
    out.printf("%d", static_cast<int>(value));
}

void printInternal(PrintStream& out, unsigned char value)
{
    out.printf("%u", static_cast<unsigned>(value));
}

void printInternal(PrintStream& out, short value)
{
    out.printf("%d", static_cast<int>(value));
}

void printInternal(PrintStream& out, unsigned short value)
{
    out.printf("%u", static_cast<unsigned>(value));
}

void printInternal(PrintStream& out, long value)
{
    out.printf("%ld", value);
}

void printInternal(PrintStream& out, unsigned long value)
{
    out.printf("%lu", value);
}

void printInternal(PrintStream& out, long long value)
{
    out.printf("%lld", value);
}

void printInternal(PrintStream& out, unsigned long long value)
{
    out.printf("%llu", value);
}

void printInternal(PrintStream& out, float value)
{
    out.print(static_cast<double>(value));
}

void printInternal(PrintStream& out, double value)
{
    out.printf("%lf", value);
}

#if __has_feature(capabilities)
void printInternal(PrintStream& out, __uintcap_t value)
{
    uint64_t* value_u64_p = reinterpret_cast<uint64_t *>(&value);

#if CPU(LITTLE_ENDIAN)
    uint64_t high64 = value_u64_p[1];
    uint64_t low64 = value_u64_p[0];
#else
    uint64_t high64 = value_u64_p[0];
    uint64_t low64 = value_u64_p[1];
#endif

    ASSERT(low64 == cheri_address_get(value));

    out.printf("0x%c:%016lx:%016lx",
               cheri_tag_get(value) ? '1' : '0',
               high64,
               low64);
}

void printInternal(PrintStream& out, __intcap_t value)
{
    printInternal(out, (__uintcap_t) value);
}
#endif

void printInternal(PrintStream& out, RawPointer value)
{
#ifdef __CHERI_PURE_CAPABILITY__
    out.printf("%#lp", value.value());
#else
    out.printf("%p", value.value());
#endif
}

void dumpCharacter(PrintStream& out, char value)
{
    out.printf("%c", value);
}

} // namespace WTF

