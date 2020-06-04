//
// Created by Grey Kumar, Vanessa Duke, and Jason Limfueco on 4/26/2020.
// Server side C/C++ program to demonstrate Socket programming

#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>
#include <fstream>
#include <pthread.h>
#define PORT 12111
using namespace std;

/**
 * class to parse string
 *
 */
class KeyValue{
private:
    char m_szKey[128]{};
    char m_szValue[2048]{};

public:

    KeyValue() = default;
    void setKeyValue(char *pszBuff){
        char *pch1;

        // find out where the "=" sign is, and take everything to the left of the equal for the key
        // go one beyond the = sign, and take everything else
        pch1 = strchr(pszBuff, '=');
        assert(pch1);
        int keyLen = (int)(pch1 - pszBuff);
        strncpy(m_szKey, pszBuff, keyLen);
        m_szKey[keyLen] = 0;
        strcpy(m_szValue, pszBuff + keyLen + 1);
    }

    char *getKey(){
        return m_szKey;
    }

    char *getValue(){
        return m_szValue;
    }

};

/**
 * class to parse string
 *
 */
class RawKeyValueString{
private:

    char m_szRawString[32768];
    int m_currentPosition;
    KeyValue *m_pKeyValue;
    char *m_pch;
public:

    explicit RawKeyValueString(char *szUnformattedString){
        assert(strlen(szUnformattedString));
        strcpy(m_szRawString, szUnformattedString);

        m_pch = m_szRawString;

    }
    ~RawKeyValueString(){
        if (m_pKeyValue)
            delete (m_pKeyValue);
    }

    void getNextKeyValue(KeyValue & keyVal){
        // It will attempt to parse out part of the string all the way up to the ";", it will then create a new KeyValue object  with that partial string
        // If it can;t it will return null;
        char *pch1;
        char szTemp[32768];

        pch1 = strchr(m_pch, ';');
        assert(pch1 != NULL);
        int subStringSize = (int)(pch1 - m_pch);
        strncpy(szTemp, m_pch, subStringSize);
        szTemp[subStringSize] = 0;
        m_pch = pch1 + 1;
        if (m_pKeyValue)
            delete (m_pKeyValue);
        keyVal.setKeyValue(szTemp);

    }

};


/**
 * Global object for the server
 * Accessed by all clients
 * manages number of users and file handling
 *
 */
class serverInformation{
private:
    int sock;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    volatile int numUsers;
    vector<string> greetings;   //holds the greetings read from file
    vector<string> facts;   //holes the facts read from file
    string messageQueue;    //current message in the queue/Inbox
    string currentFact;     //current fact in the queue

    /**
     * generate ranodm number
     *
     * @return rand number
     */
    int getRand(){
        srand(time(NULL));
        int num = rand() % greetings.size();
        return num;
    }

public:
    serverInformation(){
        sock = 0;
        numUsers = 0;

    }
    /**
     * assign current socket from client recently connected
     *
     * @param s the socket
     */
    void assignSocket(int s){
        this->sock = s;
    }

    /**
     * returns the socket for the current client
     *
     * @return the socket
     */
    int getSocket(){
        return sock;
    }

    /**
     * Increases the number of users when a new client is connected
     *
     */
    void addNewUser(){
        pthread_mutex_lock(&lock);
        numUsers++;
        pthread_mutex_unlock(&lock);
    }

    /**
     *
     * @return the number of connected users
     */
    int getUserNumber(){
        return numUsers;
    }

    /**
     * Decreases number of users when a client disconnects
     *
     * @return the number of users
     */
    int userDisconnected(){
        //lock
        if (numUsers > 0) {
            pthread_mutex_lock(&lock);
            numUsers--;
            pthread_mutex_unlock(&lock);
        }
        return numUsers;
    }

