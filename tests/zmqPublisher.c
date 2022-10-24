// A test application to receive the message published in FrameProcessorTest
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <zmq.h>

int main(int argc, char** argv)
{
    void* context = zmq_ctx_new();
    void* socket = zmq_socket(context, ZMQ_PUB);
    char* endpoint = "tcp://127.0.0.1:10000";
    int rc = zmq_bind(socket, argv[1]);
    assert(rc == 0);

    usleep(200000);

    char request[100];
    strcpy(request, argv[2]);
    printf("Sending '%s'\n", request);
    rc = zmq_send(socket, request, strlen(request), 0);
    assert(rc != -1);

    zmq_close(socket);
    zmq_ctx_destroy(context);

    return 0;
}
