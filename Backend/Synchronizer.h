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
#include "SimplePocoHandler.h"
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
  void startMQHandler();

private:
  int N;
  int threadId;

  Semaphore reqSema;
  Semaphore respSema;

  pthread_mutex_t threadIdLock;
  std::vector<pthread_mutex_t> lockArray;
  pthread_mutex_t coutLock;

  std::vector<int> responses;

  BoundedBuffer<std::pair<int, int>> mq;

  SimplePocoHandler handler;
  AMQP::Connection connection;
  AMQP::Channel channel;

};
#endif //SYNCHRONIZER_SYNCHRONIZER_H