    /**
     * reads the greetings file
     *
     */
    void readGreetings(){
        string line;
        ifstream myfile;
        myfile.open ("greetings.txt");
        if (myfile.is_open())
        {
            while ( getline (myfile,line) )
            {
               greetings.push_back(line);   //add to greetings vector
            }
            myfile.close();
        }
    }

    /**
     * reads the facts file
     *
     */
    void readFacts(){
        string line;
        ifstream myfile;
        myfile.open ("facts.txt");
        if (myfile.is_open())
        {
            while ( getline (myfile,line) )
            {
                facts.push_back(line);  //add to facts vector
            }
            myfile.close();
        }
    }

    /**
     * gets a random number then sets the message in the inbox to the rand index in greetings
     *
     * @return true if message is set
     */
    bool setMessage(){
        int num = getRand();
        messageQueue = greetings[num];
        return true;
    }

    /**
     *
     * @return the current message in the inbox
     */
    string getMessage(){
        if (messageQueue.empty()){

            return "-1";
        }
        return messageQueue;
    }

    /**
     * gets a random number then sets the fact based on the rand index in facts vector
     *
     */
    void setFact(){
        int num = getRand();
        currentFact = facts[num];

    }

    /**
     *
     * @return the current fact
     */
    string getFact(){
        if (currentFact.empty()){

            return "-1";
        }
        return currentFact;
    }

};

/**
 * Local object that stores information for each client connected
 *
 */
class clientInformation{
private:
    string username;
    int socketNum;
public:
    clientInformation(){}

    /**
     *assigns sock param to socketNum
     *
     * @param s the socket for the client
     */
    void assignSocket(int s){
        this->socketNum = s;
    }

    /**
     * assigns user param to username
     *
     * @param user the username of the client
     */
    void assignUsername(string user){
        this->username = user;
    }

    /**
     *
     * @return the username of the client
     */
    string getUsername(){
        return username;
    }
};

/**
 * Class to perform each of the RPC functions
 *
 */
class DoRpcs{
public:
    ~DoRpcs()= default;

    /**
    * server side connect Rpc
    * checks to see if username = abc and password = 123
    * @param username the username of the client
    *  @param password the password of the client
    * @return 1 if valid credentials
    */
    int connect (string username, string password, serverInformation *sharedData){
        string passTest = "123";
        int value = 1;
        cout << "Connecting..." << endl;
        //check if password is valid
        if (password.length() == passTest.length()){
            for( int i = 0; i < passTest.length(); i++){
                if (password.at(i) != passTest.at(i)){
                    value = -1;
                }
            }
        } else {
            value = -1;
        }

        if (value == 1){
            sharedData->addNewUser();
            cout <<"Credentials accepted. " << username <<" Connected. Number of users: "<<
            sharedData->getUserNumber() << endl;

            cout << ""<< endl;
        } else {
            cout <<"Not connected" << endl;
        }

        return value;
    }

    /**
     * server side disconnect rpc
     *
     * @return disconnect output
     */
    static string disconnect(serverInformation *sharedData){
        string str = "status=1;error= ;";
        cout<<"Disconnecting.." << endl;
        sharedData->userDisconnected(); //decrease number of users
        return str;
    }

    /**
     * calculates the current date and time from the system
     *
     * @return the date and time in a string format
     */
    static string datetime(){
        // current date/time based on current system
        time_t now = time(0);
        // convert now to string form
        char* dt = ctime(&now);

        string newdt = string (dt);
        newdt.push_back(';');

        string str = (string)"status=1;message=" + newdt + ";";
        return str;
    }

    /**
     * sends a message to the inbox, calls global setMessage function
     *
     * @param sharedData the global serverInformation object
     * @return message in string format
     */
    static string sendMessage(serverInformation *sharedData){
        bool sent = sharedData->setMessage();   //set message in inbox
        cout <<"New Message in Inbox"<< endl;
        string str;
        if (sent){
            str = "status=1;;";
        } else{
            str = "status=-1;error=Message Not Sent;";
        }
        return str;
    }

