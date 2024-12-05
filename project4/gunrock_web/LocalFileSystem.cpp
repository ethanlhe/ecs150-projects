#include "LocalFileSystem.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "ufs.h"

using namespace std;

LocalFileSystem::LocalFileSystem(Disk *disk) {
    this->disk = disk;
}

void setBit(unsigned char *bitmapArray, int position) {
    int byteIndex = position / 8;
    int bitIndex = position % 8;
    bitmapArray[byteIndex] |= (1 << bitIndex);
}

bool isValidBit(unsigned char *bitmapArray, int position) {
    int byteIndex = position / 8;
    int bitIndex = position % 8;
    return (bitmapArray[byteIndex] >> bitIndex) & 1;
}

void LocalFileSystem::readSuperBlock(super_t *superBlock) {
    char rawData[UFS_BLOCK_SIZE];
    disk->readBlock(0, rawData);

    int *superFields = reinterpret_cast<int *>(rawData);
    superBlock->inode_bitmap_addr = superFields[0];
    superBlock->inode_bitmap_len = superFields[1];
    superBlock->data_bitmap_addr = superFields[2];
    superBlock->data_bitmap_len = superFields[3];
    superBlock->inode_region_addr = superFields[4];
    superBlock->inode_region_len = superFields[5];
    superBlock->data_region_addr = superFields[6];
    superBlock->data_region_len = superFields[7];
    superBlock->num_inodes = superFields[8];
    superBlock->num_data = superFields[9];
}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    for (int i = 0; i < super->inode_bitmap_len; i++) {
        int blockOffset = i + super->inode_bitmap_addr;

        char buffer[UFS_BLOCK_SIZE];
        disk->readBlock(blockOffset, buffer);
        memcpy(inodeBitmap + (i * UFS_BLOCK_SIZE), buffer, UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    for (int i = 0; i < super->inode_bitmap_len; i++) {
        int blockOffset = i + super->inode_bitmap_addr;

        char buffer[UFS_BLOCK_SIZE];
        memcpy(buffer, inodeBitmap + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
        disk->writeBlock(blockOffset, buffer);
    }
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
    for (int i = 0; i < super->data_bitmap_len; i++) {
        int blockOffset = i + super->data_bitmap_addr;

        char buffer[UFS_BLOCK_SIZE];
        disk->readBlock(blockOffset, buffer);
        memcpy(dataBitmap + (i * UFS_BLOCK_SIZE), buffer, UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
    for (int i = 0; i < super->data_bitmap_len; i++) {
        int blockOffset = i + super->data_bitmap_addr;

        char buffer[UFS_BLOCK_SIZE];
        memcpy(buffer, dataBitmap + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
        disk->writeBlock(blockOffset, buffer);
    }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
    int inodeIndex = 0;
    for (int i = 0; i < super->inode_region_len; i++) {
        int blockOffset = i + super->inode_region_addr;

        char buffer[UFS_BLOCK_SIZE];
        disk->readBlock(blockOffset, buffer);

        for (int j = 0; j < UFS_BLOCK_SIZE / 128; j++) {
            inode_t inode;
            memcpy(&inode.type, buffer + (j * 128), sizeof(int));
            memcpy(&inode.size, buffer + (j * 128 + sizeof(int)), sizeof(int));
            memcpy(inode.direct, buffer + (j * 128 + 2 * sizeof(int)), sizeof(inode.direct));

            inodes[inodeIndex++] = inode;
        }
    }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
    int totalInodes = (super->inode_region_len * UFS_BLOCK_SIZE) / 128;
    char buffer[UFS_BLOCK_SIZE];
    int offset = 0;
    int blockIndex = super->inode_region_addr;

    for (int i = 0; i < totalInodes; i++) {
        memcpy(buffer + offset, &inodes[i].type, sizeof(int));
        memcpy(buffer + offset + sizeof(int), &inodes[i].size, sizeof(int));
        memcpy(buffer + offset + 2 * sizeof(int), inodes[i].direct, sizeof(inodes[i].direct));
        offset += 128;

        if (offset == UFS_BLOCK_SIZE) {
            disk->writeBlock(blockIndex++, buffer);
            offset = 0;
        }
    }

    if (offset > 0) {
        disk->writeBlock(blockIndex, buffer);
    }
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
    super_t super;
    readSuperBlock(&super);

    if (parentInodeNumber >= super.num_inodes) return -EINVALIDINODE;

    inode_t parentInode;
    if (stat(parentInodeNumber, &parentInode) < 0 || parentInode.type != UFS_DIRECTORY) {
        return -EINVALIDINODE;
    }

    char dirData[parentInode.size];
    if (read(parentInodeNumber, dirData, parentInode.size) < 0) {
        return -EINVALIDINODE;
    }

    for (int i = 0; i < parentInode.size / (int)sizeof(dir_ent_t); i++) {
        char entryName[DIR_ENT_NAME_SIZE];
        memcpy(entryName, dirData + (i * sizeof(dir_ent_t)), DIR_ENT_NAME_SIZE);

        if (entryName == name) {
            int inodeNumber = *reinterpret_cast<int *>(dirData + DIR_ENT_NAME_SIZE + (i * sizeof(dir_ent_t)));
            return inodeNumber;
        }
    }

    return -ENOTFOUND;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
    super_t super;
    readSuperBlock(&super);

    if (inodeNumber >= super.num_inodes) return -EINVALIDINODE;

    unsigned char *inodeBitmap = new unsigned char[super.inode_bitmap_len * UFS_BLOCK_SIZE];
    readInodeBitmap(&super, inodeBitmap);

    if (!isValidBit(inodeBitmap, inodeNumber)) {
        delete[] inodeBitmap;
        return -EINVALIDINODE;
    }

    delete[] inodeBitmap;

    inode_t *inodeTable = new inode_t[super.inode_region_len * UFS_BLOCK_SIZE / sizeof(inode_t)];
    readInodeRegion(&super, inodeTable);
    *inode = inodeTable[inodeNumber];
    delete[] inodeTable;

    return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
    if (size > MAX_FILE_SIZE) return -EINVALIDSIZE;

    super_t super;
    readSuperBlock(&super);

    inode_t inode;
    if (stat(inodeNumber, &inode) < 0) return -EINVALIDINODE;

    size = min(size, inode.size);

    for (int i = 0; i < (size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE; i++) {
        char tempBuffer[UFS_BLOCK_SIZE];
        disk->readBlock(inode.direct[i], tempBuffer);

        int copyLength = (i < size / UFS_BLOCK_SIZE) ? UFS_BLOCK_SIZE : size % UFS_BLOCK_SIZE;
        memcpy(static_cast<char *>(buffer) + i * UFS_BLOCK_SIZE, tempBuffer, copyLength);
    }

    return size;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
    return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
    return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
    return 0;
}
