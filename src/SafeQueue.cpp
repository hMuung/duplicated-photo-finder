#include "dpf/safeQueue.h"

// Pushes a new item into the queue and notifies one waiting thread
void SafeQueue::push(std::filesystem::path item) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(item));
    }
    cv.notify_one();
}

// Pops the next item from the queue
bool SafeQueue::pop(std::filesystem::path& item) {
    std::unique_lock<std::mutex> lock(mutex);

    // Waits until an item is available or the queue is marked finished
    cv.wait(lock, [this]() {return !queue.empty() || finished;});

    // Stop if no items remain
    if (queue.empty() && finished) {
        return false;
    }

    item = std::move(queue.front());
    queue.pop();
    return true;
}

// Marks the queue as finished and wakes all waiting threads
void SafeQueue::setFinished() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        finished = true;
    }
    cv.notify_all();
}