#ifndef __COMMON_H__
#define __COMMON_H__

#include "rpc.h"
#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>

ssize_t send_debug(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv_debug(int sockfd, void *buf, size_t len, int flags);
int establish(std::string address, std::string port);
int setup(std::string& address, std::string& port);

/* where x is an int argTypes */
#define IS_INPUT(x) ((x >> ARG_INPUT) & 1)
#define IS_OUTPUT(x) ((x >> ARG_OUTPUT) & 1)
#define DATA_TYPE(x) ((x >> 16) & 0xFF)
#define ARRAY_LENGTH(x) (x & 0xFFFF)

/* message types */
#define INITIALIZE 0
#define REGISTER 1
#define REGISTER_SUCCESS 2
#define REGISTER_FAILURE 3
#define LOC_REQUEST 4
#define LOC_SUCCESS 5
#define LOC_FAILURE 6
#define EXECUTE 7
#define EXECUTE_SUCCESS 8
#define EXECUTE_FAILURE 9
#define TERMINATE 10

/* reason codes */
#define SIGNATURE_NOT_FOUND -100
#define TERMINATE_REFUSED -300
#define BINDER_CONNECTION_REFUSED -400
#define BINDER_DISCONNECTED -425
#define SERVER_CONNECTION_REFUSED -450
#define SERVER_DISCONNECTED -475
#define SERVER_NOT_INIT -500
#define BINDER_ADDRESS_NOT_SET -600
#define BINDER_PORT_NOT_SET -700
#define SERVER_REGISTER_FAILED -800
#define NO_REGISTERED_FUNCTIONS -900
#define SERVER_FUNCTION_ALREADY_REGISTERED 100

#endif
