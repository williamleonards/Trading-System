//
// Created by William Leonard Sumendap on 6/22/21.
//

#ifndef DISPATCHER_SYNCHRONIZER_H
#define DISPATCHER_SYNCHRONIZER_H

#include <iostream>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <vector>

#include "SimplePocoHandler.h"
#include "Sema.cpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Synchronizer
{
public:
		Synchronizer(json config);
		~Synchronizer();
		json query(json &args);
		void start();

private:
		void initializeZero(pthread_mutex_t *lock);
		int getAndIncr();
		void sendRequest(int id, json &request);
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

		// MQ Configs
		std::string host;
		int port;
		std::string user;
		std::string password;
		std::string vhost;
		std::string exchange;
		std::string requestQueue;
		std::string requestRouting;
		std::string responseQueue;
		std::string responseRouting;
};
#endif //DISPATCHER_SYNCHRONIZER_H
