#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

#define MAGIC_NUMBER 2137
#define FILE_NAME_SIZE 512
#define BLOCK_SIZE 1024
#define MIN_FILE_SYSTEM_SIZE 1048576
#define I_NODES_AMOUNT_DIVIDER 4096 / 2
#define MAP_NEW_LINE 80

struct INode {
    char fileName[FILE_NAME_SIZE] = {};
    size_t fileSize = 0;
    size_t firstBlock = 0;
};

struct DataBlock {
    char data[BLOCK_SIZE] = {};
    size_t nextBlock = 0;
};

struct SuperBlock {
    size_t magicNumber;
    size_t fileSystemSize;
    size_t iNodeAmount;
    size_t blockAmount;
    size_t iNodeStart;
    size_t blockStart;
};

class VirtualFileSystem {
    private:
        SuperBlock superBlock;
        std::vector<INode> iNodes;
        std::vector<DataBlock> dataBlocks;
        std::fstream discFile;

        void initializeSuperBlock(size_t systemSize) {
            superBlock.magicNumber = MAGIC_NUMBER;
            superBlock.fileSystemSize = systemSize;
            superBlock.iNodeAmount = superBlock.fileSystemSize / I_NODES_AMOUNT_DIVIDER;
            superBlock.blockAmount = (superBlock.fileSystemSize - sizeof(SuperBlock) - superBlock.iNodeAmount * sizeof(INode)) / sizeof(DataBlock);
            superBlock.iNodeStart = sizeof(SuperBlock);
            superBlock.blockStart = superBlock.iNodeStart + superBlock.iNodeAmount * sizeof(INode);
        }

        void writeSuperBlock() {
            discFile.seekp(0, std::ios::beg);
            discFile.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
        }

        void writeINode(size_t index, INode& iNode) {
            size_t offset = superBlock.iNodeStart + index * sizeof(INode);
            discFile.seekp(offset, std::ios::beg);
            discFile.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
            iNodes.push_back(iNode);
        }

        void writeIdNodes() {
            for (size_t i = 0; i < superBlock.iNodeAmount; ++i) {
                INode iNode;
                writeINode(i, iNode);
            }
        }

        void writeDataBlock(size_t index, DataBlock& dataBlock) {
            size_t offset = superBlock.blockStart + index * sizeof(DataBlock);
            discFile.seekp(offset, std::ios::beg);
            discFile.write(reinterpret_cast<char*>(&dataBlock), sizeof(DataBlock));
            dataBlocks.push_back(dataBlock);
        }

        void writeDataBlocks() {
            for (size_t i = 0; i < superBlock.blockAmount; ++i) {
                DataBlock dataBlock;
                writeDataBlock(i, dataBlock);
            }
        }

        void loadSuperBlock() {
            discFile.seekg(0, std::ios::beg);
            discFile.read(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));

