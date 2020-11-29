#pragma once

#include <vector>
#include <windows.h>
#include "scope_protect.h"

namespace util
{
    class code_patch
    {
        public:
            template <std::size_t size>
            code_patch(void* target, std::uint8_t(&&bytes) [size]):
                _target(target), _size(size)
            {
                // backup the original bytes
                _original.reserve(size);
                _patch.reserve(size);

                std::memcpy(_original.data(), target, size);
                std::memcpy(_patch.data(), bytes, size);
            }

            void enable() const
            {
                auto guard = scope_protect { _target, _size, PAGE_EXECUTE_READWRITE };
                std::memcpy(_target, _patch.data(), _size);
                FlushInstructionCache(GetCurrentProcess(), _target, _size);
            }

            void disable() const
            {
                auto guard = scope_protect { _target, _size, PAGE_EXECUTE_READWRITE };
                std::memcpy(_target, _original.data(), _size);
                FlushInstructionCache(GetCurrentProcess(), _target, _size);
            }
        private:
            void* _target = nullptr;
            std::size_t _size = 0;
            std::vector<std::byte> _original = {};
            std::vector<std::byte> _patch = {};
    };
}