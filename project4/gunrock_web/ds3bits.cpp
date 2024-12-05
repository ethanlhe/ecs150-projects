#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

#include "Disk.h"
#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <diskImageFile>" << endl;
        return EXIT_FAILURE;
    }

    Disk *diskImage = new Disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem *fileSystem = new LocalFileSystem(diskImage);

    super_t superBlockDetails;
    fileSystem->readSuperBlock(&superBlockDetails);

    cout << "Super" << endl;
    cout << "inode_region_addr " << superBlockDetails.inode_region_addr << endl;
    cout << "inode_region_len " << superBlockDetails.inode_region_len << endl;
    cout << "num_inodes " << superBlockDetails.num_inodes << endl;
    cout << "data_region_addr " << superBlockDetails.data_region_addr << endl;
    cout << "data_region_len " << superBlockDetails.data_region_len << endl;
    cout << "num_data " << superBlockDetails.num_data << endl;
    cout << endl;

    int inodeBitmapSize = superBlockDetails.inode_bitmap_len * UFS_BLOCK_SIZE;
    unsigned char *inodeBitmapBuffer = new unsigned char[inodeBitmapSize];
    fileSystem->readInodeBitmap(&superBlockDetails, inodeBitmapBuffer);

    cout << "Inode bitmap" << endl;
    int inodeBitmapEntries = ((superBlockDetails.num_inodes - 1) / 8) + 1;
    for (int i = 0; i < inodeBitmapEntries; ++i) {
        cout << static_cast<unsigned int>(inodeBitmapBuffer[i]) << " ";
    }
    cout << endl
         << endl;

    int dataBitmapSize = superBlockDetails.data_bitmap_len * UFS_BLOCK_SIZE;
    unsigned char *dataBitmapBuffer = new unsigned char[dataBitmapSize];
    fileSystem->readDataBitmap(&superBlockDetails, dataBitmapBuffer);

    cout << "Data bitmap" << endl;
    int dataBitmapEntries = ((superBlockDetails.num_data - 1) / 8) + 1;
    for (int i = 0; i < dataBitmapEntries; ++i) {
        cout << static_cast<unsigned int>(dataBitmapBuffer[i]) << " ";
    }
    cout << endl;

    delete[] inodeBitmapBuffer;
    delete[] dataBitmapBuffer;
    delete fileSystem;
    delete diskImage;

    return EXIT_SUCCESS;
}
