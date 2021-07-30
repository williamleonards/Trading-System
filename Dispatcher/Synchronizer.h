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

#include "SimplePocoHandler.h"
#include "Sema.cpp"

class Synchronizer
{
public:
  Synchronizer(int N);
  ~Synchronizer();
  std::string query(std::string args);
  void start();

private:
  void initializeZero(pthread_mutex_t *lock);
  int getAndIncr();
  void sendRequest(int id, std::string request);
  void spawnWorkerThread(int id, std::string args);
  void startMQHandler();

private:
  int N;
  int threadId;

  Semaphore reqSema;
  Semaphore respSema;

  pthread_mutex_t threadIdLock;
  std::vector<pthread_mutex_t> lockArray;
  pthread_mutex_t coutLock;

  std::vector<std::string> responses;

  SimplePocoHandler handler;
  AMQP::Connection connection;
  AMQP::Channel channel;

};
#endif //SYNCHRONIZER_SYNCHRONIZER_H
