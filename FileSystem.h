//
// Created by abrzozowski on 21.01.18.
//

#ifndef SOI_VFS_FILESYSTEM_H
#define SOI_VFS_FILESYSTEM_H


#include <cstdint>
#include <string>
#include <vector>
#include <bits/unique_ptr.h>


class FileSystem {
    FileSystem(std::string name, uint32_t size, uint32_t blocksUsed);

    static const uint16_t BLOCK_SIZE = 1024; // 1kB
    static const uint8_t FS_BLOCKS = 8;

    struct node {
        uint32_t isUsed;
        uint32_t begin;
        uint32_t size;
        uint32_t blocks;
        char name[48];

        uint32_t end() { return begin + blocks; }
    }; // sizeof(node) = 64

private:
    void defragment();

    uint32_t alloc(uint32_t);

    bool fileExists(const std::string &);

    void close();

    std::vector<node> nodes;
    std::string name;
    uint32_t size;
    uint32_t blocksUsed;


public:
    static std::unique_ptr<FileSystem> openFileSystem(std::string);

    static std::unique_ptr<FileSystem> createFileSystem(uint32_t size, const std::string &);

    ~FileSystem();

    void addFile(std::string);

    void fileMap();

    void listFiles();

    void remove(const std::string &);

    void rename(const std::string &, const std::string &);

    void downloadFile(const std::string &, const std::string &);
};


#endif //SOI_VFS_FILESYSTEM_H
