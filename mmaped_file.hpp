#pragma once
#include <windows.h>
#include <filesystem>
#include <cassert>
#include <cstddef>

class mmaped_file {
public:
    mmaped_file(std::filesystem::path path) {
        hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        assert(hFile != INVALID_HANDLE_VALUE);
        hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        assert(hFile != INVALID_HANDLE_VALUE);
        mmaped_ptr = static_cast<byte*>(MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
        assert(mmaped_ptr != nullptr);
    }
    mmaped_file(const mmaped_file& file) = delete;
    mmaped_file(mmaped_file&& file) = delete;
    ~mmaped_file() {
        UnmapViewOfFile(mmaped_ptr);
        CloseHandle(hMapping);
        CloseHandle(hFile);
    }
    mmaped_file& operator=(const mmaped_file& file) = delete;
    mmaped_file& operator=(mmaped_file&& file) = delete;

    byte* data() const {
        return mmaped_ptr;
    }
    size_t size() const {
        return GetFileSize(hFile, NULL);
    }

private:
    HANDLE hFile;
    HANDLE hMapping;
    byte* mmaped_ptr;
};