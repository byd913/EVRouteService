
#include "EVRouteService.h"

volatile sig_atomic_t _bMainLoop;
static void LoopInterrupt()
{
    _bMainLoop = 0;
}

EVConfig *ConstructEVConfig(const char *ip, int port, int work_count, int timeout_in_sec) 
{
    EVConfig *config = (EVConfig *)calloc(1, sizeof(EVConfig));
    if (config == NULL) {
        return NULL;
    }

    config->ip = ip;
    config->port = port;
    config->work_count = work_count;
    config->timeout_in_sec = timeout_in_sec;

    return config;
}


void DestructEVConfig(EVConfig *config) 
{
    if (config != NULL) {
        free(config);
        config = NULL;
    }
}

Worker *ConstructWorker(EVRouteServer *server) 
{
    Worker *woker = (Worker *)calloc(1, sizeof(Worker));
    if (woker == NULL) {
        return NULL;
    }

    woker->server = server;

    return woker;
}

static void *woker_main(void *arg) 
{
    Worker *woker = (Worker *)arg;

    woker->base = event_base_new();
    if (woker->base == NULL) {
        return NULL;
    }
    woker->http = evhttp_new(woker->base);
    if (woker->http == NULL) {
        return NULL;
    }
    
    int ret = evhttp_accept_socket(woker->http, woker->server->fd);
    if (ret != 0) {
        return NULL;
    }

    evhttp_set_timeout(woker->http,  woker->server->config->timeout_in_sec);

    evhttp_set_cb(woker->http, "/process", ProcessFunction, woker);

    ret = event_base_dispatch(woker->base) {
        return NULL;
    }

    return NULL;
}

int StartWoker(Worker *woker)
{
    int ret = 0;
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if (ret != 0)｛
        return -1;
    }
    
    ret = pthread_attr_setstacksize(&attr, 4*1024*1024);
    if (ret != 0) {
        return -1;
    }

    ret = pthread_create(&woker->td, &attr, woker_main, woker);
    if (ret != 0) {
        return -1;
    }
    
    ret = pthread_attr_destroy(&attr);
    if (ret != 0) {
        return -1;
    }

    return ret;
}


void DestructWoker(Worker *woker)
{
    int ret = pthread_cancel(woker->fd);
    if (ret != 0) {
        //LOG
    }

    void *res;
    ret = pthread_join(woker->td, &res);
    if (ret != 0) {
        //Log
    }

    evhttp_free(woker->http);
    evhttp_base_free(woker->base);
    free(woker);
}


const char *GetEVServerType()
{
    return "Walk Service";
}

EVRouteService *ConstructEVServer(EVConfig *config)
{
    EVRouteServer *server = (EVRouteServer *)calloc(1, sizeof(EVRouteServer));
    if (server == NULL) {
        return NULL;
    }

    server->config = config;
    server->woker_pool = (Worker **)calloc(server->config->work_count, sizeof(Worker *));
    if (server->woker_pool == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < server->config->work_count; i++) {
        server->woker_pool[i] = ConstructWorker(server);
    }

    return server;
}


void DestructServer(EVRouteServer *server)
{
    for (int i = 0; i < server->config->work_count; i++) {
        DestructWoker(server->woker_pool[i]);
    }
    close(server->fd);

    free(server->woker_pool);
    DestructEVConfig(server->config);
    free(server);
}

int SetupServerSocket(const char *ip, int port) 
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memset(&(addr.sin_addr), 0, sizeof(addr.sin_addr));
    addr.sin_addr.s_addr = inet_addr(ip);

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0){
        return -1;
    }

    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    if (ret < 0) {
        return -1;
    }

    ret = evutil_make_socket_nonblocking(fd);
    if (ret < 0) {
        return -1;
    }

    ret = bind(fd, (sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        return -1;
    }

    ret = listen(fd, SOMAXCONN);
    if (ret < 0) {
        return -1;
    }
    
    return fd;
}

int StartEVServer(EVRouteServer *server) 
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    server->fd = SetupServerSocket(server->config->ip, server->config->port);
    if (server->fd < 0) {
        return;
    }

    for (int i = 0; i < server->config->work_count; i++) {
        StartWoker(server->woker_pool[i]);
    }

    _bMainLoop = 1;
    while (_bMainLoop) {
        usleep(2000 * 1000);
    }

    DestructServer(server);
}

