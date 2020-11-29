#pragma once

#include <windows.h>

namespace util
{
    class scope_protect
    {
        public:
            scope_protect(void* target, std::size_t size, DWORD new_rights):
                _target(target), _size(size), _new_rights(new_rights)
            {
                VirtualProtect(_target, _size, _new_rights, &_old_rights);
            }

            ~scope_protect()
            {
                VirtualProtect(_target, _size, _old_rights, &_old_rights);
            }
        private:
            void* _target = nullptr;
            std::size_t _size = 0;
            DWORD _old_rights = 0;
            DWORD _new_rights = 0;
    };
}