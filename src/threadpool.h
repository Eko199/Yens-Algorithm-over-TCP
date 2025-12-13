#pragma once

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class Threadpool {
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex mut;
    std::condition_variable cv;
    bool running = true;
    unsigned pendingTasks = 0;
    std::mutex waitMut;
    std::condition_variable waitCv;

public:

    Threadpool(size_t n = std::thread::hardware_concurrency());
    ~Threadpool();

    template <typename F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(mut);
            tasks.push(std::move(task));
        }
        
        {
            std::unique_lock<std::mutex> lock(waitMut);
            ++pendingTasks;
        }
        
        cv.notify_one();
    }

    void wait_finished();
};

#endif