    /**
     * checks the current message in the inbox
     *
     * @param sharedData the global serverInformation Object
     * @return message in string format
     */
    static string checkMessage(serverInformation *sharedData){
        string message = sharedData->getMessage();  //get message from inbox
        string str;
        if (message == "-1"){
            str = "status=-1;error=No Messages;;";
        } else {
            message.push_back(';');
            str = "status=1;message=" + message + ";";
        }
        return str;
    }

    /**
     * gets the current fact from the global serverInfo object
     *
     * @param sharedData the global serverInformation object
     * @return message in string format
     */
    static string getFact(serverInformation *sharedData){
        sharedData->setFact();  //set fact
        string message = sharedData->getFact();     //get fact
        string str;
        if (message == "-1"){
            str = "status=-1;error=No Fact Available;;";
        } else {
            message.push_back(';');
            str = "status=1;message=" + message + ";";
        }
        return str;

    }

};


/**
 * Class for the server, initializes the server and creates the threads
 *
 */
class Server{
private:
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int m_port;
    int totalConn;
    serverInformation *serverInfo;  //global server object

public:
    Server(int nPort){
        m_port = nPort;
        totalConn = 10;
    }

    ~Server(){}

    /**
     * starts the server
     *
     * @return 0
     */
    int startServer(){
        int opt = 1;
        serverInfo = new serverInformation();

       // cout <<"Hello world" << endl;
        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
            perror("socket failed");exit(EXIT_FAILURE);
        }

