#include "threadpool.h"

Threadpool::Threadpool(size_t n) {
    for (size_t i = 0; i < n; ++i) {
        threads.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mut);

                    cv.wait(lock, [this] {
                        return !running || !tasks.empty();
                    });

                    if (!running && tasks.empty()) {
                        return;
                    }

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                task();

                {
                    std::unique_lock<std::mutex> lock(waitMut);
                    --pendingTasks;
                }

                waitCv.notify_one();
            }
        });
    }
}

Threadpool::~Threadpool() {
    {
        std::unique_lock<std::mutex> lock(mut);
        running = false;
    }

    cv.notify_all();

    for (std::thread& thread : threads) {
        thread.join();
    }
}

void Threadpool::wait_finished() {
    std::unique_lock<std::mutex> lock(waitMut);
    waitCv.wait(lock, [this]() { 
        return pendingTasks == 0; 
    });
}