//
// Created by greyk on 4/26/2020.
//

// Client side C/C++ program to demonstrate Socket programming

#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <cassert>
#include <vector>

#define PORT "12111"
#define HOSTNAME "127.0.0.1"
using namespace std;

/**
 * connect RPC. sends connect rpc string to server with username and password
 * username must be 'abc' password must be '123'
 *
 * @param username passed in from main, collected from user input
 * @param password passed in from main, collected from user input
 * @param sock the socket being used to facilitate connection
 * @return the output string which contains the status and an error message if applicable
 */
string connectRPC(string username, string password, int & sock){

    string str = "rpc=connect;user="+ username +";password=" + password +";"; //rpc=connect;user=abc;password=123
    char buffer[1024] = { 0 };
    send(sock, str.c_str(), str.size()+1, 0); //send
    cout <<"Verifying Credentials" << endl;
    fflush(stdout);
    read(sock, buffer, 1024); //read output from server
    const char *output = buffer;
    return output;
}

/**
 * Disconnect RPC sends string to server
 *
 * @return output string for disconnect
 */
string disconnectRPC(int & sock){
    string str = "rpc=disconnect;";
    char buffer[1024] = {0};
    cout <<"Disconnect requested" << endl;
    fflush(stdout);
    send (sock, str.c_str(), str.size()+1, 0);  //send
    read(sock, buffer, 1024); //read output from server
    const char *output = buffer;
    return output;
}

string dateTimeRPC(int & sock){
    string str = "rpc=datetime;";
    char buffer[1024] = {0};
    send (sock, str.c_str(), str.size()+1, 0);
    read(sock, buffer, 1024);
    const char *output = buffer;
    return output;
}

string sendMessageRPC(int & sock){
    string str = "rpc=sendmessage;";
    char buffer[1024] = {0};
    send (sock, str.c_str(), str.size() + 1, 0);
    read(sock, buffer, 1024);
    const char *output = buffer;
    return output;
}

string checkMessageRPC(int & sock){
    string str = "rpc=checkmessage;";
    char buffer[1024] = {0};
    send (sock, str.c_str(), str.size() + 1, 0);
    read(sock, buffer, 1024);
    const char *output = buffer;
    return output;
}

string getFactRPC (int & sock){
    string str = "rpc=getfact;";
    char buffer[1024] = {0};
    send (sock, str.c_str(), str.size() + 1, 0);
    read(sock, buffer, 1024);
    const char *output = buffer;
    return output;
}

/**
 * Parses out string into a vector
 *
 * @param buffer the string to be parsed
 * @return vector example {rpc, connect, user, username, pass, password}
 */
vector <string> parseClient (string buffer){
    vector<string> vec;
    string::size_type pos = string::npos;
    while((pos = buffer.find_first_of("=;")) != string::npos){ //parse through and separate by = or ;
        string str = buffer.substr(0, pos);
        vec.push_back(str);
        buffer.erase(0, pos+1);
    }
    return vec;
}

/**
 * establishes initial connection to server
 *
 */
int connectToServer(char *szHostName, char *szPort, int & sock){
    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    auto port = (uint16_t) atoi(szPort);
    serv_addr.sin_port = htons(port);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){

        cout <<"Socket creation error" << endl;
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, szHostName, &serv_addr.sin_addr) <= 0){

        cout <<"Invalid address/ Address not supported" << endl;
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        cout <<"Connection failed" << endl;
        return -1;
    }
    return 0;
}

bool choose(int input, int & sock){
    bool ret = true;

    vector<string> vec;
    switch(input){
        case 1: {
            cout << "" << endl;
            string verification = sendMessageRPC(sock);
            vec = parseClient(verification);
            if (vec[1] == "1"){
                cout << "Message Sent Successfully" << endl;
            } else if (vec[1] == "-1"){
                cout << vec[3] << endl;
            }
            break;
        }
        case 2: {
            cout << "" << endl;
            string message = checkMessageRPC(sock);
            vec = parseClient(message);
            if (vec[1] == "1"){
                cout << "Inbox: " << vec[3] <<endl;
            } else if (vec[1] == "-1"){
                cout << vec[3] <<endl;
            }
            break;
        }
        case 3:{
            cout << "" << endl;
            string datetimeOutput = dateTimeRPC(sock);
            vec = parseClient(datetimeOutput);
            cout << "The local date and time is: " << vec[3] << endl;
            break;
        }
        case 4: {
            cout << "" << endl;
            string factOutput = getFactRPC(sock);
            vec = parseClient(factOutput);
            if (vec[1] == "1"){
                cout << "Did you know... " << vec[3] <<endl;
            } else if (vec[1] == "-1"){
                cout << vec[3] <<endl;
            }
            break;
        }
        case 5:
            //call DisconnectRPC
            cout << "" << endl;
            string disconnectOutput = disconnectRPC(sock);
            vec = parseClient(disconnectOutput);
            if (vec[0] == "status"){
                if(vec[1] == "1"){
                    close(sock);
                    cout << "Successfully disconnected" << endl;
                } else if (vec[1] == "-1"){
                    cout << vec[3] << endl;
                }
            }

            ret = false;
            break;
    }
    return ret;
}


int main(int argc, char const *argv[]){
    int sock = 0;
    int num = 0;

    if (argc < 3){
        connectToServer((char *) HOSTNAME, (char *) PORT, sock);
    } else{
        connectToServer((char *) argv[1], (char *) argv[2], sock);
    }

    //loop if password is entered incorrectly, allow for input until correct
    do {
        cout << "Enter the username: " << endl; //abc
        string username;
        cin >> username;
        cout << "Enter the password: " << endl; //123
        string password;
        cin >> password;

        //call connectRPC
        string connectOutput = connectRPC(username, password, sock);       //output = "status=1;error="

        vector<string> vec = parseClient(connectOutput); //check vector at index
        if (vec[0] == "status"){
            if(vec[1] == "1"){
                cout <<"Accepted" << endl;
                fflush(stdout);
                num = 1;
            } else if (vec[1] == "-1"){
                num = -1;
                cout << vec[3] << endl;
                cout <<"Try again" << endl;
                fflush(stdout);
            }
        }
        //notepad code
    }while (num == -1);

    cout <<"Waiting 3 seconds.." << endl;
    sleep(3);
    bool cont = true;
    do{
        cout << "" << endl;
        cout <<"What would you like to do?" << endl;
        cout <<"1 = Send Message" << endl;
        cout <<"2 = Check Current Message" << endl;
        cout <<"3 = Check the date and time" << endl;
        cout <<"4 = Fun Fact" << endl;
        cout <<"5 = Disconnect" << endl;
        int input;
        cin >> input;

        cont = choose(input, sock);
    } while(cont);


    //unused code pt 2
    cout <<"Waiting 3 seconds.." << endl;
    sleep(3);
    return 0;
}

