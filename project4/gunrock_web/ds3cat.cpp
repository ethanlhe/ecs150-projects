#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "Disk.h"
#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
        return EXIT_FAILURE;
    }

    Disk *diskImage = new Disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem *fileSystem = new LocalFileSystem(diskImage);
    int targetInodeNumber = atoi(argv[2]);

    super_t superBlockDetails;
    fileSystem->readSuperBlock(&superBlockDetails);

    inode_t fileInode;
    int status = fileSystem->stat(targetInodeNumber, &fileInode);
    if (status < 0 || fileInode.type != UFS_REGULAR_FILE) {
        cerr << "Error reading file" << endl;
        delete fileSystem;
        delete diskImage;
        return EXIT_FAILURE;
    }

    // Dynamically allocate memory for file content
    char *fileContent = new char[fileInode.size];
    int bytesRead = fileSystem->read(targetInodeNumber, fileContent, fileInode.size);
    if (bytesRead < 0) {
        cerr << "Error reading file" << endl;
        delete[] fileContent;
        delete fileSystem;
        delete diskImage;
        return EXIT_FAILURE;
    }

    cout << "File blocks" << endl;
    for (int i = 0; i < ((fileInode.size - 1) / UFS_BLOCK_SIZE) + 1; ++i) {
        cout << fileInode.direct[i] << endl;
    }
    cout << endl;

    cout << "File data" << endl;
    for (int i = 0; i < fileInode.size; ++i) {
        cout << fileContent[i];
    }

    delete[] fileContent;
    delete fileSystem;
    delete diskImage;
    return EXIT_SUCCESS;
}
