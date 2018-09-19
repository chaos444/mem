/*
    Copyright 2018 Brick

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software
    and associated documentation files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge, publish, distribute,
    sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
    BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#if !defined(MEM_BRICK_H)
#define MEM_BRICK_H

#include "mem_defines.h"

#include <type_traits>

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <vector>
#include <string>

namespace mem
{
    class pointer;
    class region;
    class pattern;

#if defined(MEM_PLATFORM_WINDOWS)
    class protect;
    class module;
#endif // MEM_PLATFORM_WINDOWS

    class pointer
    {
    protected:
        uintptr_t value_ {0};

    public:
        MEM_CONSTEXPR pointer() noexcept = default;

        MEM_CONSTEXPR pointer(const std::nullptr_t value) noexcept;
        MEM_CONSTEXPR pointer(const uintptr_t value) noexcept;

        template <typename T>
        pointer(T* const value) noexcept;

        template <typename T, typename C>
        pointer(T C::* const value) noexcept;

        MEM_CONSTEXPR bool null() const noexcept;

        MEM_CONSTEXPR pointer add(const ptrdiff_t value) const noexcept;
        MEM_CONSTEXPR pointer sub(const ptrdiff_t value) const noexcept;

        MEM_CONSTEXPR pointer shift(const pointer& from, const pointer& to) const noexcept;

#if defined(MEM_ARCH_X64)
        pointer rip(const uint8_t offset) const noexcept;
#endif // MEM_ARCH_X64

        pointer& deref() const noexcept;

        MEM_CONSTEXPR pointer operator+(const ptrdiff_t value) const noexcept;
        MEM_CONSTEXPR pointer operator-(const ptrdiff_t value) const noexcept;

        MEM_CONSTEXPR ptrdiff_t operator-(const pointer& value) const noexcept;

        MEM_CONSTEXPR_14 pointer& operator+=(const ptrdiff_t value) noexcept;
        MEM_CONSTEXPR_14 pointer& operator-=(const ptrdiff_t value) noexcept;

        MEM_CONSTEXPR_14 pointer& operator++() noexcept;
        MEM_CONSTEXPR_14 pointer& operator--() noexcept;

        MEM_CONSTEXPR_14 pointer operator++(int) noexcept;
        MEM_CONSTEXPR_14 pointer operator--(int) noexcept;

        MEM_CONSTEXPR bool operator==(const pointer& value) const noexcept;
        MEM_CONSTEXPR bool operator!=(const pointer& value) const noexcept;

        MEM_CONSTEXPR bool operator<(const pointer& value) const noexcept;
        MEM_CONSTEXPR bool operator>(const pointer& value) const noexcept;

        MEM_CONSTEXPR bool operator<=(const pointer& value) const noexcept;
        MEM_CONSTEXPR bool operator>=(const pointer& value) const noexcept;

        template <typename T = pointer>
        typename std::add_lvalue_reference<T>::type at(const ptrdiff_t offset) const noexcept;

        template <typename T>
        typename std::enable_if<std::is_integral<T>::value, T>::type as() const noexcept;

        template <typename T>
        typename std::enable_if<std::is_pointer<T>::value, T>::type as() const noexcept;

        template <typename T>
        typename std::enable_if<std::is_member_pointer<T>::value, T>::type as() const noexcept;

        template <typename T>
        typename std::enable_if<std::is_lvalue_reference<T>::value, T>::type as() const noexcept;

        template <typename T>
        typename std::enable_if<std::is_array<T>::value, typename std::add_lvalue_reference<T>::type>::type as() const noexcept;
    };

    class region
    {
    public:
        pointer base {nullptr};
        size_t size {0};

        MEM_CONSTEXPR region() noexcept = default;

        MEM_CONSTEXPR region(const pointer& base, const size_t size) noexcept;

        MEM_CONSTEXPR bool contains(const region& value) const noexcept;

        MEM_CONSTEXPR bool contains(const pointer& value) const noexcept;
        MEM_CONSTEXPR bool contains(const pointer& value, const size_t size) const noexcept;

        template <typename T>
        MEM_CONSTEXPR bool contains(const pointer& value) const noexcept;

        MEM_CONSTEXPR bool operator==(const region& other) const noexcept;
        MEM_CONSTEXPR bool operator!=(const region& other) const noexcept;

        void copy(const pointer& source) const noexcept;
        void fill(const uint8_t value) const noexcept;

        MEM_CONSTEXPR region sub_region(const pointer& address) const noexcept;

#if defined(MEM_PLATFORM_WINDOWS)
        protect unprotect() const noexcept;
#endif // MEM_PLATFORM_WINDOWS

        pointer scan(const pattern& pattern) const noexcept;

        std::vector<pointer> scan_all(const pattern& pattern) const;

        bool is_ascii() const noexcept;
        bool is_utf8() const noexcept;

        std::string str() const;
        std::string hex(bool upper_case = true, bool padded = false) const;
    };

    /*
        PATTERN SCANNING
        ================

        +------+--------------------------------------------------+
        | Arch | Regs                                             |
        +------+-----+--------------------------------------------+
        |  x86 |   7 | eax, rbx, ecx, edx, esi, edi, ebp          |
        |  x64 |  15 | rax, rbx, rcx, rdx, rsi, rdi, rbp, r[8-15] |
        |  ARM | 13+ | r[0-12]                                    |
        +------+-----+--------------------------------------------+

        +---------------+-------+----------------------------------------------------------+
        | Type          | Reads |  Vars                                                    |
        +---------------+---+---+---+------------------------------------------------------+
        | Masks + Skips | 3 | 2 | 8 | current, end, i, last, bytes, masks, skips, skip_pos |
        | Masks         | 3 | 0 | 6 | current, end, i, last, bytes, masks                  |
        | Skips         | 2 | 2 | 6 | current, end, i, last, bytes,        skips           |
        |               | 2 | 0 | 5 | current, end, i, last, bytes                         |
        | Suffixes      | 2 | 3 | 7 | current, end, i, last, bytes,        skips, suffixes |
        +---------------+---+---+---+------------------------------------------------------+

        To avoid accessing the stack during the main scap loop, you would need at least 1 register per variable plus at least 1 or 2 temporary registers for operations.
        This is of course assuming the compiler is smart enough to use all of the registers efficiently. Lots of them aren't.
    */

    struct pattern_settings
    {
        // 0 = Disabled
        size_t min_bad_char_skip
        {
#if defined(MEM_ARCH_X86)
            10
#elif defined(MEM_ARCH_X64)
            4
#else
            8
#endif
        };

        size_t min_good_suffix_skip {min_bad_char_skip};

        char wildcard {'?'};
    };

    class pattern
    {
    protected:
        std::vector<uint8_t> bytes_ {};
        std::vector<uint8_t> masks_ {};

        // Boyer–Moore + Boyer–Moore–Horspool Implementation
        std::vector<size_t> bad_char_skips_ {};
        std::vector<size_t> good_suffix_skips_ {};

        size_t skip_pos_ {0};
        size_t original_size_ {0};

        size_t get_longest_run(size_t& length) const;

        bool is_prefix(const size_t pos) const;
        size_t get_suffix_length(const size_t pos) const;

        void finalize(const pattern_settings& settings);

    public:
        pattern() = default;

        pattern(const char* pattern, const pattern_settings& settings = {});
        pattern(const char* pattern, const char* mask, const pattern_settings& settings = {});
        pattern(const void* pattern, const void* mask, const size_t length, const pattern_settings& settings = {});

        template <typename UnaryPredicate>
        pointer scan_predicate(const region& region, UnaryPredicate pred) const noexcept(noexcept(pred(static_cast<const uint8_t*>(nullptr))));

        pointer scan(const region& region) const noexcept;

        bool match(const pointer& address) const noexcept;

        std::vector<pointer> scan_all(const region& region) const;

        const std::vector<uint8_t>& bytes() const noexcept;
        const std::vector<uint8_t>& masks() const noexcept;

        size_t size() const noexcept;
    };

