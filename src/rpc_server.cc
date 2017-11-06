#include "common.h"
#include <iostream>
#include <map>
#include <vector>
using namespace std;

typedef map < pair < string,vector < int > >, skeleton> Database; //local database of all functions registered
Database db; 

skeleton getFunction(char* s, int* arr){//returns function fromdatabase
	vector<int> vec;
	string st = string(s);
	int count = 0;

	Database::iterator it;
	vector<int>::iterator jt;



	while(arr[count]!=0){
		vec.push_back(arr[count]);
		count++;
	}
	for (jt = vec.begin(); jt != vec.end(); ++jt) {
		if (ARRAY_LENGTH(*jt) > 0) {
			*jt = *jt & (~0U << 16);
			*jt = *jt | 1;
		}
	}

	it = db.find(make_pair(st,vec));  //dont really know how this behaves, need ot find out
	assert(it != db.end());
	return it->second;
}
bool addFunction(string s, vector<int> v, skeleton f){

if(db.find(make_pair(s,v)) == db.end()){
db.insert(make_pair(make_pair(s,v),f));
return false;
}
else{//return true if already exists
return true;
}




}
//class used to keep track of args passed to it by the client

class serverArg{

	public:
		bool input,output; //this is to tell if the variable is an input or output variable, false by default
		int size;          //array size of payload
		int type; //this is for the type of variable it is
		void* payload; //pointer to data retrieved from args
		int value; //value of argType


		serverArg(){
			input = false;
			output = false;
			size = 0;
			payload = NULL;
		}

		~serverArg(){
			if (payload != NULL){

				switch(type){
					case ARG_CHAR:
						delete [] (char*) payload;
						break;
					case ARG_SHORT:
						delete [] (short*) payload;
						break;
					case ARG_INT:
						delete [] (int*) payload;
						break;
					case ARG_DOUBLE:
						delete [] (double*) payload;
						break;
					case ARG_LONG:
						delete [] (long*) payload;
						break;
					case ARG_FLOAT:
						delete [] (float*) payload;
						break;
				}



			}
		}


		void setUp(const int val){	//this function decodes the argtype integer
			value = val;
			size = ARRAY_LENGTH(val);
			if(size == 0){
				size =1;}
			if((val & (1<<31)) != 0 ){ // check if its an input/output variable
				input = true;
			}
			if((val & (1<<30)) != 0){
				output = true;
			}

			//check the type
			type = DATA_TYPE(val);
		}


};



//class used to hold the servers sockets
class serverSock{
	public:
		static int binderSock, clientSock, isSetUp;
		static struct pollfd fds[2];
		static string address,port;
		serverSock(){}

};
int serverSock::binderSock = 0;
int serverSock::clientSock = 0;
int serverSock::isSetUp = 0;
string serverSock::address;
string serverSock::port;
struct pollfd serverSock::fds[2];

int rpcInit(){
	int msg = INITIALIZE;
	signal(SIGPIPE,SIG_IGN);
	
	serverSock::clientSock = setup(serverSock::address, serverSock::port);


	if(getenv("BINDER_ADDRESS") != NULL){}
	else{
	return BINDER_ADDRESS_NOT_SET;}
	if(getenv("BINDER_PORT") !=NULL){}
	else{
	return BINDER_PORT_NOT_SET;
	}

	string binderAddress = string(getenv("BINDER_ADDRESS"));
	string binderPort = string(getenv("BINDER_PORT"));

	serverSock::binderSock = establish(binderAddress, binderPort);
	if (serverSock::binderSock < 0) {
		return BINDER_CONNECTION_REFUSED;
	}
	
	send_debug(serverSock::binderSock,&msg,4,0);
	
	cout << "SERVER_ADDRESS " << serverSock::address << endl;
	cout << "SERVER_PORT " << serverSock::port << endl;
	
	serverSock::fds[0].fd = serverSock::binderSock;
	serverSock::fds[0].events =POLLIN;
	serverSock::fds[1].fd = serverSock::clientSock;
	serverSock::fds[1].events = POLLIN;

	serverSock::isSetUp = 1;
	return 0;
	//return serverSock::clientSock;// TEST VERSION
}

