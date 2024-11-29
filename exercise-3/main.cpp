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
    int *body;
    int in = 0;
    int out = 0;

    Semaphore mutex;
    Semaphore empty;
    Semaphore full;

    IntegerBuffer(int p_size) : size(p_size), body(new int[size]), mutex(Semaphore(1)), empty(Semaphore(size)), full(Semaphore(0)) {}
    ~IntegerBuffer() { delete[] body; }

    void put(int product) {
        empty.p();
        mutex.p();
        body[in] = product;
        in = (in + 1) % size;
        mutex.v();
        full.v();
    }

    int get() {
        full.p();
        mutex.p();
        int product = body[out];
        out = (out + 1) % size;
        mutex.v();
        empty.v();
        return product;
    }
};

void producer(IntegerBuffer *buffer, int min, int max) {
    while(true) {
            int product = makeProduct(min, max);
            std::cout << "PRODUCER CREATED: " << product << std::endl;
            buffer -> put(product);
            std::cout << "PRODUCER PUT: " << product << std::endl;
            usleep(1 * MUL);
        }
}

void consumer(IntegerBuffer* buffer) {
    while(true) {
        std::cout << "CONSUMER GETS PRODUCT" << std::endl;
        int product = buffer -> get();
        std::cout << "CONSUMER GOT: " << product << std::endl;
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

void newProducer(IntegerBuffer *buffer) {
    int created = fork();
    if (created == 0) {
        std::cout << "NEW PRODUCER PID: " << getpid() << std::endl;
        producer(buffer, 1, 100);
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

    while (true) {}

    return 0;
}