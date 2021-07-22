//
// Created by William Leonard Sumendap on 6/22/21.
//

#ifndef SYNCHRONIZER_SYNCHRONIZER_H
#define SYNCHRONIZER_SYNCHRONIZER_H

#include <iostream>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <vector>

#include "BoundedBuffer.cpp"
#include "Sema.cpp"

class Synchronizer
{
public:
  Synchronizer(int N);
  ~Synchronizer();
  int query(int n);
  void start();

private:
  void initializeZero(pthread_mutex_t *lock);
  int getAndIncr();
  void sendRequest(int id, int n);
  void mqPublish(int id, int n);
  void initListener();
  void spawnWorkerThread(int id, int n);


private:

  int N;

  Semaphore reqSema;
  Semaphore respSema;

  pthread_mutex_t threadIdLock;
  int threadId;

  std::vector<pthread_mutex_t> lockArray;
  std::vector<int> responses;

  BoundedBuffer<std::pair<int, int>> mq;

  pthread_mutex_t coutLock;
};
#endif //SYNCHRONIZER_SYNCHRONIZER_H
