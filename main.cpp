#include <iostream>
#include "FileSystem.h"

void first();

void second();

void third();

void fourth();

void five();

int main() {
    return 0;
}

void five() {
    {
        auto fs = FileSystem::createFileSystem(11, "olek");
        fs->addFile("1.txt");
        fs->addFile("2.txt");
    }

    auto fs = FileSystem::openFileSystem("olek");
    fs->listFiles();
}

void fourth() {
    auto fs = FileSystem::createFileSystem(11, "olek");
    fs->addFile("1.txt");
    fs->addFile("2.txt");

    fs->remove("1.txt");
    fs->addFile("1.txt");

    fs->remove("2.txt");
    fs->addFile("2.txt");

    fs->fileMap();
    fs->listFiles();
}

void third() {
    auto fs = FileSystem::createFileSystem(11, "olek");
    fs->addFile("1.txt");
    fs->addFile("2.txt");
    fs->downloadFile("1.txt", "/home/abrzozowski/CLionProjects/soi-vfs/downloaded");
}

void second() {
    auto fs = FileSystem::createFileSystem(9, "olek");
    fs->addFile("1.txt");
}

void first() {
    auto fs = FileSystem::createFileSystem(32, "olek");
    fs->addFile("1.txt");
    fs->addFile("2.txt");
    fs->addFile("4.txt");

    fs->rename("4.txt", "10.txt");
    fs->addFile("4.txt");

    fs->addFile("5.txt");

    fs->listFiles();
}