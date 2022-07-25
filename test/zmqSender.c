// A test application to receive the message published in FrameProcessorTest
#include <assert.h>
#include <string.h>

#include <zmq.h>

int main(int argc, char** argv)
{
    void* context = zmq_ctx_new();
    void* socket = zmq_socket(context, ZMQ_REQ);
    char* endpoint = "tcp://127.0.0.1:10001";
    int rc = zmq_connect(socket, argv[1]);
    assert(rc == 0);

    char request[50];
    strcpy(request, argv[2]);
    printf("Sending '%s'\n", request);
    rc = zmq_send(socket, request, strlen(request), 0);
    assert(rc != -1);

    // Check response
    int LENGTH = 100;
    char response[LENGTH];
    rc = zmq_recv(socket, response, LENGTH, 0);
    assert(rc != -1);
    printf("Received '%s'\n", response);

    zmq_close(socket);
    zmq_ctx_destroy(context);

    return 0;
}
