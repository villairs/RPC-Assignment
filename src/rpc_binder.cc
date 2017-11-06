#include "rpc_binder.h"
using namespace std;

Binder::Binder() {
    numSocks = 0;
    string address;
    string port;
    int sock_fd = setup(address, port);
    fds[numSocks].fd = sock_fd;
    fds[numSocks].events = POLLIN;
    numSocks++;


    cout << "BINDER_ADDRESS " << address << endl;
    cout << "BINDER_PORT " << port << endl;
}

Binder::~Binder() {}

Signature Binder::createSignature(char* name, int* argTypes) {
    vector<int> v;
    string s = string(name);
    int count = 0;

    while (argTypes[count] != 0) {
        if (ARRAY_LENGTH(argTypes[count]) > 0) {
            argTypes[count] = argTypes[count] & (~0U << 16);
            argTypes[count] = argTypes[count] | 1;
        }
        v.push_back(argTypes[count]);
        count++;
    }

    return make_pair(s,v);
}

Server Binder::createServer(string address, string port){
return make_pair(address,port);
}

Server Binder::getNextServer(Signature sig) {
    list<Server>::iterator it;
    Server result;

    // Check if sig exists at all
    if (bindings.find(sig) == bindings.end()) {
        result = make_pair("0", "0");
        return result;
    }


    // the set of servers which can fulfill sig
    set<Server> canFulfill = bindings.find(sig)->second;
    if (canFulfill.size() == 0) {
        result = make_pair("0", "0");
        return result;
    }

    for (it = queue.begin(); it != queue.end(); ++it) {
        if (canFulfill.find(*it) != canFulfill.end()) {
            result = *it;
            queue.erase(it); //iterator invalidated
            queue.push_back(result);
            return result;
        }
    }

    assert(1 == 0); //shouldn't reach here
}

bool Binder::initServer(Server v) {
    // Check if v already exists in queue
    if (find(queue.begin(), queue.end(), v) != queue.end()) {
        return false;
    }

    queue.push_front(v);
    return true;
}

bool Binder::registerSignature(Server v, Signature sig) {
    // Check if bindings[sig] exists at all
    if (bindings.find(sig) == bindings.end()) {
        set<Server> s;
        s.insert(v);
        bindings.insert(make_pair(sig, s));
        return true;
    }

    // Check if v already exists in bindings[sig]
    if (bindings[sig].find(v) != bindings[sig].end()) {
        return false;
    }

    bindings[sig].insert(v);
    return true;
}

void Binder::run() {
    while (true) {
        int sock_fd = 0;
        int i;
        int r = poll(fds, numSocks, 0);
        if (r < 0) {
            perror("poll");
            continue;
        }
        else {
            if (fds[0].revents & POLLIN) {
                // new connection
                sock_fd = accept(fds[0].fd, NULL, NULL);
            }
            else {
                for (i = 1; i < numSocks; i++) {
                    if (fds[i].revents & POLLIN) {
                        sock_fd = fds[i].fd;
                        break;
                    }
                }
            }
        }

        if (sock_fd > 0) {
            // Get message type
            int msgType;
            if (recv_debug(sock_fd, &msgType, 4, 0) == 0) {
                deregister(sock_fd);
                fds[i].fd = 0;
                fds[i].revents = 0;
                continue;
            }

            switch (msgType) {
                case INITIALIZE:
                    doInit(sock_fd);
                    break;
                case REGISTER:
                    doRegister(sock_fd);
                    break;
                case LOC_REQUEST:
                    doCall(sock_fd);
                    break;
                case TERMINATE:
                    doTerminate(sock_fd);
                    break;
                default:
                    assert(1 == 0);
                    break;
            }
        }
    }
}

void Binder::deregister(int socket) {
    Server v = socketInfo[socket];
    map< Signature, set<Server> >::iterator it;

    for (it = bindings.begin(); it != bindings.end(); ++it) {
        it->second.erase(v);
    }
    queue.remove(v);
}

