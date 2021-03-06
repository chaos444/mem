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

// cl.exe make_frequencies.cpp /O2 /std:c++17

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

inline bool ends_with(const std::string& needle, const std::string& haystack)
{
    return (needle.size() >= haystack.size()) && std::equal(haystack.rbegin(), haystack.rend(), needle.rbegin());
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return 1;
    }

    std::string exts[] = { "exe", "dll", "so" };

    size_t freqs[256] {0};
    size_t total = 0;

    std::vector<uint8_t> buffer(1024 * 1024 * 64);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(argv[1]))
    {
        if (!std::filesystem::is_regular_file(entry))
        {
            continue;
        }

        std::string path = entry.path().string();

        if (!std::any_of(std::begin(exts), std::end(exts), [&] (const std::string& ext) { return ends_with(path, ext); }))
        {
            continue;
        }

        std::ifstream file(path, std::ifstream::binary);

        while (file)
        {
            file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

            size_t count = file.gcount();

            for (size_t i = 0; i < count; ++i)
                ++freqs[buffer[i]];

            total += count;
        }
    }

    std::printf("Total: 0x%zX\n", total);

    size_t rel_freqs[256];

    std::iota(std::begin(rel_freqs), std::end(rel_freqs), 0);

    std::sort(std::begin(rel_freqs), std::end(rel_freqs), [&freqs] (size_t i1, size_t i2)
    {
        return freqs[i1] < freqs[i2];
    });

    for (size_t i = 0; i < 256; ++i)
    {
        freqs[rel_freqs[i]] = i;
    }

    for (size_t i = 0; i < 256; ++i)
    {
        if (i && !(i % 16))
        {
            std::printf("\n");
        }

        std::printf("0x%02zX,", freqs[i]);
    }
}
