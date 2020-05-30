//
// Created by greyk on 4/26/2020.
//
//#define NDEBUG

#include <cstdio>
// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <cstdio>
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


//#define PORT 8080
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

class serverInformation{
private:
    int sock;
    int numUsers;
    vector<string> greetings;
    string messageQueue;

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
    void assignSocket(int s){
        this->sock = s;
    }
    int getSocket(){
        return sock;
    }
    int getUserNumber(){
        numUsers++;
        return numUsers;
    }
    int userDisconnected(){
        if (numUsers > 0) {
            numUsers--;
        }
        return numUsers;
    }

    void readGreetings(){
        string line;
        ifstream myfile;
        myfile.open ("greetings.txt");
        if (myfile.is_open())
        {
            while ( getline (myfile,line) )
            {
               greetings.push_back(line);
            }
            myfile.close();
        }

    }

    bool setMessage(){
        int num = getRand();
        messageQueue = greetings[num];
        return true;
    }

    string getMessage(){
        if (messageQueue.empty()){

            return "-1";
        }
        return messageQueue;
    }

};


class Server{
private:
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int m_port;
    int totalConn;
    serverInformation *serverInfo;

public:
    Server(int nPort){
        m_port = nPort;
        totalConn = 10;
    }

    ~Server(){}

    int startServer(){
        int opt = 1;
        serverInfo = new serverInformation();

        cout <<"Hello world" << endl;
        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
            perror("socket failed");exit(EXIT_FAILURE);
        }

        cout <<"Got Socket" << endl;
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
        if (bind(server_fd, (struct sockaddr *)&address,sizeof(address)) < 0){
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        if (listen(server_fd, 3) < 0){
            perror("listen");
            exit(EXIT_FAILURE);
        }

        serverInfo->readGreetings();

        return 0;
    }

    // Socket returned from function is going to be specific to the client that is connecting to us

    int acceptNewConnection(){
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0){
            perror("accept");
            return (-1);
        }
        return new_socket;
    }

    int threadFunc(){
        auto *threads = new pthread_t[totalConn];
        int i = 0;
        while (true){
            int newSocket = acceptNewConnection();
            serverInfo->assignSocket(newSocket);
            pthread_create(&threads[i], NULL, rpcFunc, (void *)serverInfo);
            i = (i+1) % totalConn;
        }
        delete [] threads;
        return 0;
    }

    /**
 * server side connect Rpc
 * checks to see if username = abc and password = 123
 * @param username
 *  @param password
 * @return
 */
    static int connect (string username, string password, serverInformation *sharedData){
        string userTest = "abc";
        string passTest = "123";
        int value = 1;
        cout << "Connecting..." << endl;
        if (username.length() == userTest.length() && password.length() == passTest.length()){
            for( int i = 0; i < userTest.length(); i++){
                if (username.at(i) != userTest.at(i)){
                    value = -1;
                }
                if (password.at(i) != passTest.at(i)){
                    value = -1;
                }
            }
        } else {
            value = -1;
        }

        if (value == 1){
            cout <<"Credentials accepted. Connected. Number of users: "<< sharedData->getUserNumber() << endl;
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
        sharedData->userDisconnected();
        return str;
    }

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

    static string sendMessage(serverInformation *sharedData){
        bool sent = sharedData->setMessage();
        cout <<"Sending message"<< endl;
        string str;
        if (sent){
            str = "status=1;;";
        } else{
            str = "status=-1;error=Message Not Sent;";
        }
        return str;
    }

    static string checkMessage(serverInformation *sharedData){
        string message = sharedData->getMessage();
        string str;
        if (message == "-1"){
            str = "status=-1;error=No Messages;;";
        } else {
            message.push_back(';');
            str = "status=1;message=" + message + ";";
        }
        return str;
    }

    static void *rpcFunc(void *arg){
        int new_socket;
        int valread;
        char buffer[1024] = {0};
        srand(time(0));
        int clientNum = rand() % 100 + 1;

        serverInformation *pSharedData = (serverInformation *)arg;
        new_socket = pSharedData->getSocket();

        while ((valread = read(new_socket, buffer, 1024)) != 0) {

             //rpc=connect;user=abc;password=123 //rpc=disconnect;
            cout << "Received RPC call from client number "<< clientNum << ": "<< buffer << endl;      //print buffer
            fflush(stdout);

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
                    status = connect(pszUserValue, pszPassValue, pSharedData);
                    if (status == 1){
                        str = "status=1;error= ;";
                    } else if (status == -1){
                        str = "status=-1;error=Invalid credentials;";
                    }
                    //send output back to client
                    send(new_socket, str.c_str(), str.size()+1, 0);

                } else if (strcmp(pszRpcValue, "disconnect") == 0){
                    string message = disconnect(pSharedData);
                    send(new_socket, message.c_str(), message.size()+1, 0);
                    cout <<"Client number "<< clientNum <<" disconnected." <<endl;
                    cout << " " << endl;
                } else if (strcmp(pszRpcValue, "datetime") == 0){
                    string message = datetime();
                    send(new_socket, message.c_str(), message.size()-1, 0);
                } else if (strcmp(pszRpcValue, "sendmessage") == 0) {
                    string message = sendMessage(pSharedData);
                    send(new_socket, message.c_str(), message.size() - 1, 0);
                } else if (strcmp(pszRpcValue, "checkmessage") == 0) {
                    string message = checkMessage(pSharedData);
                    send(new_socket, message.c_str(), message.size() - 1, 0);
                }
            }

        }
        return NULL;
    }


    int closeServer(){
        return 0;
    }

};




int main(int argc, char const *argv[]){
    //server setup
    int server_fd, new_socket = 0;

    int nPort = atoi((char const  *)argv[1]);
    Server *serverObj = new Server(nPort);

    serverObj->startServer();
    //wait for new socket
    cout <<"Waiting" << endl;
    serverObj->threadFunc();
    //int count = 0;

    //while true to constantly read rpcs from the client

    return 0;
}


