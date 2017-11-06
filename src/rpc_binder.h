#include "common.h"
#include <map>
#include <vector>
#include <set>
#include <string>
#include <list>
#include <utility>
#include <algorithm>
#include <iostream>

using namespace std;

typedef pair< string, vector<int> > Signature;
typedef pair< string, string > Server;

class Binder {
public:
    Binder();
    ~Binder();

    // Returns the Signature from name and argTypes
    Signature createSignature(char* name, int* argTypes);

    // Returns a server with address and port
    Server createServer(string address, string port);    

    // Gets the next Server that can fulfill sig and sends that Server to end of queue
    // Returns make_pair("0", "0") if no server can fulfill sig
    Server getNextServer(Signature sig);

    // Places v at the beginning of queue
    // Returns false if init fails (e.g. Server already exists)
    bool initServer(Server v);

    // Adds an entry for v to bindings[sig]
    // Returns false if registration fails (e.g. Signature already exists for this Server)
    bool registerSignature(Server v, Signature sig);

    // Main loop
    void run();

    void deregister(int socket);

    // Serves rpcCall
    // Close socket when done
    void doCall(int socket);

    // Serves rpcInit
    // Adds socket to fds
    void doInit(int socket);

    // Serves rpcRegister
    void doRegister(int socket);

    // Serves rpcTerminate
    void doTerminate(int socket);

    // Print the contents of bindings and queue, for debugging purposes
    void print();
private:
    map< Signature, set<Server> > bindings;
    list<Server> queue;
    struct pollfd fds[100];
    map<int, Server> socketInfo;
    int numSocks;
};

ostream& operator<<(ostream& os, const Server& v);
ostream& operator<<(ostream& os, const Signature& sig);
