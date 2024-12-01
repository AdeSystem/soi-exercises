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
    int in = 0;
    int out = 0;
    
    int* body;
    Semaphore* mutex;
    Semaphore* empty;
    Semaphore* full;

    static IntegerBuffer* create(int id, int size) {
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

void produce(IntegerBuffer* buffer, int min, int max) {
    while (true) {
        std::cout << "[PRODUCER] WAITING FOR EMPTY" << std::endl;
        buffer->empty->p();
        std::cout << "[PRODUCER] WAITING FOR MUTEX" << std::endl;
        buffer->mutex->p();
        std::cout << "[PRODUCER] ENTERED CRITICAL SECTION" << std::endl;
        int product = makeProduct(min, max);
        buffer->body[buffer->in] = product;
        std::cout << "[PRODUCER] PRODUCED " << product << " AT INDEX " << buffer->in << std::endl;
        buffer->in = (buffer->in + 1) % buffer->size;
        buffer->mutex->v();
        std::cout << "[PRODUCER] EXITED CRITICAL SECTION" << std::endl;
        buffer->full->v();
        std::cout << "[PRODUCER] SIGNALLED FULL" << std::endl;
        sleep(1);
    }
}

void consume(IntegerBuffer* buffer) {
    while (true) {
        std::cout << "[CONSUMER] WAITING FOR FULL" << std::endl;
        buffer->full->p();
        std::cout << "[CONSUMER] WAITING FOR MUTEX" << std::endl;
        buffer->mutex->p();
        std::cout << "[CONSUMER] ENTERED CRITICAL SECTION" << std::endl;
        int product = buffer->body[buffer->out];
        std::cout << "[CONSUMER] CONSUMED " << product << " FROM INDEX " << buffer->out << std::endl;
        buffer->out = (buffer->out + 1) % buffer->size;
        buffer->mutex->v();
        std::cout << "[CONSUMER] EXITED CRITICAL SECTION" << std::endl;
        buffer->empty->v();
        std::cout << "[CONSUMER] SIGNALLED EMPTY" << std::endl;
        sleep(1);
    }
}

void newProducer(IntegerBuffer* buffer, int min, int max) {
    int created = fork();
    if (created == 0) {
        produce(buffer, min, max);
        exit(0);
    } else if (created < 0) {
        perror("fork");
        exit(1);
    }
}

void newConsumer(IntegerBuffer* buffer) {
    int created = fork();
    if (created == 0) {
        consume(buffer);
    } else if (created < 0) {
        perror("fork");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    std::cout << getpid() << std::endl;

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

    IntegerBuffer* buffer = IntegerBuffer::create(1, bufferSize);

    if (!buffer) {
        std::cerr << "ERROR CREATING BUFFER" << std::endl;
        return 1;
    }

    IntegerBuffer* buffer4 = IntegerBuffer::create(4, bufferSize);

    if (!buffer4) {
        std::cerr << "ERROR CREATING BUFFER" << std::endl;
        return 1;
    }

    newProducer(buffer, 10, 19);
    newProducer(buffer4, 40, 49);

    newConsumer(buffer);
    newConsumer(buffer4);

    while(true) {
        int status;
        pid_t pid = wait(&status);
        if (pid == -1) {
            break;  
        }
        std::cout << "PROCESS " << pid << " ENDED" << std::endl;
    }

    IntegerBuffer::destroy(buffer);
    IntegerBuffer::destroy(buffer4);

    return 0;
}