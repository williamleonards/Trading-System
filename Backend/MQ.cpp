////
//// Created by William Leonard Sumendap on 6/23/21.
////
//
//#include "MQ.h"
//
//template <typename T>
//MQ<T>::MQ(int N) {
//    pthread_mutex_init(&lock, NULL);
//    sem_init(&sema, 0, 0);
//}
//
//template <typename T>
//void MQ<T>::push(T obj) {
//    pthread_mutex_lock(&lock);
//    sem_post(&sema);
//    queue.push(obj);
//    pthread_mutex_unlock(&lock);
//}
//
//template <typename T>
//T MQ<T>::pop() {
//    pthread_mutex_lock(&lock);
//    sem_wait()
//    return queue.pop();
//    pthread_mutex_unlock(&lock);
//}
//
