//
// Created by William Leonard Sumendap on 6/23/21.
//

#ifndef SYNCHRONIZER_MQ_H
#define SYNCHRONIZER_MQ_H

#include <iostream>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <vector>

template <typename T>
class MQ {
public:
    MQ(int N);
    void push(T obj);
    T pop();

private:
    std::queue<T> queue;
    pthread_mutex_t lock;
    sem_t sema;
};
#endif //SYNCHRONIZER_MQ_H
