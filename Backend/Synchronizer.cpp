//
// Created by William Leonard Sumendap on 6/22/21.
//

#include "Synchronizer.h"
#include <thread>
#include <chrono>
#include <tuple>

Synchronizer::Synchronizer(int N) :
mq(100 * N), reqSema(N), respSema(N)
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

    mqPublish(id, n);
}

void Synchronizer::mqPublish(int id, int n)
{
    const std::shared_ptr<std::pair<int, int>> pair = std::make_shared<std::pair<int, int>>(std::pair<int, int>(id, n));

    mq.push_wait(*pair);
}

void Synchronizer::initListener()
{
    while (true)
    {
        std::shared_ptr<std::pair<int, int>> resp = mq.pop_wait();
        respSema.wait(); // WILL RELEASE UPON WORKER COMPLETION

        spawnWorkerThread(resp->first, resp->second);
    }
}

void Synchronizer::start()
{
    std::cout << "Starting Listener..." << std::endl
              << std::endl;
    initListener();
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

    ref[state->id] = 2 * state->n;

    // TODO:vacate the corresponding sema for requester AND RESPONSE SEMA
    pthread_mutex_unlock(state->arrayLock);
    state->respSema->notify();

    free(arg);
}

void Synchronizer::spawnWorkerThread(int id, int n)
{
    pthread_t workerThread;
    // TODO: CREATE APPROPRIATE TUPLE!!!!!

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
