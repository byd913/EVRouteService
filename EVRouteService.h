#ifndef EVROUTESERVICE_H
#define EVROUTESERVICE_H

#include <pthread.h>

struct EVConfig {
    const char *ip;
    int port;
    int work_count;
    int timeout_in_sec; //timeout for an HTTP request, in second
};

EVConfig *ConstructEVConfig(const char *ip, int port, int work_count, int timeout_in_sec);

void DestructEVConfig(EVConfig *config);

struct EVRouteServer;

#define DEF_RECVBUFF_SIZE 1024 * 64

struct Worker {
    pthread_t td;
    struct event_base *base;
    struct evhttp *http;

    EVRouteServer *server;
};

Worker *ConstructWorker(EVRouteServer *server);

int StartWorker(Worker *worker);

void DestructWoker(Worker *worker);

struct EVRouteServer {
    EVConfig *config;
    int fd;
    Worker ** woker_pool;
    int requestId;
    mcMutex mutex;

    EVRouteServer () {
    }
};

const char *GetEVServerType();

EVRouteServer *ConstructEVServer(EVConfig *config);

int StartEVServer(EVRouteServer *server);

void DestructServer(EVRouteServer *server);

int SetupServerSocket(const char *ip, int port);

#endif //EVROUTESERVICE_H


