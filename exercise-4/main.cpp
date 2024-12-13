#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <chrono>

#include "monitor.h"

int makeProduct(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

class IntegerBufferMonitor : public Monitor {
    
    private:
        int id;
        int *body;
        int size;
        int counter;
        int in;
        int out;

        Condition empty;
        Condition full;

        std::string getBufferState() {
            std::string bufferState = "[" + std::to_string(id) + "] [";
            for (int i = 0; i < size; ++i) {

                if (body[i] == -1) {
                    bufferState += "X";
                } else {
                    bufferState += std::to_string(body[i]);
                }

                if (i < size - 1) {
                    bufferState += " ";
                }
            }
            bufferState += "]";

            return bufferState;
        }
    
    public:
        IntegerBufferMonitor(int p_id, int p_size) : id(p_id), size(p_size), counter(0), in(0), out(0) {
            body = new int[size];
            std::fill(body, body + size, -1);
        }

        ~IntegerBufferMonitor() {
            delete[] body;
        }

        void produce(int producer_id) {
            enter();

            if (counter == size) {
                wait(full);
            }

            int item = makeProduct(id * 10, id * 10 + 9);

            body[in] = item;

            std::cout << "[PRODUCER " << producer_id << "] PRODUCED " << item << " AT INDEX " << in << std::endl << getBufferState() << std::endl;
            
            in = (in + 1) % size;
            counter++;

            signal(empty);

            leave();

        }

        void consume(int consumer_id) {
            enter();

            if (counter == 0) {
                wait(empty);
            }

            int item = body[out];

            body[out] = -1;

            std::cout << "[CONSUMER " << consumer_id << "] CONSUMED " << item << " FROM INDEX " << out << std::endl << getBufferState() << std::endl;

            out = (out + 1) % size;
            counter--;

            signal(full);
            
            leave();
        }
};

void newProducer(IntegerBufferMonitor &buffer, int id) {
    while (true) {
        buffer.produce(id);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void newEntrepreneur(std::vector<std::reference_wrapper<IntegerBufferMonitor>> &buffers, int id) {
    int counter = 0;
    int size = buffers.size();
    while (true) {
        buffers[counter].get().produce(id);
        counter = (counter + 1) % size;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void newConsumer(IntegerBufferMonitor &buffer, int id) {
    while (true) {
        buffer.consume(id);
        std::this_thread::sleep_for(std::chrono::seconds(1));
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

    IntegerBufferMonitor buffer(1, bufferSize);
    IntegerBufferMonitor buffer2(2, bufferSize);
    IntegerBufferMonitor buffer3(3, bufferSize);
    IntegerBufferMonitor buffer4(4, bufferSize);

    std::vector<std::reference_wrapper<IntegerBufferMonitor>> buffers = {buffer, buffer2, buffer3, buffer4};

    std::thread producerFirstBuffer(newProducer, std::ref(buffer), 1);
    std::thread producerLastBuffer(newProducer, std::ref(buffer4), 2);
    std::thread entrepreneur(newEntrepreneur, std::ref(buffers), 3);
    std::thread consumerFirstBuffer(newConsumer, std::ref(buffer), 1);
    std::thread consumerSecondBuffer(newConsumer, std::ref(buffer2), 2);
    std::thread consumerThirdBuffer(newConsumer, std::ref(buffer3), 3);
    std::thread consumerLastBuffer(newConsumer, std::ref(buffer4), 4);

    producerFirstBuffer.join();
    producerLastBuffer.join();
    entrepreneur.join();
    consumerFirstBuffer.join();
    consumerSecondBuffer.join();
    consumerThirdBuffer.join();
    consumerLastBuffer.join();

    return 0;
}