#if defined(MEM_PLATFORM_WINDOWS)
    class protect
        : public region
    {
    protected:
        DWORD old_protect_;
        bool success_;

    public:
        protect(const region& region, DWORD new_protect);
        ~protect();

        protect(protect&&) noexcept;
        protect(const protect&) = delete;
    };

    class module
        : public region
    {
    protected:
        static module get_nt_module(const pointer& address);

    public:
        using region::region;

        static module named(const char* name);
        static module named(const wchar_t* name);

        static module main();
        static module self();
    };
#endif // MEM_PLATFORM_WINDOWS

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer::pointer(const std::nullptr_t) noexcept
        : value_(0)
    { }

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer::pointer(const uintptr_t value) noexcept
        : value_(value)
    { }

    template <typename T>
    MEM_STRONG_INLINE pointer::pointer(T* const value) noexcept
        : value_(reinterpret_cast<uintptr_t>(value))
    { }

    template <typename T, typename C>
    MEM_STRONG_INLINE pointer::pointer(T C::* const value) noexcept
        : value_(reinterpret_cast<const uintptr_t&>(value))
    {
        static_assert(sizeof(value) == sizeof(uintptr_t), "That's no pointer. It's a space station.");
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::null() const noexcept
    {
        return !value_;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer pointer::add(const ptrdiff_t value) const noexcept
    {
        return value_ + value;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer pointer::sub(const ptrdiff_t value) const noexcept
    {
        return value_ - value;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer pointer::shift(const pointer& from, const pointer& to) const noexcept
    {
        return (value_ - from.value_) + to.value_;
    }

#if defined(MEM_ARCH_X64)
    MEM_STRONG_INLINE pointer pointer::rip(const uint8_t offset) const noexcept
    {
        return value_ + offset + as<int32_t&>();
    }
#endif // MEM_ARCH_X64

    MEM_STRONG_INLINE pointer& pointer::deref() const noexcept
    {
        return as<pointer&>();
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer pointer::operator+(const ptrdiff_t value) const noexcept
    {
        return value_ + value;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE pointer pointer::operator-(const ptrdiff_t value) const noexcept
    {
        return value_ - value;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE ptrdiff_t pointer::operator-(const pointer& value) const noexcept
    {
        return static_cast<ptrdiff_t>(value_ - value.value_);
    }

    MEM_CONSTEXPR_14 MEM_STRONG_INLINE pointer& pointer::operator+=(const ptrdiff_t value) noexcept
    {
        value_ += value;

        return *this;
    }

    MEM_CONSTEXPR_14 MEM_STRONG_INLINE pointer& pointer::operator-=(const ptrdiff_t value) noexcept
    {
        value_ -= value;

        return *this;
    }

    MEM_CONSTEXPR_14 MEM_STRONG_INLINE pointer& pointer::operator++() noexcept
    {
        ++value_;

        return *this;
    }

    MEM_CONSTEXPR_14 MEM_STRONG_INLINE pointer& pointer::operator--() noexcept
    {
        --value_;

        return *this;
    }

    MEM_CONSTEXPR_14 MEM_STRONG_INLINE pointer pointer::operator++(int) noexcept
    {
        pointer result = *this;

        ++value_;

        return result;
    }

    MEM_CONSTEXPR_14 MEM_STRONG_INLINE pointer pointer::operator--(int) noexcept
    {
        pointer result = *this;

        --value_;

        return result;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::operator==(const pointer& value) const noexcept
    {
        return value_ == value.value_;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::operator!=(const pointer& value) const noexcept
    {
        return value_ != value.value_;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::operator<(const pointer& value) const noexcept
    {
        return value_ < value.value_;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::operator>(const pointer& value) const noexcept
    {
        return value_ > value.value_;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::operator<=(const pointer& value) const noexcept
    {
        return value_ <= value.value_;
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool pointer::operator>=(const pointer& value) const noexcept
    {
        return value_ >= value.value_;
    }

    template <typename T>
    MEM_STRONG_INLINE typename std::add_lvalue_reference<T>::type pointer::at(const ptrdiff_t offset) const noexcept
    {
        return add(offset).as<typename std::add_lvalue_reference<T>::type>();
    }

    template <typename T>
    MEM_STRONG_INLINE typename std::enable_if<std::is_integral<T>::value, T>::type pointer::as() const noexcept
    {
        static_assert(std::is_same<typename std::make_unsigned<T>::type, uintptr_t>::value, "Invalid Integer Type");

        return static_cast<T>(value_);
    }

    template <typename T>
    MEM_STRONG_INLINE typename std::enable_if<std::is_pointer<T>::value, T>::type pointer::as() const noexcept
    {
        return reinterpret_cast<T>(value_);
    }

    template <typename T>
    MEM_STRONG_INLINE typename std::enable_if<std::is_member_pointer<T>::value, T>::type pointer::as() const noexcept
    {
        static_assert(sizeof(T) == sizeof(value_), "That's no pointer. It's a space station.");

        return reinterpret_cast<const T&>(value_);
    }

    template <typename T>
    MEM_STRONG_INLINE typename std::enable_if<std::is_lvalue_reference<T>::value, T>::type pointer::as() const noexcept
    {
        return *reinterpret_cast<typename std::add_pointer<T>::type>(value_);
    }

    template <typename T>
    MEM_STRONG_INLINE typename std::enable_if<std::is_array<T>::value, typename std::add_lvalue_reference<T>::type>::type pointer::as() const noexcept
    {
        return *reinterpret_cast<typename std::add_pointer<T>::type>(value_);
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE region::region(const pointer& base_, const size_t size_) noexcept
        : base(base_)
        , size(size_)
    { }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool region::contains(const region& value) const noexcept
    {
        return (value.base >= base) && ((value.base + value.size) <= (base + size));
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool region::contains(const pointer& value) const noexcept
    {
        return (value >= base) && (value < (base + size));
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool region::contains(const pointer& value, const size_t length) const noexcept
    {
        return (value >= base) && ((value + length) <= (base + size));
    }

    template <typename T>
    MEM_CONSTEXPR MEM_STRONG_INLINE bool region::contains(const pointer& value) const noexcept
    {
        return (value >= base) && ((value + sizeof(T)) <= (base + size));
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool region::operator==(const region& other) const noexcept
    {
        return (base == other.base) && (size == other.size);
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE bool region::operator!=(const region& other) const noexcept
    {
        return (base != other.base) || (size != other.size);
    }

    MEM_STRONG_INLINE void region::copy(const pointer& source) const noexcept
    {
        std::memcpy(base.as<void*>(), source.as<const void*>(), size);
    }

    MEM_STRONG_INLINE void region::fill(const uint8_t value) const noexcept
    {
        std::memset(base.as<void*>(), value, size);
    }

    MEM_CONSTEXPR MEM_STRONG_INLINE region region::sub_region(const pointer& address) const noexcept
    {
        return region(address, size - (address - base));
    }

#if defined(MEM_PLATFORM_WINDOWS)
    MEM_STRONG_INLINE protect region::unprotect() const noexcept
    {
        return protect(*this, PAGE_EXECUTE_READWRITE);
    }
#endif // MEM_PLATFORM_WINDOWS

    inline pointer region::scan(const pattern& pattern) const noexcept
    {
        return pattern.scan(*this);
    }

    inline std::vector<pointer> region::scan_all(const pattern& pattern) const
    {
        return pattern.scan_all(*this);
    }

    namespace detail
    {
        static MEM_CONSTEXPR const int8_t utf8_length_table[256]
        {
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
            3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,
        };
    }

    inline bool region::is_ascii() const noexcept
    {
        for (size_t i = 0; i < size;)
        {
            const size_t length = detail::utf8_length_table[base.at<uint8_t>(i)];

            if (length != 1)
            {
                return false;
            }

            i += length;
        }

        return true;
    }

    inline bool region::is_utf8() const noexcept
    {
        for (size_t i = 0; i < size;)
        {
            const size_t length = detail::utf8_length_table[base.at<uint8_t>(i)];

            if (length == 0)
            {
                return false;
            }

            if ((i + length) > size)
            {
                return false;
            }

            for (size_t j = 1; j < length; ++j)
            {
                if ((base.at<uint8_t>(i + j) & 0xC0) != 0x80)
                {
                    return false;
                }
            }

            i += length;
        }

        return true;
    }

    inline std::string region::str() const
    {
        return std::string(base.as<const char*>(), size);
    }

    inline std::string region::hex(bool upper_case, bool padded) const
    {
        const char* const char_hex_table = upper_case ? "0123456789ABCDEF" : "0123456789abcdef";

        std::string result;

        result.reserve(size * (padded ? 3 : 2));

        for (size_t i = 0; i < size; ++i)
        {
            const uint8_t v = base.add(i).as<const uint8_t&>();

            if (i && padded)
            {
                result.push_back(' ');
            }

            result.push_back(char_hex_table[(v >> 4) & 0xF]);
            result.push_back(char_hex_table[(v >> 0) & 0xF]);
        }

        return result;
    }

    namespace detail
    {
        // '0'-'9' => 0x0-0x9
        // 'a'-'f' => 0xA-0xF
        // 'A'-'F' => 0xA-0xF
        static MEM_CONSTEXPR const int8_t hex_char_table[256]
        {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00 => 0x0F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x10 => 0x1F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20 => 0x2F
             0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1, // 0x30 => 0x3F
            -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40 => 0x4F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x50 => 0x5F
            -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60 => 0x6F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x70 => 0x7F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x80 => 0x8F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x90 => 0x9F
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xA0 => 0xAF
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xB0 => 0xBF
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0 => 0xCF
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xD0 => 0xDF
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0 => 0xEF
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xF0 => 0xFF
        };
    }

    inline pattern::pattern(const char* pattern, const pattern_settings& settings)
    {
        while (true)
        {
            uint8_t b = 0x00;
            uint8_t m = 0x00;

            char c = 0;
            size_t i = 0;

            while ((c = *pattern++) != '\0')
            {
                const int8_t value = detail::hex_char_table[static_cast<uint8_t>(c)];
                const bool is_wildcard = c == settings.wildcard;

                if (((value >= 0) && (value <= 0xF)) || is_wildcard)
                {
                    b <<= 4;
                    m <<= 4;

                    if (!is_wildcard)
                    {
                        b |= value;
                        m |= 0xF;
                    }

                    if (++i >= 2)
                    {
                        break;
                    }
                }
                else if (i)
                {
                    break;
                }
            }

            if (i)
            {
                if ((i == 1) && (m != 0))
                {
                    m |= 0xF0;
                }

                bytes_.push_back(b);
                masks_.push_back(m);
            }

            if (!c)
            {
                break;
            }
        }

        finalize(settings);
    }

    inline pattern::pattern(const char* pattern, const char* mask, const pattern_settings& settings)
    {
        if (mask)
        {
            const size_t size = strlen(mask);

            bytes_.resize(size);
            masks_.resize(size);

            for (size_t i = 0; i < size; ++i)
            {
                if (mask[i] == settings.wildcard)
                {
                    bytes_[i] = 0x00;
                    masks_[i] = 0x00;
                }
                else
                {
                    const char c = pattern[i];

                    bytes_[i] = static_cast<uint8_t>(c);
                    masks_[i] = 0xFF;
                }
            }
        }
        else
        {
            const size_t size = strlen(pattern);

            bytes_.resize(size);
            masks_.resize(size);

            for (size_t i = 0; i < size; ++i)
            {
                const char c = pattern[i];

                bytes_[i] = static_cast<uint8_t>(c);
                masks_[i] = 0xFF;
            }
        }

        finalize(settings);
    }

    inline pattern::pattern(const void* pattern, const void* mask, const size_t length, const pattern_settings& settings)
    {
        if (mask)
        {
            bytes_.resize(length);
            masks_.resize(length);

            for (size_t i = 0; i < length; ++i)
            {
                const uint8_t v = static_cast<const uint8_t*>(pattern)[i];
                const uint8_t m = static_cast<const uint8_t*>(mask)[i];

                bytes_[i] = v & m;
                masks_[i] = m;
            }
        }
        else
        {
            bytes_.resize(length);
            masks_.resize(length);

            for (size_t i = 0; i < length; ++i)
            {
                bytes_[i] = static_cast<const uint8_t*>(pattern)[i];
                masks_[i] = 0xFF;
            }
        }

        finalize(settings);
    }

    inline size_t pattern::get_longest_run(size_t& length) const
    {
        size_t max_skip = 0;
        size_t skip_pos = 0;

        {
            size_t current_skip = 0;

            for (size_t i = 0; i < bytes_.size(); ++i)
            {
                if (masks_[i] != 0xFF)
                {
                    if (current_skip > max_skip)
                    {
                        max_skip = current_skip;
                        skip_pos = i - current_skip;
                    }

                    current_skip = 0;
                }
                else
                {
                    ++current_skip;
                }
            }

            if (current_skip > max_skip)
            {
                max_skip = current_skip;
                skip_pos = bytes_.size() - current_skip;
            }
        }

        length = max_skip;

        return skip_pos;
    }

    inline bool pattern::is_prefix(const size_t pos) const
    {
        const size_t suffix_length = bytes_.size() - pos;

        for (size_t i = 0; i < suffix_length; ++i)
        {
            if (bytes_[i] != bytes_[pos + i])
            {
                return false;
            }
        }

        return true;
    }

    inline size_t pattern::get_suffix_length(const size_t pos) const
    {
        const size_t last = bytes_.size() - 1;

        size_t i = 0;

        while ((i < pos) && (bytes_[pos - i] == bytes_[last - i]))
        {
            ++i;
        }

        return i;
    }

    inline void pattern::finalize(const pattern_settings& settings)
    {
        if (bytes_.empty() || masks_.empty() || (bytes_.size() != masks_.size()))
        {
            bytes_.clear();
            masks_.clear();
            bad_char_skips_.clear();
            good_suffix_skips_.clear();

            skip_pos_ = 0;
            original_size_ = 0;

            return;
        }

        original_size_ = bytes_.size();

        size_t last_mask = masks_.size();

        while (last_mask && (masks_[last_mask - 1] == 0x00))
        {
            --last_mask;
        }

        bytes_.resize(last_mask);
        masks_.resize(last_mask);

        size_t max_skip = 0;
        size_t skip_pos = get_longest_run(max_skip);

        if ((settings.min_bad_char_skip > 0) && (max_skip > settings.min_bad_char_skip))
        {
            bad_char_skips_.resize(256, max_skip);
            skip_pos_ = skip_pos + max_skip - 1;

            for (size_t i = skip_pos, last = skip_pos + max_skip - 1; i < last; ++i)
            {
                bad_char_skips_[bytes_[i]] = last - i;
            }
        }

        if ((skip_pos == 0) && (max_skip == masks_.size()))
        {
            masks_.clear();

            if (!bad_char_skips_.empty() && (settings.min_good_suffix_skip > 0) && (max_skip > settings.min_good_suffix_skip))
            {
                good_suffix_skips_.resize(bytes_.size());

                const size_t last = bytes_.size() - 1;

                size_t last_prefix = last;

                for (size_t i = bytes_.size(); i--;)
                {
                    if (is_prefix(i + 1))
                    {
                        last_prefix = i + 1;
                    }

                    good_suffix_skips_[i] = last_prefix + (last - i);
                }

                for (size_t i = 0; i < last; ++i)
                {
                    size_t suffix_length = get_suffix_length(i);
                    size_t pos = last - suffix_length;

                    if (bytes_[i - suffix_length] != bytes_[pos])
                    {
                        good_suffix_skips_[pos] = suffix_length + (last - i);
                    }
                }
            }
        }
    }

    template <typename UnaryPredicate>
    inline pointer pattern::scan_predicate(const region& region, UnaryPredicate pred) const noexcept(noexcept(pred(static_cast<const uint8_t*>(nullptr))))
    {
        if (bytes_.empty())
        {
            return nullptr;
        }

        const size_t original_size = original_size_;
        const size_t region_size = region.size;

        if (original_size > region_size)
        {
            return nullptr;
        }

        const uint8_t* const region_base = region.base.as<const uint8_t*>();
        const uint8_t* const region_end = region_base + region_size;

        const uint8_t* current = region_base;
        const uint8_t* const end = region_end - original_size;

        const size_t last = bytes_.size() - 1;

        const uint8_t* const bytes = bytes_.data();
        const uint8_t* const masks = !masks_.empty() ? masks_.data() : nullptr;

        const size_t* const skips = !bad_char_skips_.empty() ? bad_char_skips_.data() : nullptr;

        if (masks)
        {
            if (skips)
            {
                const size_t skip_pos = skip_pos_;

                for (; MEM_LIKELY(current <= end); current += skips[current[skip_pos]])
                {
                    if (MEM_UNLIKELY((current[last] & masks[last]) == bytes[last]))
                    {
                        for (size_t i = last; MEM_LIKELY(i--);)
                        {
                            if (MEM_LIKELY((current[i] & masks[i]) != bytes[i]))
                            {
                                goto mask_skip_next;
                            }
                        }

                        if (pred(current))
                        {
                            return current;
                        }
                    }

                mask_skip_next:;
                }

                return nullptr;
            }
            else
            {
                for (; MEM_LIKELY(current <= end); ++current)
                {
                    if (MEM_UNLIKELY((current[last] & masks[last]) == bytes[last]))
                    {
                        for (size_t i = last; MEM_LIKELY(i--);)
                        {
                            if (MEM_LIKELY((current[i] & masks[i]) != bytes[i]))
                            {
                                goto mask_noskip_next;
                            }
                        }

                        if (pred(current))
                        {
                            return current;
                        }
                    }

                mask_noskip_next:;
                }

                return nullptr;
            }
        }
        else
        {
            const size_t* const suffixes = !good_suffix_skips_.empty() ? good_suffix_skips_.data() : nullptr;

            if (suffixes)
            {
                current += last;
                const uint8_t* const end_plus_last = end + last;

                for (; MEM_LIKELY(current <= end_plus_last);)
                {
                    size_t i = last;

                    do
                    {
                        if (MEM_LIKELY(*current != bytes[i]))
                        {
                            goto suffix_skip_next;
                        }
                        else if (MEM_LIKELY(i))
                        {
                            --current;
                            --i;
                        }
                        else
                        {
                            break;
                        }
                    } while (true);

                    if (pred(current))
                    {
                        return current;
                    }

                suffix_skip_next:
                    const size_t bc_skip = skips[*current];
                    const size_t gs_skip = suffixes[i];

                    current += (bc_skip > gs_skip) ? bc_skip : gs_skip;
                }

                return nullptr;
            }
            else if (skips)
            {
                for (; MEM_LIKELY(current <= end); current += skips[current[last]])
                {
                    if (MEM_UNLIKELY(current[last] == bytes[last]))
                    {
                        for (size_t i = last; MEM_LIKELY(i--);)
                        {
                            if (MEM_LIKELY(current[i] != bytes[i]))
                            {
                                goto nomask_skip_next;
                            }
                        }

                        if (pred(current))
                        {
                            return current;
                        }
                    }

                nomask_skip_next:;
                }

                return nullptr;
            }
            else
            {
                for (; MEM_LIKELY(current <= end); ++current)
                {
                    if (MEM_UNLIKELY(current[last] == bytes[last]))
                    {
                        for (size_t i = last; MEM_LIKELY(i--);)
                        {
                            if (MEM_LIKELY(current[i] != bytes[i]))
                            {
                                goto nomask_noskip_next;
                            }
                        }

                        if (pred(current))
                        {
                            return current;
                        }
                    }

                nomask_noskip_next:;
                }

                return nullptr;
            }
        }
    }

    namespace detail
    {
        struct always_true
        {
            template <typename... Args>
            MEM_CONSTEXPR MEM_STRONG_INLINE bool operator()(Args&&...) const noexcept
            {
                return true;
            }
        };
    }

    inline pointer pattern::scan(const region& region) const noexcept
    {
        return scan_predicate(region, detail::always_true {});
    }

    inline bool pattern::match(const pointer& address) const noexcept
    {
        if (bytes_.empty())
        {
            return false;
        }

        const uint8_t* current = address.as<const uint8_t*>();

        const size_t last = bytes_.size() - 1;

        const uint8_t* const bytes = bytes_.data();
        const uint8_t* const masks = !masks_.empty() ? masks_.data() : nullptr;

        if (masks)
        {
            size_t i = last;

            do
            {
                if (MEM_LIKELY((current[i] & masks[i]) != bytes[i]))
                {
                    return false;
                }
            } while (MEM_LIKELY(i--));

            return true;
        }
        else
        {
            size_t i = last;

            do
            {
                if (MEM_LIKELY(current[i] != bytes[i]))
                {
                    return false;
                }
            } while (MEM_LIKELY(i--));

            return true;
        }
    }

    inline std::vector<pointer> pattern::scan_all(const region& region) const
    {
        std::vector<pointer> results;

        scan_predicate(region, [&results] (const uint8_t* result)
        {
            results.emplace_back(result);

            return false;
        });

        return results;
    }

    MEM_STRONG_INLINE const std::vector<uint8_t>& pattern::bytes() const noexcept
    {
        return bytes_;
    }

    MEM_STRONG_INLINE const std::vector<uint8_t>& pattern::masks() const noexcept
    {
        return masks_;
    }

    MEM_STRONG_INLINE size_t pattern::size() const noexcept
    {
        return original_size_;
    }

#if defined(MEM_PLATFORM_WINDOWS)
    MEM_STRONG_INLINE protect::protect(const region& region, DWORD new_protect)
        : region(region)
        , old_protect_(0)
        , success_(false)
    {
        success_ = VirtualProtect(base.as<void*>(), size, new_protect, &old_protect_);
    }

    MEM_STRONG_INLINE protect::~protect()
    {
        if (success_)
        {
            bool flush = old_protect_ & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);

            VirtualProtect(base.as<void*>(), size, old_protect_, &old_protect_);

            if (flush)
            {
                FlushInstructionCache(GetCurrentProcess(), base.as<void*>(), size);
            }
        }
    }

    MEM_STRONG_INLINE protect::protect(protect&& rhs) noexcept
        : region(rhs)
        , old_protect_(rhs.old_protect_)
        , success_(rhs.success_)
    {
        rhs.old_protect_ = 0;
        rhs.success_ = false;
    }

    extern "C" namespace detail
    {
        IMAGE_DOS_HEADER __ImageBase;
    }

    MEM_STRONG_INLINE module module::get_nt_module(const pointer& address)
    {
        const IMAGE_DOS_HEADER* dos = address.as<const IMAGE_DOS_HEADER*>();
        const IMAGE_NT_HEADERS* nt = address.add(dos->e_lfanew).as<const IMAGE_NT_HEADERS*>();

        return module(address, nt->OptionalHeader.SizeOfImage);
    }

    MEM_STRONG_INLINE module module::named(const char* name)
    {
        return get_nt_module(GetModuleHandleA(name));
    }

    MEM_STRONG_INLINE module module::named(const wchar_t* name)
    {
        return get_nt_module(GetModuleHandleW(name));
    }

    MEM_STRONG_INLINE module module::main()
    {
        return module::named(static_cast<const char*>(nullptr));
    }

    MEM_STRONG_INLINE module module::self()
    {
        return get_nt_module(&detail::__ImageBase);
    }
#endif // MEM_PLATFORM_WINDOWS
}
#endif // !MEM_BRICK_H
