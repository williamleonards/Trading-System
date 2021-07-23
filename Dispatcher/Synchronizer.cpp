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

    responses = std::vector<int>(N);

    pthread_mutex_init(&coutLock, NULL);
}

void Synchronizer::initializeZero(pthread_mutex_t *lock)
{
    pthread_mutex_init(lock, NULL);
    pthread_mutex_lock(lock);
}

int Synchronizer::query(int n)
{
    reqSema.wait();
    int id = getAndIncr();

    sendRequest(id, n);

    pthread_mutex_lock(&lockArray[id]); // WILL BE RELEASED BY WORKER THREAD

    int ans = responses[id];

    reqSema.notify();

    pthread_mutex_lock(&coutLock);
    std::cout << std::endl;
    if (ans == 2 * n)
    {
        std::cout << "ANSWER CORRECT, which is " << ans << std::endl
                  << std::endl;
    }
    else
    {
        std::cout << "WRONG ANSWER, GOT " << ans << std::endl
                  << std::endl;
    }
    pthread_mutex_unlock(&coutLock);

    return ans;
}

void Synchronizer::sendRequest(int id, int n)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string request = std::to_string(id) + " " + std::to_string(n) + " | ";
    std::cout << "Request is " << request << std::endl;

    if (channel.ready())
    {
        channel.publish("ts-exchange", "generic-request", request);
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
                std::vector<std::string> tokens;
                boost::algorithm::split(tokens, msg, boost::algorithm::is_any_of(" "));

                int id = std::stoi(tokens[0]);
                int n = std::stoi(tokens[1]);

                std::cout << "Request id is " << id << " and n is " << n << std::endl;
                respSema.wait(); // WILL RELEASE UPON WORKER COMPLETION
                spawnWorkerThread(id, n);
            });

    pthread_join(starter, NULL);
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
    int n;
    std::vector<int> *responses;
    pthread_mutex_t *arrayLock;
    Semaphore *respSema;

    State(int id_, int n_, std::vector<int> *responses_, pthread_mutex_t *arrayLock_, Semaphore *respSema_) :
            id(id_), n(n_), responses(responses_), arrayLock(arrayLock_), respSema(respSema_)
    {
    }
};

void *workerJob(void *arg)
{
    State *state = (State *)arg;


    std::vector<int> *responses = state->responses;
    std::vector<int> &ref = *responses;

    ref[state->id] = state->n; // NOW, DOUBLING IS DONE IN THE BACKEND

    // vacate the corresponding sema for requester AND RESPONSE SEMA
    pthread_mutex_unlock(state->arrayLock);
    state->respSema->notify();

    free(arg);
}

void Synchronizer::spawnWorkerThread(int id, int n)
{
    pthread_t workerThread;

    State *state = new State(id, n, &responses, &lockArray[id], &respSema);

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