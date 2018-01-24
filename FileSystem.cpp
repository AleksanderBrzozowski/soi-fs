//
// Created by abrzozowski on 21.01.18.
//

#include <stdexcept>
#include <fstream>
#include "FileSystem.h"
#include <boost/filesystem.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>


FileSystem::FileSystem(std::string name, uint32_t size, uint32_t blocksUsed) : name(std::move(name)),
                                                                               size(size),
                                                                               blocksUsed(blocksUsed) {
}

FileSystem::~FileSystem() {
    close();
}

std::unique_ptr<FileSystem> FileSystem::createFileSystem(uint32_t size, const std::string &name) {
    if (size <= FS_BLOCKS) {
        throw std::runtime_error("Provided filesystem space is too small");
    }

    char buffer[BLOCK_SIZE];
    std::fill_n(buffer, BLOCK_SIZE, '\0');

    std::fstream os;
    os.open(name, std::ios::out | std::ios::binary);

    for (uint32_t i = 0; i < size; ++i) {
        os.write(buffer, BLOCK_SIZE);
    }
    os.close();

    return std::unique_ptr<FileSystem>(new FileSystem(name, size, FS_BLOCKS));
}

std::unique_ptr<FileSystem> FileSystem::openFileSystem(std::string name) {
    const uint32_t size = static_cast<const uint32_t>(boost::filesystem::file_size(name) / BLOCK_SIZE);

    std::unique_ptr<FileSystem> fs = std::unique_ptr<FileSystem>(new FileSystem(name, size, FS_BLOCKS));

    std::ifstream is;
    is.open(name.c_str(), std::ios::in | std::ios::binary);

    char buffer[FS_BLOCKS][BLOCK_SIZE];

    for (uint8_t j = 0; j < FS_BLOCKS; j++) {
        is.read(buffer[j], BLOCK_SIZE);
    }

    for (uint8_t j = 0; j < FS_BLOCKS; j++) {
        for (uint16_t i = 0; i < BLOCK_SIZE; i += sizeof(node)) {
            auto nodeToAdd = reinterpret_cast<node *>(buffer[j] + i);
            if (nodeToAdd->isUsed) {
                fs->nodes.push_back(*nodeToAdd);
                fs->blocksUsed += nodeToAdd->blocks;
            }
        }
    }
    is.close();

    return fs;
}

