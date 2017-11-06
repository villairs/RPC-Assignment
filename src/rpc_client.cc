#include "common.h"

int rpcCall(char* name, int* argTypes, void** args) {
    int sock_fd;
    int nameLen = strlen(name) + 1;
    int argTypesLen = 0;
    int msgType;
    int reasonCode;

    char* address;
    int addressLen;
    char* port;
    int portLen;

    int msgReplyType;
    char* nameReply;
    int nameReplyLen;
    int* argTypesReply;
    int argTypesReplyLen;

    while (argTypes[argTypesLen++] != 0) {}

    if(getenv("BINDER_ADDRESS")!=NULL){}
    else{return BINDER_ADDRESS_NOT_SET;}
    if(getenv("BINDER_PORT")!=NULL){}
    else{return BINDER_PORT_NOT_SET;}


    // 1) connect to binder
    sock_fd = establish(std::string(getenv("BINDER_ADDRESS")), std::string(getenv("BINDER_PORT")));
    if (sock_fd < 0) {
        return BINDER_CONNECTION_REFUSED;
    }

    // 2) send the LOC_REQUEST message
    msgType = LOC_REQUEST;
    send_debug(sock_fd, &msgType, 4, 0);

    // 3) send length of name (including null terminator) followed by name
    send_debug(sock_fd, &nameLen, 4, 0);
    send_debug(sock_fd, name, sizeof(char) * nameLen, 0);

    // 4) send length of argTypes followed by argTypes
    send_debug(sock_fd, &argTypesLen, 4, 0);
    send_debug(sock_fd, argTypes, sizeof(int) * argTypesLen, 0);

    // 5) get server information
    recv_debug(sock_fd, &msgReplyType, 4, 0);
    if (msgReplyType == LOC_FAILURE) {
        recv_debug(sock_fd, &reasonCode, 4, 0);
        close(sock_fd);
        return reasonCode;
    }

    if (msgReplyType != LOC_SUCCESS) {
        return BINDER_DISCONNECTED;
    }
    recv_debug(sock_fd, &addressLen, 4, 0);
    address = new char[addressLen];
    recv_debug(sock_fd, address, sizeof(char) * addressLen, 0);
    recv_debug(sock_fd, &portLen, 4, 0);
    port = new char[portLen];
    recv_debug(sock_fd, port, sizeof(char) * portLen, 0);

    // 6) connect to server
    close(sock_fd);
    sock_fd = establish(std::string(address), std::string(port));
    if (sock_fd < 0) {
        return SERVER_CONNECTION_REFUSED;
    }

    // 7) cleanup
    delete [] address;
    delete [] port;

    // 8) send the EXECUTE message
    msgType = EXECUTE;
    send_debug(sock_fd, &msgType, 4, 0);

    // 9) send length of name (including null terminator) followed by name
    send_debug(sock_fd, &nameLen, 4, 0);
    send_debug(sock_fd, name, sizeof(char) * nameLen, 0);

    // 10) send length of argTypes followed by argTypes
    send_debug(sock_fd, &argTypesLen, 4, 0);
    send_debug(sock_fd, argTypes, sizeof(int) * argTypesLen, 0);

    // 11) send the args
    for (int i = 0; i < argTypesLen - 1; i++) {
        int arrayLen = ARRAY_LENGTH(argTypes[i]);
        if (arrayLen == 0) arrayLen = 1;
        switch (DATA_TYPE(argTypes[i])) {
            case ARG_CHAR:
                send_debug(sock_fd, args[i], sizeof(char) * arrayLen, 0);
                break;
            case ARG_SHORT:
                send_debug(sock_fd, args[i], sizeof(short) * arrayLen, 0);
                break;
            case ARG_INT:
                send_debug(sock_fd, args[i], sizeof(int) * arrayLen, 0);
                break;
            case ARG_LONG:
                send_debug(sock_fd, args[i], sizeof(long) * arrayLen, 0);
                break;
            case ARG_DOUBLE:
                send_debug(sock_fd, args[i], sizeof(double) * arrayLen, 0);
                break;
            case ARG_FLOAT:
                send_debug(sock_fd, args[i], sizeof(float) * arrayLen, 0);
                break;
            default: assert(1 == 0); break; //what else could it be?
        }
    }

    // 12) get either EXECUTE_SUCCESS or EXECUTE_FAILURE from server
    recv_debug(sock_fd, &msgReplyType, 4, 0);
    if (msgReplyType == EXECUTE_FAILURE) {
        // get the reason code
        int reasonCode;
        recv_debug(sock_fd, &reasonCode, 4, 0);
        return reasonCode;
    }

    if(msgReplyType != EXECUTE_SUCCESS) {
        return SERVER_DISCONNECTED;
    }

    // 13) get the new name (should be unchanged)
    recv_debug(sock_fd, &nameReplyLen, 4, 0);
    nameReply = new char[nameReplyLen];

    recv_debug(sock_fd, nameReply, sizeof(char) * nameReplyLen, 0);

    // 14) get the new argTypes (should be unchanged)
    recv_debug(sock_fd, &argTypesReplyLen, 4, 0);
    argTypesReply = new int[argTypesReplyLen];

    recv_debug(sock_fd, argTypesReply, sizeof(int) * argTypesReplyLen, 0);

    // 15) get the new args
    // DANGER: WILL OVERWRITE args !!!
    for (int i = 0; i < argTypesReplyLen - 1; i++) {
        int arrayLen = ARRAY_LENGTH(argTypesReply[i]);
        if (arrayLen == 0) arrayLen = 1;
        switch (DATA_TYPE(argTypesReply[i])) {
            case ARG_CHAR:
                recv_debug(sock_fd, args[i], sizeof(char) * arrayLen, 0);
                break;
            case ARG_SHORT:
                recv_debug(sock_fd, args[i], sizeof(short) * arrayLen, 0);
                break;
            case ARG_INT:
                recv_debug(sock_fd, args[i], sizeof(int) * arrayLen, 0);
                break;
            case ARG_LONG:
                recv_debug(sock_fd, args[i], sizeof(long) * arrayLen, 0);
                break;
            case ARG_DOUBLE:
                recv_debug(sock_fd, args[i], sizeof(double) * arrayLen, 0);
                break;
            case ARG_FLOAT:
                recv_debug(sock_fd, args[i], sizeof(float) * arrayLen, 0);
                break;
            default: assert(1 == 0); break; //what else could it be?
        }
    }

    // 16) cleanup
    close(sock_fd);
    delete [] nameReply;
    delete [] argTypesReply;

    // 17) we gucci fam
    return 0;

}

int rpcTerminate() {
    int sock_fd;
    int msgType = TERMINATE;

    sock_fd = establish(std::string(getenv("BINDER_ADDRESS")), std::string(getenv("BINDER_PORT")));
    if (sock_fd < 0) {
        return TERMINATE_REFUSED;
    }
    send_debug(sock_fd, &msgType, 4, 0);
    return 0;
}
