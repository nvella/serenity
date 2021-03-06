/*
 * Copyright (c) 2020, Itamar S. <itamar8910@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CompilationUnit.h"
#include "DwarfTypes.h"
#include <AK/ByteBuffer.h>
#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>
#include <AK/String.h>
#include <LibELF/Image.h>

namespace Debug::Dwarf {

class DwarfInfo {
public:
    explicit DwarfInfo(const ELF::Image&);

    ReadonlyBytes debug_info_data() const { return m_debug_info_data; }
    ReadonlyBytes abbreviation_data() const { return m_abbreviation_data; }
    ReadonlyBytes debug_strings_data() const { return m_debug_strings_data; }

    template<typename Callback>
    void for_each_compilation_unit(Callback) const;

private:
    void populate_compilation_units();

    ReadonlyBytes section_data(const String& section_name) const;

    const ELF::Image& m_elf;
    ReadonlyBytes m_debug_info_data;
    ReadonlyBytes m_abbreviation_data;
    ReadonlyBytes m_debug_strings_data;

    Vector<Dwarf::CompilationUnit> m_compilation_units;
};

template<typename Callback>
void DwarfInfo::for_each_compilation_unit(Callback callback) const
{
    for (const auto& unit : m_compilation_units) {
        callback(unit);
    }
}

}
