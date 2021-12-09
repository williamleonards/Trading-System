//
// Created by William Leonard Sumendap on 6/22/21.
//

#include "Synchronizer.h"
#include <thread>
#include <chrono>
#include <tuple>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

Synchronizer::Synchronizer(int N) :
	handler("127.0.0.1", 5672),
	connection(&handler, AMQP::Login("guest", "guest"), "/"),
	channel(&connection),
	reqSema(N), respSema(N)
{
    this->N = N;
    lockArray = std::vector<pthread_mutex_t>(N);

    for (int i = 0; i < N; i++)
    {
        initializeZero(&lockArray[i]);
    }

    pthread_mutex_init(&threadIdLock, NULL);
    threadId = 0;

    responses = std::vector<std::string>(N);

    pthread_mutex_init(&coutLock, NULL);
}

Synchronizer::~Synchronizer() {}

void Synchronizer::initializeZero(pthread_mutex_t *lock)
{
    pthread_mutex_init(lock, NULL);
    pthread_mutex_lock(lock);
}

json Synchronizer::query(json &args)
{
    reqSema.wait();
    int id = getAndIncr();
    
    args["id"] = id;
    sendRequest(id, args);

    pthread_mutex_lock(&lockArray[id]); // WILL BE RELEASED BY WORKER THREAD

    std::string ans = responses[id];
    reqSema.notify();
    
    return json::parse(ans);
}

void Synchronizer::sendRequest(int id, json &request)
{
    std::cout << "Request is " << request << std::endl;

    if (channel.ready())
    {
        channel.publish("ts-exchange", "generic-request", request.dump() + '\n');
    } else {
        std::cout << "Can't publish, channel unavailable" << std::endl;
    }
}

void *startHandler(void *arg)
{
    SimplePocoHandler *h = (SimplePocoHandler *) arg;

    h->loop();
}

void Synchronizer::startMQHandler()
{
    pthread_t starter;
    pthread_create(&starter, NULL, &startHandler, (void *) &handler);

    channel.onReady([&]() {
        std::cout << "Channel is ready!" << std::endl;

    });

    channel.declareExchange("ts-exchange", AMQP::direct);
    channel.declareQueue("ts-generic-request");
    channel.bindQueue("ts-exchange", "ts-generic-request", "generic-request");
    channel.consume("ts-generic-response", AMQP::noack).onReceived(
            [this](const AMQP::Message &message,
                   uint64_t deliveryTag,
                   bool redelivered)
            {
                std::cout << "[x] Received " << message.body() << std::endl << std::endl;

                std::string msg = message.body();
                msg = msg.substr(0, msg.find_first_of('\n'));

                json args = json::parse(msg);

                int id = args["id"];
                std::string response = args["response"].dump();

                std::cout << "Request id is " << id << " and response is " << response << std::endl;
                respSema.wait(); // WILL RELEASE UPON WORKER COMPLETION
                spawnWorkerThread(id, response);
            });

    // TODO: TO BLOCK OR NOT TO BLOCK?? pthread_join(starter, NULL);
}

void Synchronizer::start()
{
    std::cout << "Starting Listener..." << std::endl
              << std::endl;
    startMQHandler();
}

struct State
{
    int id;
    std::string args;
    std::vector<std::string> *responses;
    pthread_mutex_t *arrayLock;
    Semaphore *respSema;

    State(int id_, std::string args_, std::vector<std::string> *responses_, pthread_mutex_t *arrayLock_, Semaphore *respSema_) :
            id(id_), args(args_), responses(responses_), arrayLock(arrayLock_), respSema(respSema_)
    {
    }
};

void *workerJob(void *arg)
{
    State *state = (State *)arg;


    std::vector<std::string> *responses = state->responses;
    std::vector<std::string> &ref = *responses;

    ref[state->id] = state->args;

    // vacate the corresponding sema for requester AND RESPONSE SEMA
    pthread_mutex_unlock(state->arrayLock);
    state->respSema->notify();

    free(arg);
}

void Synchronizer::spawnWorkerThread(int id, std::string args)
{
    pthread_t workerThread;

    State *state = new State(id, args, &responses, &lockArray[id], &respSema);

    pthread_create(&workerThread, NULL, &workerJob, (void *)state);
}

int Synchronizer::getAndIncr()
{
    pthread_mutex_lock(&threadIdLock);
    int ans = threadId;
    threadId = (threadId + 1) % N;
    pthread_mutex_unlock(&threadIdLock);
    return ans;
}