void Binder::doCall(int socket) {
    char* name;
    int nameLen;
    int* argTypes;
    int argTypesLen;
    int msgType;
    int reasonCode;

    // 1) receive the length of name (including null terminator) followed by name
    recv_debug(socket, &nameLen, 4, 0);
    name = new char[nameLen];
    recv_debug(socket, name, sizeof(char) * nameLen, 0);

    // 2) receive length of argTypes followed by argTypes
    recv_debug(socket, &argTypesLen, 4, 0);
    argTypes = new int[argTypesLen];
    recv_debug(socket, argTypes, sizeof(int) * argTypesLen, 0);

    // 3) perform lookup
    Signature sig = createSignature(name, argTypes);
    delete [] name;
    delete [] argTypes;

    Server v = getNextServer(sig);
    if (v == make_pair(string("0"), string("0"))) {
        // unable to locate any suitable server
        msgType = LOC_FAILURE;
        reasonCode = SIGNATURE_NOT_FOUND;
        send_debug(socket, &msgType, 4, 0);
        send_debug(socket, &reasonCode, 4, 0);
        close(socket);
        return;
    }

    // 4) send server info back
    msgType = LOC_SUCCESS;
    const char* address = v.first.c_str();
    int addressLen = strlen(address) + 1;
    const char* port = v.second.c_str();
    int portLen = strlen(port) + 1;
    send_debug(socket, &msgType, 4, 0);
    send_debug(socket, &addressLen, 4, 0);
    send_debug(socket, address, sizeof(char) * addressLen, 0);
    send_debug(socket, &portLen, 4, 0);
    send_debug(socket, port, sizeof(char) * portLen, 0);

    // 5) cleanup
    close(socket);
    return;
}

void Binder::doInit(int socket) {
    fds[numSocks].fd = socket;
    fds[numSocks].events = POLLIN;
    numSocks++;
}


void Binder::doRegister(int socket) {

    int error,nameLen,argLen,addressLen,portLen,msg;
    char* name;
    char* address;
    char* port;
    int* argTypes;


    recv_debug(socket,&nameLen,4,0);
    name = new char[nameLen];
    recv_debug(socket,name,nameLen*sizeof(char),0);

    recv_debug(socket,&argLen,4,0);
    argTypes = new int[argLen];
    recv_debug(socket,argTypes,argLen*sizeof(int),0);

    recv_debug(socket,&addressLen,4,0);
    address = new char[addressLen];
    recv_debug(socket,address,addressLen*sizeof(char),0);

    recv_debug(socket,&portLen,4,0);
    port = new char[portLen];
    recv_debug(socket,port,portLen*sizeof(char),0);

    Server v = createServer(string(address), string(port));
    if (initServer(v)) {
        socketInfo.insert(make_pair(socket, v));
    }
    error = registerSignature(v,createSignature(name,argTypes));

    if (!error) {
        msg = REGISTER_FAILURE;
        send_debug(socket,&msg,4,0);
    }

    else {
        msg = REGISTER_SUCCESS;
        send_debug(socket,&msg,4,0);
    }

    delete[] name;
    delete[] argTypes;
    delete[] address;
    delete[] port;
}

void Binder::doTerminate(int socket) {
int msg = TERMINATE;
int size = sizeof(int)*1;

for (int i = 1; i < numSocks; i++){
send_debug(fds[i].fd,&msg,size,0);
}
exit(0);
}

void Binder::print() {
    cout << "===== PRINT =====" << endl;
    cout << "Queue:" << endl;
    list<Server>::iterator it;
    for (it = queue.begin(); it != queue.end(); ++it) {
        cout << *it << endl;
    }
    cout << endl;

    cout << "Bindings:" << endl;
    map< Signature, set<Server> >::iterator jt;
    for (jt = bindings.begin(); jt != bindings.end(); ++jt) {
        cout << "Signature " << jt->first << endl;
        set<Server>::iterator kt;
        for (kt = jt->second.begin(); kt != jt->second.end(); kt++) {
            cout << "  " << *kt << endl;
        }
    }
    cout << endl;
    cout << "=================" << endl;
}

ostream& operator<<(ostream& os, const Server& v) {
    os << "(" << v.first << " " << v.second << ")";
    return os;
}

ostream& operator<<(ostream& os, const Signature& sig) {
    os << sig.first << " ";
    std::vector<int>::const_iterator it;
    for (it = sig.second.begin(); it != sig.second.end(); ++it) {
        os << *it << " ";
    }
    return os;
}

int main() {
    signal(SIGPIPE,SIG_IGN);
    Binder binder;
    binder.run();
}