int rpcRegister(char* name, int* argTypes, skeleton f){
	if(serverSock::isSetUp != 1) {
		return SERVER_NOT_INIT;
	}
	string s = string(name);
	vector<int> vec;
	int countArgs = 0;
	int countChars = 0;
	int countAddress = strlen(serverSock::address.c_str()) + 1;
	int countPort = strlen(serverSock::port.c_str()) + 1;
	int msg = REGISTER;
	int error;
	
	while(argTypes[countArgs]!=0){
		if (ARRAY_LENGTH(argTypes[countArgs]) > 0) {
			argTypes[countArgs] = argTypes[countArgs] & (~0U << 16);
			argTypes[countArgs] = argTypes[countArgs] | 1;
		}
		vec.push_back(argTypes[countArgs]);
		countArgs++;
	}
	countArgs++;

	countChars = strlen(name) + 1;

	
	if(addFunction(s,vec,f)){
	return SERVER_FUNCTION_ALREADY_REGISTERED;
	}

	
	send_debug(serverSock::binderSock,&msg,4,0);	//send register request

	send_debug(serverSock::binderSock,&countChars,4,0);	//send name
	send_debug(serverSock::binderSock,name,countChars*sizeof(char),0);

	send_debug(serverSock::binderSock,&countArgs,4,0);	//send args
	send_debug(serverSock::binderSock,argTypes,countArgs*sizeof(int),0);

	send_debug(serverSock::binderSock,&countAddress,4,0);	//send address
	send_debug(serverSock::binderSock,serverSock::address.c_str(),countAddress*sizeof(char),0);

	send_debug(serverSock::binderSock,&countPort,4,0);	//send port
	send_debug(serverSock::binderSock,serverSock::port.c_str(),countPort*sizeof(char),0);

	recv_debug(serverSock::binderSock,&error,4,0); // get success or failure

	if (error == REGISTER_SUCCESS) {
		return 0;
	}
	else {
		return SERVER_REGISTER_FAILED;
	}

}

