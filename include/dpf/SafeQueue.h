#include <queue>
#include <filesystem>
#include <mutex>
#include <condition_variable>

/*
Thread-safe queue for file path exchange between threads

Manages synchronized access to a shared queue of filesystem paths, ensuring safe
communication between producer and consumer threads without data races

Provides blocking retrieval using condition variables, allowing consumer threads
to efficiently wait for new items while supporting graceful shutdown signaling
when no more tasks remain to be processed
*/


class SafeQueue {
    private:
        
        std::queue<std::filesystem::path> queue;
        std::mutex mutex;
        std::condition_variable cv;
        bool finished = false;

    public:

        void push(std::filesystem::path item);
        bool pop(std::filesystem::path& item);
        void setFinished();

};