            if (superBlock.magicNumber != MAGIC_NUMBER) {
            throw std::runtime_error("INVALID MAGIC NUMBER, SYSTEM CORRUPTED");
            }
        }

        void loadINodes() {
            for (size_t i = 0; i < superBlock.iNodeAmount; ++i) {
                INode iNode;
                size_t offset = superBlock.iNodeStart + i * sizeof(INode);
                discFile.seekg(offset, std::ios::beg);
                discFile.read(reinterpret_cast<char*>(&iNode), sizeof(INode));
                iNodes.push_back(iNode);
            }
        }

        void loadDataBlock() {
            for (size_t i = 0; i < superBlock.blockAmount; ++i) {
                DataBlock dataBlock;
                size_t offset = superBlock.blockStart + i * sizeof(DataBlock);
                discFile.seekg(offset, std::ios::beg);
                discFile.read(reinterpret_cast<char*>(&dataBlock), sizeof(DataBlock));
                dataBlocks.push_back(dataBlock);
            }
        }

        void loadSystem(const std::string& name) {

            if (!fileExists(name)) {
                throw std::runtime_error("SYSTEM " + name + " NOT FOUND");
            }

            discFile.open(name, std::ios::in | std::ios::out | std::ios::binary);
            loadSuperBlock();
            loadINodes();
            loadDataBlock();
            discFile.close();
        }

        bool fileExists(const std::string& name) {
            std::ifstream file(name);
            return file.good();
        }

        std::string extractFileName(const std::string& filePath) {
            return std::filesystem::path(filePath).filename().string();
        }

        bool isINodeFree(int index) {
            return iNodes[index].fileSize == 0 && iNodes[index].firstBlock == 0;
        }

        bool isDataBlockFree(int index) {
            return std::all_of(std::begin(dataBlocks[index].data), std::end(dataBlocks[index].data), [](char c) { return c == '\0'; });
        }

        int getFirstFreeINodeIndex() {
            for (int i = 0; i < iNodes.size(); i++) {
                if (isINodeFree(i)) {
                    return i;
                }
            }
            return -1;
        }

        int getFirstFreeDataBlockIndex() {
            for (int i = 0; i < dataBlocks.size(); i++) {
                if (isDataBlockFree(i)) {
                    return i;
                }
            }
            
            return -1;
        }

        int getNextFreeDataBlockIndex(int index) {
            for (int i = index + 1; i < dataBlocks.size(); i++) {
                if (isDataBlockFree(i)) {
                    return i;
                }
            }
            return -1;
        }

        int getAmountOfFreeDataBlocks() {
            int freeDataBlocks = 0;
            for (int i = 0; i < dataBlocks.size(); i++) {
                if (isDataBlockFree(i)) {
                    freeDataBlocks++;
                }
            }
            return freeDataBlocks;
        }

        int getINodeIndex(const std::string& fileName) {
            for (int i = 0; i < iNodes.size(); i++) {
                if (std::strcmp(iNodes[i].fileName, fileName.c_str()) == 0) {
                    return i;
                }
            }
            
            return -1;
        }

        int calculateDataBlockIndexFromOffset(size_t blockOffset) {
            return (blockOffset - superBlock.blockStart) / sizeof(DataBlock);
        }

        size_t calculateDataBlockOffsetFromIndex(int blockIndex) {
            return superBlock.blockStart + blockIndex * sizeof(DataBlock);
        }

    public:
        VirtualFileSystem() = default;

        void createSystem(size_t size, const std::string& name) {

            if (fileExists(name)) {
                std::cout << "SYSTEM " << name << " ALREADY EXISTS" << std::endl;
                return;
            }

            if (size < MIN_FILE_SYSTEM_SIZE) {
                std::cout << "SYSTEM " << name << " CANNOT BE CREATED" << std::endl;
                std::cout << "FILE SYSTEM SIZE TOO SMALL" << std::endl;
                return;
            }

            discFile.open(name, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
            initializeSuperBlock(size);
            writeSuperBlock();
            writeIdNodes();
            writeDataBlocks();
            discFile.close();

            std::cout << "SYSTEM " << name << " HAS BEEN CREATED" << std::endl;
        }

        void deleteFile(const std::string& systemName, const std::string& fileName) {
            loadSystem(systemName);
            
            // Getting the index of the file from INode in memory
            int fileIndexInteger = getINodeIndex(fileName);

            // Checking if the file exists on the system
            if (fileIndexInteger == -1) {
                std::cout << "FILE " << fileName << " NOT FOUND" << std::endl;
                return;
            }

            size_t fileIndex = size_t(fileIndexInteger);

            // Getting the first and current block offset from INode in memory
            size_t currentBlockOffset = iNodes[fileIndex].firstBlock;
            discFile.open(systemName, std::ios::in | std::ios::out | std::ios::binary);
            while (currentBlockOffset != 0) {
                // Calculating the index of the current block in memory
                int currentBlockIndex = calculateDataBlockIndexFromOffset(currentBlockOffset);
                // Getting the next block offset from the current block in memory
                size_t nextBlockOffset = dataBlocks[currentBlockIndex].nextBlock;
                // Clearning data block in memory and file
                dataBlocks[currentBlockIndex] = DataBlock();
                discFile.seekp(currentBlockOffset, std::ios::beg);
                discFile.write(reinterpret_cast<char*>(&dataBlocks[currentBlockIndex]), sizeof(DataBlock));
                currentBlockOffset = nextBlockOffset;
            }

            // Clearing INode in memory and file
            iNodes[fileIndex] = INode();
            discFile.seekp(superBlock.iNodeStart + fileIndex * sizeof(INode), std::ios::beg);
            discFile.write(reinterpret_cast<char*>(&iNodes[fileIndex]), sizeof(INode));
            discFile.close();

            std::cout << "FILE " << fileName << " HAS BEEN DELETED" << std::endl;
        }

        void copyFileToSystem(const std::string& systemName, std::string name) {
            // Checking if the file exists outside the system
            if (!fileExists(name)) {
                std::cout << "CANNOT COPY FILE " << name << " TO SYSTEM " << systemName << std::endl;
                std::cout << "FILE " << name << " NOT FOUND" << std::endl;
                return;
            }
            
            loadSystem(systemName);

            // Checking if the file already exists in the system
            if (getINodeIndex(extractFileName(name)) != -1) {
                std::cout << "CANNOT COPY FILE " << extractFileName(name) << " TO SYSTEM " << systemName << std::endl;
                std::cout << "FILE " << extractFileName(name) << " ALREADY EXISTS" << std::endl;
                return;
            }
            
            // Getting the size of the file
            std::ifstream file(name, std::ios::binary | std::ios::ate);

            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            int freeINodeIndex = getFirstFreeINodeIndex();
            
            // Checking if free INode exists
            if (freeINodeIndex == -1) {
                std::cout << "CANNOT COPY FILE " << extractFileName(name) << " TO SYSTEM " << systemName << std::endl;
                std::cout << "NO FREE INODES" << std::endl;
                return;
            }

            // Checking if there is enough space for the file
            if (fileSize > getAmountOfFreeDataBlocks() * sizeof(DataBlock)) {
                std::cout << "CANNOT COPY FILE " << extractFileName(name) << " TO SYSTEM " << systemName << std::endl;
                std::cout << "NOT ENOUGH SPACE" << std::endl;
                return;
            }

            discFile.open(systemName, std::ios::in | std::ios::out | std::ios::binary);
            
            // Creating new INode in memory and file
            INode iNode;
            strncpy(iNode.fileName, extractFileName(name).c_str(), sizeof(iNode.fileName) - 1);
            iNode.fileName[sizeof(iNode.fileName) - 1] = '\0';
            iNode.fileSize = fileSize;
            int freeDataBlockIndex = getFirstFreeDataBlockIndex();
            iNode.firstBlock = calculateDataBlockOffsetFromIndex(freeDataBlockIndex);

            discFile.seekp(superBlock.iNodeStart + freeINodeIndex * sizeof(INode), std::ios::beg);
            discFile.write(reinterpret_cast<char*>(&iNode), sizeof(INode));

            // Writing the file to memory and file
            size_t remainingSize = fileSize;
            size_t currentBlockOffset = iNode.firstBlock;
            while (remainingSize > 0) {
                DataBlock dataBlock;
                size_t sizeToRead = std::min(remainingSize, sizeof(dataBlock.data));
                int currentBlockIndex = calculateDataBlockIndexFromOffset(currentBlockOffset);

                file.read(dataBlock.data, sizeToRead);
                remainingSize -= sizeToRead;

                // Find the next free data block
                int nextBlockIndex = (remainingSize > 0) ? getNextFreeDataBlockIndex(currentBlockIndex) : 0;
                dataBlock.nextBlock = (nextBlockIndex != -1) ? calculateDataBlockOffsetFromIndex(nextBlockIndex) : 0;
                
                // Write the data block to the file
                discFile.seekp(currentBlockOffset, std::ios::beg);
                discFile.write(reinterpret_cast<char*>(&dataBlock), sizeof(DataBlock));

                dataBlocks[currentBlockIndex] = dataBlock;

                currentBlockOffset = dataBlock.nextBlock;
            }

            file.close();
            discFile.close();

            std::cout << "FILE '" << extractFileName(name) << "' HAS BEEN SUCCESSFULLY COPIED TO SYSTEM '" << systemName << "'." << std::endl;
        }

        void copyFileFromSystem(const std::string& systemName, const std::string& fileName) {
            loadSystem(systemName);

            // Checking if the file exists in the system
            int fileIndex = getINodeIndex(fileName);

            if (fileIndex == -1) {
                std::cout << "CANNOT COPY FILE '" << fileName << "' FROM SYSTEM '" << systemName << "'" << std::endl;
                std::cout << "FILE " << fileName << " NOT FOUND" << std::endl;
                return;
            }

            // Getting the file size and the first block offset from INode in memory
           INode fileINode = iNodes[fileIndex];
           size_t fileSize = fileINode.fileSize;
           size_t currentBlockOffset = fileINode.firstBlock;

           std::ofstream file(fileName, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

           discFile.open(systemName, std::ios::in | std::ios::out | std::ios::binary);

            // Writing the file from memory to the file
           while (fileSize > 0) {
                int currentBlockIndex = calculateDataBlockIndexFromOffset(currentBlockOffset);

                DataBlock dataBlock = dataBlocks[currentBlockIndex];

                size_t sizeToWrite = std::min(fileSize, sizeof(dataBlock.data));
                file.write(dataBlock.data, sizeToWrite);

                fileSize -= sizeToWrite;
                currentBlockOffset = dataBlock.nextBlock;
           }

            file.close();
            discFile.close();

            std::cout << "FILE '" << fileName << "' HAS BEEN SUCCESSFULLY COPIED FROM SYSTEM '" << systemName << std::endl;
        }   

        void deleteSystem(const std::string& name) {
            loadSystem(name);
            
            if (remove(name.c_str()) != 0) {
                std::cout << "ERROR DURING DELETING SYSTEM HAS OCCURED" << std::endl;
            } else {
                std::cout << "SYSTEM " << name << " HAS BEEN DELETED" << std::endl;
            }
        }

        void showFiles(const std::string& systemName) {
            loadSystem(systemName);
            for (size_t i = 0; i < iNodes.size(); i++) {
                if (!isINodeFree(i)) {
                    std::cout << iNodes[i].fileName << std::endl;
                }
            }
        }
        
        void showMemoryMap(const std::string& systemName) {
            loadSystem(systemName);
            std::cout << "------INODES------" << std::endl;
            for (size_t i = 0; i < iNodes.size(); i++) {
                if (i % MAP_NEW_LINE == 0 && i != 0) {
                    std::cout << std::endl;
                }
                std::cout << (isINodeFree(i) ? '0' : '*');
            }
            std::cout << std::endl;

            std::cout << "----DATA BLOCKS----" << std::endl;
            for (size_t i = 0; i < dataBlocks.size(); i++) {
                if (i % MAP_NEW_LINE == 0 && i != 0) {
                    std::cout << std::endl;
                }
                std::cout << (isDataBlockFree(i) ? '0' : '*');
            }
            std::cout << std::endl;
        }
};

void printHelp() {
    std::cout << "USAGE:" << std::endl;
    std::cout << "<FILE_SYSTEM_NAME> <COMMAND> <COMMAND_ARGS>" << std::endl;
    std::cout << "AVAILABLE COMMANDS: " << std::endl;
    std::cout << "CREATE <SIZE> - CREATE A NEW FILE SYSTEM" << std::endl;
    std::cout << "DELETE - DELETE FILE SYSTEM" << std::endl;
    std::cout << "COPYTO <FILE PATH> - COPY FILE TO FILE SYSTEM" << std::endl;
    std::cout << "COPYFROM <FILE NAME> - COPY FILE FROM FILE SYSTEM" << std::endl;
    std::cout << "RM <FILE NAME> - DELETE FILE FROM FILE SYSTEM" << std::endl;
    std::cout << "LS - SHOW FILES IN FILE SYSTEM" << std::endl;
    std::cout << "MAP - SHOW MEMORY MAP" << std::endl;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        printHelp();
        return 0;
    }

    std::string command = argv[2];

    VirtualFileSystem fileSystem;

    if (command == "CREATE") {
        
        if (argc != 4) {
            printHelp();
            return 0;
        }
        size_t size = std::stoul(argv[3]);
        fileSystem.createSystem(size, argv[1]);

    } else if (command == "DELETE") {

        fileSystem.deleteSystem(argv[1]);

    } else if (command == "COPYTO") {

        if (argc != 4) {
            printHelp();
            return 0;
        }
        fileSystem.copyFileToSystem(argv[1], argv[3]);

    } else if (command == "COPYFROM") {

        if (argc != 4) {
            printHelp();
            return 0;
        }
        fileSystem.copyFileFromSystem(argv[1], argv[3]);

    } else if (command == "RM") {

        if (argc != 4) {
            printHelp();
            return 0;
        }
        fileSystem.deleteFile(argv[1], argv[3]);

    } else if (command == "LS") {

        if (argc != 3) {
            printHelp();
            return 0;
        }
        fileSystem.showFiles(argv[1]);

    } else if (command == "MAP") {

        if (argc != 3) {
            printHelp();
            return 0;
        }
        fileSystem.showMemoryMap(argv[1]);

    } else {
        printHelp();
    }
    
    return 0;
}


