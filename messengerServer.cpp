// Alexander Edwards
// Student ID: 1188902
// CISP 360 - Tuesday 4:30-5:50pm
// This is a simple server for a message sending program
// Help and tutorials from:
// http://beej.us/net2/html/index.html
// http://www.binarytides.com/code-tcp-so
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms740632(v=vs.85).aspx
// http://johnnie.jerrata.com/winsocktutorial/
// https://tangentsoft.net/wskfaq/newbie.html#normalclose
#include <winsock2.h> //This header provides all the necessary definitions, structures, and functions dealing with network sockets
#include <ws2tcpip.h> //Provides functions for get IP addresses
#include <iostream>
#include <cstring>
#include <string>
#include <map> //provides a hashmap for user database
#pragma comment(lib, "Ws2_32.lib") //links to Ws2_32.lib a necessary library for using Windows network sockets.

using namespace std;
#define DEFAULT_PORT 1234
int main()
{
    map<string, string> userIPDatabase; //This will hold username, ip pairs in char arrays
    //initialize winsock
    cout << "Initializing Winsock" << endl;
    WSADATA wsaData; //An object that holds information about Windows sockets in use by the host machine
    if( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 ){ //initializes the wsaData object with Windows Socket implementation details
        cout << "WSAStartup failed" << endl;        //MAKEWORD(2,2) askes for details for Windows Sockets ver 2.2
        return 1;
    }
    //create a socket
    cout << "Creating socket" << endl;
    SOCKET listener = INVALID_SOCKET; //declares and initializes a socket variable
    listener = socket(AF_INET, SOCK_DGRAM, 0); //creating listening socket
    if(listener == INVALID_SOCKET){ //error checking to make sure socket was created 
        cout << "Error creating socket" << endl;
        WSACleanup();
        return 1;
    }
    //bind the socket
    cout << "Binding to socket" << endl;
    struct sockaddr_in serveraddr; //sockaddr_in struct is used to specify an endpoint address for a socket
    serveraddr.sin_family = AF_INET; //declares IPv4 socket type
    serveraddr.sin_addr.s_addr = INADDR_ANY; //auto-fill my IP
    serveraddr.sin_port = htons(DEFAULT_PORT); //sets port and in the right byte order for network
    if ( bind(listener,(struct sockaddr *)&serveraddr, sizeof(serveraddr) ) == SOCKET_ERROR ) { //performs the socket binding and checks that it was successful
        cout << "bind failed" << endl;
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    //listen on the socket for client
    cout << "Server listening on port: " << ntohs(serveraddr.sin_port) << endl;

    //accept a connection from client
    struct sockaddr_in clientaddr; //This will hold connecting
    clientaddr.sin_family = AF_INET; //Indicates client address will be IPv4
    int clientaddrSize = sizeof(clientaddr); //stores size of clientaddr struct so that casting will work appropriately in call to accept
    
    while(true){
        const int bufferSize = 64;
        char cmdBuffer[bufferSize];
        char responseBuffer[bufferSize]; //buffer to hold data for client
        ZeroMemory(cmdBuffer, bufferSize); // fills arrays with zeros
        ZeroMemory(responseBuffer, bufferSize);
        recvfrom(listener, cmdBuffer, bufferSize, 0, (struct sockaddr *) &clientaddr, &clientaddrSize);
        cout << "Client connected [" << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) <<"]\n";
        // receive and send data
        char * token; //used to parse commands from client
		char * nextToken;
        token = strtok_s(cmdBuffer, " ", &nextToken); // begins parsing command
        if( strcmp(token, "reg") == 0){
            token = strtok_s(NULL, "\n", &nextToken);
            cout << "--[Registering User] " << token << " : " << inet_ntoa(clientaddr.sin_addr) << endl; //inet_ntoa produces a string from stored ip addres
            userIPDatabase[token] = inet_ntoa(clientaddr.sin_addr); // stores username and ip in database
        }
        else if( strcmp(token, "query") == 0) {
            token = strtok_s(NULL, "\n", &nextToken);
            cout << "--[Query User] " << token << " -> ";
            map<string,string>::iterator itr;
            itr = userIPDatabase.find(token); //iterator itr points to end() if there is no username IP pair found 
            if(itr != userIPDatabase.end()){
                cout << userIPDatabase[token] << endl;
                strncpy_s(responseBuffer, userIPDatabase[token].c_str(),  bufferSize); // builds response ip for client
            }
            else {
                cout << "Not Found" << endl;
                strncpy_s(responseBuffer,"NF", 2); //if username doesn't have ip associated then NF will be sent to client
            }
            sendto(listener, responseBuffer, bufferSize, 0, (struct sockaddr *) &clientaddr, clientaddrSize);
        }
        else{
            cout << "Invalid request" << endl;
        }

    //disconnect
    
    }
    closesocket(listener); //closes the socket just like closing a file
    WSACleanup(); //properly frees network resources used by the server program
    return 0;
}