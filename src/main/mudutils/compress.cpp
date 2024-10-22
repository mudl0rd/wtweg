#include "utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <vector>
#include <array>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
using namespace std;

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.h"

struct head
{
    uint32_t magic;
    uint32_t unpacked;
    uint32_t packed;
};

namespace MudUtil
{

    std::vector<unsigned char> compress_buf(unsigned char *buf, size_t size)
    {
        head header = {0x12111988, 0, 0};
        std::vector<unsigned char> out_buf;
        std::vector<unsigned char> out = compress_deflate(buf, size);
        header.packed = out.size();
        header.unpacked = size;
        uint8_t *bytes = (uint8_t *)(&header);
        vector_appendbytes(out_buf, bytes, sizeof(head));
        vector_appendbytes(out_buf, out.data(), out.size());
        return out_buf;
    }

    std::vector<unsigned char> decompress_buf(
        uint8_t *buf,
        size_t size)
    {
        std::vector<unsigned char> out;
        struct head *header = (struct head *)buf;
        if (header->magic != 0x12111988)
            return {};
        out = decompress_deflate(buf + sizeof(header), header->packed, header->unpacked);
        return out;
    }

    std::vector<unsigned char> compress_deflate(unsigned char *buf, size_t size)
    {
        std::vector<unsigned char> out;
        mz_ulong out_len = mz_compressBound(size);
        out.resize(out_len);
        mz_compress(out.data(), &out_len, buf, size);
        out.resize(out_len);
        return out;
    }

    std::vector<unsigned char> decompress_deflate(
        uint8_t *buf,
        size_t size, size_t uncomp_sz)
    {
        std::vector<unsigned char> out;
        out.resize(uncomp_sz);
        mz_ulong us = uncomp_sz;
        mz_uncompress(out.data(), &us, buf, size);
        return out;
    }
}