        //cout <<"Got Socket" << endl;
        // Forcefully attaching socket to the port 8080
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEADDR,&opt, sizeof(opt))){
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);
        // Forcefully attaching socket to the port

        cout <<"About to bind" << endl;
        cout <<"Server started" << endl;
        if (bind(server_fd, (struct sockaddr *)&address,sizeof(address)) < 0){
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        if (listen(server_fd, 3) < 0){
            perror("listen");
            exit(EXIT_FAILURE);
        }

        serverInfo->readGreetings();
        serverInfo->readFacts();

        return 0;
    }

    // Socket returned from function is going to be specific to the client that is connecting to us

    /**
     * Accepts a new connection from an incoming client
     *
     * @return the socket number for the client
     */
    int acceptNewConnection(){
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0){
            perror("accept");
            return (-1);
        }
        return new_socket;
    }

    /**
     * Create a new thread for each incoming client connection
     *
     * @return 0
     */
    int threadFunc(){
        auto *threads = new pthread_t[totalConn];
        int i = 0;
        while (true){
            int newSocket = acceptNewConnection();   //get incoming connection
            serverInfo->assignSocket(newSocket);
            pthread_create(&threads[i], NULL, rpcFunc, (void *)serverInfo);     //create thread for connection
            i = (i+1) % totalConn;
        }
        delete [] threads;
        delete serverInfo;
        return 0;
    }

    /**
     * prints the incoming rpc call from a client
     *
     * @param name the username
     * @param buffer the rpc call message
     * @return message in string format
     */
    static string incomingMessage(string name, char buffer[1024]){
        //print buffer
        string str = "Received RPC call from client " + name + ": " + buffer + "\n";
        return str;
    }


    /**
     * manages rpc calls from a connected client
     *
     * @param arg the global server object
     * @return NULL
     */
    static void *rpcFunc(void *arg){
        int new_socket;
        int valread;
        char buffer[1024] = {0};
        srand(time(NULL));
        int clientNum = rand() % 100 + 1;
        serverInformation *pSharedData = (serverInformation *)arg;

        clientInformation clientInfo;
        new_socket = pSharedData->getSocket();  //get socket number for client

        DoRpcs doRpcs;

        //read input from client
        while ((valread = read(new_socket, buffer, 1024)) != 0) {
            //parse string
            auto *pRawKey = new RawKeyValueString((char *)buffer);
            KeyValue rpcKeyValue;
            char *pszRpcKey;
            char *pszRpcValue;

            // Figure out which rpc it is
            pRawKey->getNextKeyValue(rpcKeyValue); //rpc=connect; //rpc=disconnect;
            pszRpcKey = rpcKeyValue.getKey(); //rpc  //rpc
            pszRpcValue = rpcKeyValue.getValue();       //connect        //disconnect

            if (strcmp(pszRpcKey, "rpc") == 0)
            {
                if (strcmp(pszRpcValue, "connect") == 0)
                {
                    // Get the next two arguments (user and password);
                    KeyValue userKeyValue;
                    KeyValue passKeyValue;

                    char *pszUserValue;
                    char *pszPassValue;
                    int status;

                    pRawKey->getNextKeyValue(userKeyValue);
                    pszUserValue = userKeyValue.getValue();        //abc

                    pRawKey->getNextKeyValue(passKeyValue);
                    pszPassValue = passKeyValue.getValue();         //123
                    string str; //output

                    clientInfo.assignSocket(new_socket);
                    clientInfo.assignUsername(pszUserValue);

                    cout << incomingMessage(clientInfo.getUsername(), buffer);

                    status = doRpcs.connect(pszUserValue, pszPassValue, pSharedData);

                    if (status == 1){
                        str = "status=1;error= ;";
                    } else if (status == -1){
                        str = "status=-1;error=Invalid credentials;";
                    }
                    //send output back to client
                    send(new_socket, str.c_str(), str.size()+1, 0);

                } else if (strcmp(pszRpcValue, "disconnect") == 0){
                    //print incoming rpc call
                    cout << incomingMessage(clientInfo.getUsername(), buffer);
                    string message = DoRpcs::disconnect(pSharedData);
                    //send message back to client
                    send(new_socket, message.c_str(), message.size()+1, 0);
                    cout <<"Client "<< clientInfo.getUsername() <<" disconnected. " << "Number of users: " <<
                    pSharedData->getUserNumber() <<endl;
                    cout << " " << endl;
                } else if (strcmp(pszRpcValue, "datetime") == 0){
                    //print incoming rpc call
                    cout << incomingMessage(clientInfo.getUsername(), buffer);
                    string message = DoRpcs::datetime();
                    //send message back to client
                    send(new_socket, message.c_str(), message.size()-1, 0);
                } else if (strcmp(pszRpcValue, "sendmessage") == 0) {
                    //print incoming rpc call
                    cout << incomingMessage(clientInfo.getUsername(), buffer);
                    string message = DoRpcs::sendMessage(pSharedData);
                    //send message back to client
                    send(new_socket, message.c_str(), message.size() - 1, 0);
                } else if (strcmp(pszRpcValue, "checkmessage") == 0) {
                    //print incoming rpc call
                    cout << incomingMessage(clientInfo.getUsername(), buffer);
                    string message = DoRpcs::checkMessage(pSharedData);
                    //send message back to client
                    send(new_socket, message.c_str(), message.size() - 1, 0);
                } else if (strcmp(pszRpcValue, "getfact") == 0) {
                    //print incoming rpc call
                    cout << incomingMessage(clientInfo.getUsername(), buffer);
                    string message = DoRpcs::getFact(pSharedData);
                    //send message back to client
                    send(new_socket, message.c_str(), message.size() - 1, 0);
                }
            }
            delete(pRawKey);
        }

        return NULL;
    }


    /**
     * closes the server
     *
     * @return 0
     */
    int closeServer(){
        return 0;
    }

};



int main(int argc, char const *argv[]){
    //server setup
    int nPort;

    if (argc < 3){
         nPort = atoi((char const  *)"12111");
    }else {
         nPort = atoi((char const  *)argv[1]);
    }

    Server *serverObj = new Server(nPort);

    serverObj->startServer();
    //wait for new socket
    cout <<"Waiting" << endl;
    serverObj->threadFunc();

    delete serverObj;

    return 0;
}


