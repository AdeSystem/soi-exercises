#include <iostream>
#include <random>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#include "monitor.h"

#define MUL 1000000

int makeProduct(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

struct IntegerBuffer {
    int size;
    int in = 0;
    int out = 0;
    
    int* body;
    Semaphore* mutex;
    Semaphore* empty;
    Semaphore* full;

    static IntegerBuffer* create(int bufferSize) {
        void* memory = mmap(nullptr, sizeof(IntegerBuffer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        
        if (memory == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }

        IntegerBuffer* buffer = reinterpret_cast<IntegerBuffer*>(memory);

        buffer -> size = bufferSize;
        buffer -> in = 0;
        buffer -> out = 0;

        buffer -> body = reinterpret_cast<int*>(mmap(nullptr, sizeof(int) * bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

        if (buffer->body == MAP_FAILED) {
            perror("mmap for body");
            exit(1);
        }

        buffer -> mutex = new (mmap(nullptr, sizeof(Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) Semaphore(1);
        buffer -> empty = new (mmap(nullptr, sizeof(Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) Semaphore(bufferSize);
        buffer -> full = new (mmap(nullptr, sizeof(Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) Semaphore(0);

        return buffer;
    }

    static void destroy(IntegerBuffer* buffer) {
        munmap(buffer->body, sizeof(int) * buffer -> size);
        buffer -> mutex -> ~Semaphore();
        munmap(buffer -> mutex, sizeof(Semaphore));
        buffer -> empty -> ~Semaphore();
        munmap(buffer -> empty, sizeof(Semaphore));
        buffer -> full -> ~Semaphore();
        munmap(buffer -> full, sizeof(Semaphore));
        munmap(buffer, sizeof(IntegerBuffer));
    }
};

void producer(IntegerBuffer *buffer, int min, int max) {
    while(true) {
            int product = makeProduct(min, max);
            buffer->empty->p();
            std::cout << "PRODUCER entering critical section..." << std::endl;
            buffer->mutex->p();
            buffer->body[buffer->in] = product;
            std::cout << "PRODUCER produced: " << product << " at index " << buffer->in << std::endl;
            buffer->in = (buffer->in + 1) % buffer->size;
            buffer->mutex->v();
            buffer->full->v();
            std::cout << "PRODUCER signals full, exiting critical section." << std::endl;
            usleep(1 * MUL);
        }
}

void consumer(IntegerBuffer* buffer) {
    while(true) {
        std::cout << "CONSUMER waiting on full..." << std::endl;
        buffer->full->p();
        std::cout << "CONSUMER entering critical section..." << std::endl;
        buffer->mutex->p();
        int product = buffer->body[buffer->out];
        std::cout << "CONSUMER consumed: " << product << " from index " << buffer->out << std::endl;
        buffer->out = (buffer->out + 1) % buffer->size;
        buffer->mutex->v();
        buffer->empty->v();
        std::cout << "CONSUMER signals empty, exiting critical section." << std::endl;
        usleep(1 * MUL);
    }
}

void newConsumer(IntegerBuffer *buffer) {
    int created = fork();
    if (created == 0) {
        std::cout << "NEW CONSUMER PID: " << getpid() << std::endl;
        consumer(buffer);
    }
}

void newProducer(IntegerBuffer *buffer, int min, int max) {
    int created = fork();
    if (created == 0) {
        std::cout << "NEW PRODUCER PID: " << getpid() << std::endl;
        producer(buffer, min, max);
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

    IntegerBuffer* buffer = IntegerBuffer::create(bufferSize);

    newProducer(buffer, 1, 100);
    newConsumer(buffer);
    
    while(true) {}

    IntegerBuffer::destroy(buffer);

    return 0;
}