void FileSystem::addFile(std::string fileName) {
    if (fileExists(fileName)) {
        throw std::runtime_error("File with this name already exists in file system.");
    }

    const uint32_t fileSize = static_cast<const uint32_t>(boost::filesystem::file_size(fileName));
    const uint32_t fileBlocks = ((fileSize - 1) / BLOCK_SIZE) + 1;

    const uint32_t freeBlocks = size - blocksUsed;
    if (fileBlocks > freeBlocks) {
        throw std::runtime_error("Cannot add file. There is not enough space in file system.");
    }

    unsigned position = alloc(fileBlocks);
    char buffers[fileBlocks][BLOCK_SIZE];
    std::ifstream is;
    is.open(fileName.c_str());
    for (unsigned i = 0; i < fileBlocks; ++i) {
        is.read(buffers[i], BLOCK_SIZE);
    }
    std::fstream os;
    os.open(name.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    os.seekp(position * BLOCK_SIZE);
    for (unsigned i = 0; i < fileBlocks; ++i) {
        os.write(buffers[i], BLOCK_SIZE);
    }

    node newNode{
            isUsed: 1,
            begin: position,
            size: fileSize,
            blocks: fileBlocks,
    };
    strcpy(newNode.name, fileName.c_str());
    nodes.push_back(newNode);

    blocksUsed += newNode.blocks;
}

bool FileSystem::fileExists(const std::string &fileName) {
    return std::find_if(
            nodes.begin(),
            nodes.end(),
            [&fileName](const node &node) { return std::strcmp(node.name, fileName.c_str()) == 0; })
           != nodes.end();
}

uint32_t FileSystem::alloc(uint32_t blocks) {
    while (true) {
        if (nodes.empty() || nodes[0].begin - FS_BLOCKS >= blocks) {
            return FS_BLOCKS;
        }

        for (auto i = 1; i < nodes.size(); ++i) {
            auto hole = nodes[i].begin - nodes[i - 1].end();
            if (hole >= blocks) {
                return nodes[i - 1].end();
            }
        }

        if (size - nodes[nodes.size() - 1].end() >= blocks) {
            return nodes[nodes.size() - 1].end();
        }

        defragment();
    }
}

void FileSystem::defragment() {
    auto index = 0;
    uint32_t pos = FS_BLOCKS;

    if (nodes[0].begin == FS_BLOCKS) {
        for (index = 1; index < nodes.size(); ++index) {
            if (nodes[index - 1].end() < nodes[index].begin) {
                pos = nodes[index - 1].end();
                break;
            }
        }
    }

    unsigned blocks = nodes[index].blocks;
    unsigned oldPos = nodes[index].begin;
    nodes[index].begin = pos;

    char buffers[blocks][BLOCK_SIZE];
    std::ifstream is;
    is.open(name.c_str());
    is.seekg(oldPos * BLOCK_SIZE);
    for (unsigned i = 0; i < blocks; ++i) {
        is.read(buffers[i], BLOCK_SIZE);
    }
    std::fstream os;
    os.open(name.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    os.seekp(pos * BLOCK_SIZE);
    for (unsigned i = 0; i < blocks; ++i) {
        os.write(buffers[i], BLOCK_SIZE);
    }
}

void FileSystem::fileMap() {
    std::string tab[size];
    for (unsigned i = 0; i < FS_BLOCKS; ++i) {
        tab[i] = "[FS]";
    }
    for (unsigned i = FS_BLOCKS; i < size; ++i) {
        tab[i] = "[+]";
    }

    for (auto &node : nodes) {
        for (unsigned i = node.begin; i < node.end(); ++i) {
            std::stringstream ss;
            ss << "[" << node.name << "]";
            tab[i] = ss.str();
        }
    }

    for (int j = 0; j < size; ++j) {
        std::cout << tab[j];
    }
    std::cout << std::endl;
}


void FileSystem::close() {
    std::sort(nodes.begin(), nodes.end(), [this](const node &a, const node &b) { return a.begin < b.begin; });

    char buffers[FS_BLOCKS][BLOCK_SIZE] = {0};

    auto idx = 0;
    for (auto &buffer : buffers) {
        for (auto i = 0; i < BLOCK_SIZE; i += sizeof(node), idx++) {
            auto *inode = reinterpret_cast<node *>(buffer + i);
            if (idx < nodes.size()) {
                *inode = nodes[idx];
            }
        }
    }

    std::fstream os;
    os.open(name.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    os.seekp(std::ios_base::beg);
    for (auto &buffer : buffers) {
        os.write(buffer, BLOCK_SIZE);
    }
}

void FileSystem::listFiles() {
    std::cout << std::setw(10) << "start"
              << std::setw(20) << "długość"
              << std::setw(10) << "rozmiar"
              << std::setw(48) << "nazwa"
              << std::endl;
    for (const auto &node: nodes) {
        std::cout << std::setw(10) << node.begin
                  << std::setw(20) << node.blocks
                  << std::setw(10) << node.size
                  << std::setw(48) << node.name
                  << std::endl;
    }
}

void FileSystem::remove(const std::string &fileName) {
    auto it = std::find_if(
            nodes.begin(),
            nodes.end(),
            [&fileName](const node &node) { return std::strcmp(node.name, fileName.c_str()) == 0; }
    );
    if (it == nodes.end()) {
        throw std::runtime_error("File with this name does not exist in file system");
    }

    blocksUsed -= (*it).size;
    nodes.erase(it);
}

void FileSystem::rename(const std::string &from, const std::string &to) {
    auto it = std::find_if(
            nodes.begin(),
            nodes.end(),
            [&from](const node &node) { return std::strcmp(node.name, from.c_str()) == 0; }
    );
    if (it == nodes.end()) {
        throw std::runtime_error("File with this name does not exist in file system");
    }

    strcpy((*it).name, to.c_str());
}

void FileSystem::downloadFile(const std::string &fileName, const std::string &dest) {
    auto it = std::find_if(
            nodes.begin(),
            nodes.end(),
            [&fileName](const node &node) { return std::strcmp(node.name, fileName.c_str()) == 0; }
    );
    if (it == nodes.end()) {
        throw std::runtime_error("File with this name does not exist in file system");
    }

    std::ofstream os;
    os.open(dest.c_str());

    std::ifstream is;
    is.open(name.c_str());
    is.seekg((*it).begin * BLOCK_SIZE);

    char buffer[BLOCK_SIZE];
    auto blocksToCopy = BLOCK_SIZE;
    for (auto blocksLeft = (*it).size; blocksLeft > 0;) {
        if (blocksLeft < BLOCK_SIZE) {
            blocksToCopy = static_cast<uint16_t>(blocksLeft);
        }
        is.read(buffer, blocksToCopy);
        os.write(buffer, blocksToCopy);
        blocksLeft -= blocksToCopy;
    }

    os.close();
    is.close();
}
