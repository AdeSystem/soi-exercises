#include <iostream>
#include <random>
#include <string>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "monitor.h"

int makeProduct(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

struct IntegerBuffer {
    int id;
    int size;
    int min;
    int max;
    int in = 0;
    int out = 0;
    
    int* body;
    Semaphore* mutex;
    Semaphore* empty;
    Semaphore* full;

    static IntegerBuffer* create(int id, int size, int min, int max) {
        std::string memoryName = "Buffer_" + std::to_string(id);
        
        int shm_fd = shm_open(memoryName.c_str(), O_CREAT | O_RDWR, 0666);
        
        if (shm_fd == -1) {
            std::cerr << "ERROR OPENING SHARED MEMORY" << std::endl;
            return nullptr;
        }

        size_t bufferSize = sizeof(IntegerBuffer) + size * sizeof(int);

        if (ftruncate(shm_fd, bufferSize) == -1) {
            std::cerr << "ERROR TRUNCATING SHARED MEMORY" << std::endl;
            return nullptr;
        }

        IntegerBuffer* buffer = (IntegerBuffer*)mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        if (buffer == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }

        buffer->id = id;
        buffer->size = size;
        buffer->min = min;
        buffer->max = max;
        buffer->body = (int*)(buffer + 1);
        buffer->in = 0;
        buffer->out = 0;

        buffer->mutex = new(mmap(nullptr, sizeof(Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) Semaphore(1);
        buffer->empty = new(mmap(nullptr, sizeof(Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) Semaphore(size);
        buffer->full = new(mmap(nullptr, sizeof(Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) Semaphore(0);

        return buffer;
    }

    static void destroy(IntegerBuffer* buffer) {
        if (!buffer) {
            std::cerr << "Buffer is null, cannot destroy." << std::endl;
            return;
        }

        std::string memoryName = "Buffer_" + std::to_string(buffer->id);

        if (buffer->mutex) {
            buffer->mutex->~Semaphore();
        }
        
        if (buffer->empty) {
            buffer->empty->~Semaphore();
        }
        
        if (buffer->full) {
            buffer->full->~Semaphore();
        }

        size_t bufferSize = sizeof(IntegerBuffer) + buffer->size * sizeof(int);
        if (munmap(buffer, bufferSize) == -1) {
            perror("munmap");
        }

        if (shm_unlink(memoryName.c_str()) == -1) {
            perror("shm_unlink");
        }
    }
};

void produce(int id, IntegerBuffer* buffer) {
        std::cout << "[PRODUCER " << id <<"] WAITING FOR EMPTY" << std::endl << "--------------------------------------" << std::endl;
        buffer->empty->p();
        std::cout << "[PRODUCER " << id <<"] WAITING FOR MUTEX" << std::endl << "--------------------------------------" << std::endl;
        buffer->mutex->p();
        std::cout << "[PRODUCER " << id <<"] ENTERED CRITICAL SECTION" << std::endl << "--------------------------------------" << std::endl;
        int product = makeProduct(buffer->min, buffer->max);
        buffer->body[buffer->in] = product;
        std::cout << "[PRODUCER " << id <<"] PRODUCED " << product << " AT INDEX " << buffer->in << std::endl << "--------------------------------------" << std::endl;
        buffer->in = (buffer->in + 1) % buffer->size;
        buffer->mutex->v();
        std::cout << "[PRODUCER " << id <<"] LEFT CRITICAL SECTION" << std::endl << "--------------------------------------" << std::endl;
        buffer->full->v();
        std::cout << "[PRODUCER " << id <<"] SIGNALLED FULL" << std::endl << "--------------------------------------" << std::endl;
        sleep(1);
}

void consume(int id, IntegerBuffer* buffer) {
    while (true) {
        std::cout << "[CONSUMER " << id <<"] WAITING FOR FULL" << std::endl << "--------------------------------------" << std::endl;
        buffer->full->p();
        std::cout << "[CONSUMER " << id <<"] WAITING FOR MUTEX" << std::endl << "--------------------------------------" << std::endl;
        buffer->mutex->p();
        std::cout << "[CONSUMER " << id <<"] ENTERED CRITICAL SECTION" << std::endl << "--------------------------------------" << std::endl;
        int product = buffer->body[buffer->out];
        std::cout << "[CONSUMER " << id <<"] CONSUMED " << product << " FROM INDEX " << buffer->out << std::endl << "--------------------------------------" << std::endl;
        buffer->out = (buffer->out + 1) % buffer->size;
        buffer->mutex->v();
        std::cout << "[CONSUMER " << id <<"] LEFT CRITICAL SECTION" << std::endl << "--------------------------------------" << std::endl;
        buffer->empty->v();
        std::cout << "[CONSUMER " << id <<"] SIGNALLED EMPTY" << std::endl << "--------------------------------------" << std::endl;
        sleep(1);
    }
}

void newProducer(int id, IntegerBuffer* buffer) {
    int created = fork();
    if (created == 0) {
        while (true) { produce(id, buffer); }
        exit(0);
    } else if (created < 0) {
        perror("fork");
        exit(1);
    }
}

void newEntrepreneur(int id, IntegerBuffer* buffer, IntegerBuffer* buffer2, IntegerBuffer* buffer3, IntegerBuffer* buffer4) {
    int created = fork();
    if (created == 0) {
        int counter = 0;
        while (true) {
            if (counter == 0) {
                produce(id, buffer);
            } else if (counter == 1) {
                produce(id, buffer2);
            } else if (counter == 2) {
                produce(id, buffer3);
            } else if (counter == 3) {
                produce(id, buffer4);
            }
            counter = (counter + 1) % 4;
        }
        exit(0);
    } else if (created < 0) {
        perror("fork");
        exit(1);
    }
}

void newConsumer(int id, IntegerBuffer* buffer) {
    int created = fork();
    if (created == 0) {
        while (true) { consume(id, buffer); }
        exit(0);
    } else if (created < 0) {
        perror("fork");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int bufferSize;
    
    if (argc != 2) {
        std::cerr << "ENTER BUFFER SIZE" << std::endl;
        return 1;
    }

    try {
        bufferSize = std::stoi(argv[1]);
    } catch (std::exception& e) {
        std::cerr << "INVALID BUFFER SIZE" << std::endl;
        return 1;
    }

    IntegerBuffer* buffer = IntegerBuffer::create(1, bufferSize, 10, 19);

    if (!buffer) {
        std::cerr << "ERROR CREATING BUFFER" << std::endl;
        return 1;
    }

    IntegerBuffer* buffer2 = IntegerBuffer::create(2, bufferSize, 20, 29);

    if (!buffer2) {
        std::cerr << "ERROR CREATING BUFFER" << std::endl;
        return 1;
    }

    IntegerBuffer* buffer3 = IntegerBuffer::create(3, bufferSize, 30, 39);

    if (!buffer3) {
        std::cerr << "ERROR CREATING BUFFER" << std::endl;
        return 1;
    }

    IntegerBuffer* buffer4 = IntegerBuffer::create(4, bufferSize, 40, 49);

    if (!buffer4) {
        std::cerr << "ERROR CREATING BUFFER" << std::endl;
        return 1;
    }

    newProducer(1, buffer);
    newProducer(2, buffer4);
    newEntrepreneur(3, buffer, buffer2, buffer3, buffer4);

    newConsumer(1, buffer);
    newConsumer(2, buffer2);
    newConsumer(3, buffer3);
    newConsumer(4, buffer4);

    while(true) {}

    IntegerBuffer::destroy(buffer);
    IntegerBuffer::destroy(buffer2);
    IntegerBuffer::destroy(buffer3);
    IntegerBuffer::destroy(buffer4);

    return 0;
}