int doSomething(int cSock) {
	int msgLen,nameLen,numArgs,msgType,error;
	char* funcName;
	skeleton func;
	serverArg * argL; //array for holding arg information


	recv_debug(cSock,&msgType,4,0);//get length of first component which should be execute
	if(msgType != EXECUTE){
		//do error
	}


	recv_debug(cSock,&nameLen,4,0);//get length of second component, should be name
	funcName = new char[nameLen];
	recv_debug(cSock,funcName,nameLen*sizeof(char),0); // the info recieved from name should be thename of the skeleton

	recv_debug(cSock,&msgLen,4,0);//get length of third component, should be argtypes+1
	numArgs = msgLen;

	argL = new serverArg[numArgs];

	for(int i = 0; i < numArgs; i++){
		recv_debug(cSock,&msgType,4,0);	//decode argtype information, use msgType array cause its already set up.
		argL[i].setUp(msgType); 
	}


	for(int i = 0; i < numArgs-1; i++){ // store args into argL
		int arsize = argL[i].size;
		void* tempAr;
		switch(argL[i].type){
			case ARG_CHAR: //char
				tempAr = new char[arsize];
				recv_debug(cSock,(char*)tempAr,arsize*sizeof (char),0);
				argL[i].payload =  tempAr;
				break;
			case ARG_SHORT: //short
				tempAr = new short[arsize];
				recv_debug(cSock,(short*)tempAr,arsize*sizeof (short),0);
				argL[i].payload =  tempAr;
				break;
			case ARG_INT:// int
				tempAr = new int[arsize];
				recv_debug(cSock,(int*)tempAr,arsize*sizeof (int),0);
				argL[i].payload =  tempAr;
				break;
			case ARG_DOUBLE:// double 
				tempAr = new double[arsize];
				recv_debug(cSock,(double*)tempAr,arsize*sizeof (double),0);
				argL[i].payload =  tempAr;
				break;
			case ARG_LONG:// long
				tempAr = new long[arsize];
				recv_debug(cSock,(long*)tempAr,arsize*sizeof (long),0);
				argL[i].payload =  tempAr;
				break;
			case ARG_FLOAT:// float
				tempAr = new float[arsize];
				recv_debug(cSock,(float*)tempAr,arsize*sizeof (float),0);
				argL[i].payload =  tempAr;
				break;
		}
		//do error
	}

	void *	args[numArgs-1]; 	//these two are passed into the function
	int	argTypes[numArgs];
	argTypes[numArgs - 1] = 0;

	for(int i = 0; i < numArgs-1; i++){ // populate the arrays
		args[i] = argL[i].payload;
		argTypes[i] = argL[i].value;
	}
	
	func = getFunction(funcName,argTypes);
	assert(func != NULL);
	error =func(argTypes,args);	//execute the function

	if(error != 0){
		msgType = EXECUTE_FAILURE;		//do error
		send_debug(cSock,&msgType,4,0);
		send_debug(cSock,&error,4,0); //send failure and error code
	}
	else{
		//start sending stuff back
		msgType = EXECUTE_SUCCESS;
		send_debug(cSock, &msgType,4,0);//type
		send_debug(cSock, &nameLen,4,0);//name length
		send_debug(cSock, funcName,nameLen,0);//name
		send_debug(cSock, &numArgs,4,0);//send number of argtypes
		send_debug(cSock, argTypes,(numArgs) *sizeof(int),0);

		for(int i = 0; i < numArgs-1; i++){	//send args
			int arsize = argL[i].size;
			switch(argL[i].type){
				case ARG_CHAR:
					send_debug(cSock,args[i],sizeof(char)*arsize,0);
					break;
				case ARG_SHORT:
					send_debug(cSock,args[i],sizeof(short)*arsize,0);
					break;
				case ARG_INT:
					send_debug(cSock,args[i],sizeof(int)*arsize,0);
					break;
				case ARG_LONG:
					send_debug(cSock,args[i],sizeof(long)*arsize,0);
					break;
				case ARG_DOUBLE:
					send_debug(cSock,args[i],sizeof(double)*arsize,0);
					break;
				case ARG_FLOAT:
					send_debug(cSock,args[i],sizeof(float)*arsize,0);
					break;

			}

		}
	}
	delete []argL;
	delete []funcName;

	return 0;
}

int rpcExecute() {

	if(serverSock::isSetUp != 1) {
	return SERVER_NOT_INIT;
	}
	if(db.size() <=0){
	return NO_REGISTERED_FUNCTIONS;
	}


	int cSock;
	struct sockaddr_storage clientAddr;
	socklen_t cSize;

	cSize = sizeof clientAddr;

	while(1) {//accept loop

		
		int r = poll(serverSock::fds,2,0);
		if ( r < 0) {
			perror("poll");
			continue;
		}
	
		else {
			if (serverSock::fds[0].revents & POLLIN) {
				int msgType;
				if (recv_debug(serverSock::binderSock,&msgType,4,0) == 0) {
					return BINDER_DISCONNECTED;
				}
				return 0;
			}
			else if(serverSock::fds[1].revents & POLLIN) {

				cSock = accept(serverSock::clientSock, (struct sockaddr *)&clientAddr, &cSize);
				//accept connection

				switch(fork()) {
		    		case -1: // error
		    			close(serverSock::clientSock);
		    			close(cSock);
		    			perror("fork");
		    			exit(-1);
		    		case 0: // child
		    			close(serverSock::clientSock);
		    			doSomething(cSock);
		    			exit(0);
		    		default: // parent
		    			close(cSock);
		    			continue;
		    	}
		    }
	    }
	}
}
