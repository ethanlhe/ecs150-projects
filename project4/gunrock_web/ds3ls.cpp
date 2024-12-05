#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Disk.h"
#include "LocalFileSystem.h"
#include "StringUtils.h"
#include "ufs.h"

using namespace std;

bool compareName(const dir_ent_t &entry1, const dir_ent_t &entry2) {
    return strcmp(entry1.name, entry2.name) < 0;
}

vector<string> splitString(string &input, char delimiter) {
    vector<string> parts;
    string currentToken = "";

    for (size_t idx = 0; idx < input.size(); ++idx) {
        char &currentChar = input[idx];
        if (currentChar == delimiter && !currentToken.empty()) {
            parts.push_back(currentToken);
            currentToken = "";
        } else if (currentChar != delimiter) {
            currentToken += currentChar;
        }
    }

    if (!currentToken.empty()) {
        parts.push_back(currentToken);
    }

    return parts;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << argv[0] << ": diskImageFile directory" << endl;
        cerr << "For example:" << endl;
        cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
        return 1;
    }

    Disk *diskImage = new Disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem *fileSystem = new LocalFileSystem(diskImage);
    string path = string(argv[2]);

    super_t superBlock;
    fileSystem->readSuperBlock(&superBlock);

    vector<string> pathTokens = splitString(path, '/');
    int currentInode = UFS_ROOT_DIRECTORY_INODE_NUMBER;
    int tokenCount = pathTokens.size();
    inode_t currentInodeData;

    for (int idx = 0; idx < tokenCount; ++idx) {
        if (idx != tokenCount - 1) {
            fileSystem->stat(currentInode, &currentInodeData);

            if (currentInodeData.type != UFS_DIRECTORY) {
                cerr << "Directory not found" << endl;
                delete fileSystem;
                delete diskImage;
                return 1;
            }
        }

        int lookupResult = fileSystem->lookup(currentInode, pathTokens[idx].c_str());
        if (lookupResult < 0) {
            cerr << "Directory not found" << endl;
            delete fileSystem;
            delete diskImage;
            return 1;
        }

        if (fileSystem->stat(lookupResult, &currentInodeData) < 0) {
            cerr << "Directory not found" << endl;
            delete fileSystem;
            delete diskImage;
            return 1;
        }

        currentInode = lookupResult;
    }

    int statResult = fileSystem->stat(currentInode, &currentInodeData);
    if (statResult < 0) {
        delete fileSystem;
        delete diskImage;
        return 1;
    }

    if (currentInodeData.type == UFS_DIRECTORY) {
        char *dirBuffer = new char[currentInodeData.size];
        int bytesRead = fileSystem->read(currentInode, dirBuffer, currentInodeData.size);
        if (bytesRead < 0) {
            cerr << "Directory not found" << endl;
            delete[] dirBuffer;
            delete fileSystem;
            delete diskImage;
            return 1;
        }

        vector<dir_ent_t> dirEntries;
        for (int idx = 0; idx < currentInodeData.size / 32; ++idx) {
            dir_ent_t dirEntry;
            memcpy(dirEntry.name, dirBuffer + (idx * 32), DIR_ENT_NAME_SIZE);
            dirEntry.inum = *reinterpret_cast<int *>(dirBuffer + DIR_ENT_NAME_SIZE + (idx * 32));
            dirEntries.push_back(dirEntry);
        }

        sort(dirEntries.begin(), dirEntries.end(), compareName);

        for (size_t idx = 0; idx < dirEntries.size(); ++idx) {
            dir_ent_t &entry = dirEntries[idx];
            cout << entry.inum << "\t" << entry.name << endl;
        }

        delete[] dirBuffer;
    } else {
        cout << currentInode << "\t" << pathTokens.back() << endl;
    }

    delete fileSystem;
    delete diskImage;
    return 0;
}
