// Alexander Edwards
// Student ID: 1188902
// CISP 360 - Tuesday 4:30-5:50pm
// This is the client for a simple message sending program

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")
using std::cout;
using std::endl;
using std::string;
using std::cin;
using std::vector;
using std::thread;

void setUsername(string &);
void configServer(struct sockaddr_in &);
void registerUser(string &, struct sockaddr_in &, SOCKET &, int);
void sendMessage(struct sockaddr_in &, SOCKET &, int, string&);
void displayMessages(vector<string>&);
void messageReceivingWorker(vector<string>&, bool &);
#define DEFAULT_PORT 1234
#define CLIENT_PORT 4321

int main()
{
	vector<string> messages; //this holds a the messages sent to the user
	//initialize winsock
	WSADATA wsaData; //An object that holds information about Windows sockets in use by the host machine
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){ //initializes the wsaData object with Windows Socket implementation details
		cout << "WSAStartup failed" << endl;        //MAKEWORD(2,2) askes for details for Windows Sockets ver 2.2
		return 1;
	}
	//creating socket
	SOCKET serverSocket; //declares and initializes a socket variable
	serverSocket = INVALID_SOCKET;
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //creating listening socket
	if (serverSocket == INVALID_SOCKET){ //error checking to make sure socket was created
		cout << "Error creating socket" << endl;
		WSACleanup();
		return 1;
	}
	//setting up username and server info and connecting
	string username;
	struct sockaddr_in serveraddr; //sockaddr_in struct is used to specify an endpoint address for a socket
	int serveraddrSize = sizeof(serveraddr);
	setUsername(username);
	configServer(serveraddr);
	registerUser(username, serveraddr, serverSocket, serveraddrSize);
	bool continueFlag = true;
	thread receivingMessagesThread(messageReceivingWorker, std::ref(messages), std::ref(continueFlag)); //creates a thread that runs the message receiving function

	int choice;
	do{
		cout << "\n\n" << username << ", how would you like to proceed?" << endl
			<< "1. Send a message to a user" << endl
			<< "2. Read your messages" << endl
			<< "3. Quit" << endl
			<< "Enter your choice: ";
		cin >> choice;
		cin.ignore();
		switch (choice){
		case 1: sendMessage(serveraddr, serverSocket, serveraddrSize, username); break;
		case 2: displayMessages(messages); break;
		case 3: continueFlag = false; break;
		default:; break;
		}
	} while (choice != 3);
	WSACleanup(); //properly frees network resources used by the server program
	return 0;
}
void setUsername(string &uname)
{
	cout << "Enter A username: ";
	getline(cin,uname);
}
void configServer(struct sockaddr_in &server)
{
	string serverIP;
	cout << "Enter server IP (example: 165.196.204.170): ";
	cin >> serverIP;
	server.sin_family = AF_INET;
	server.sin_port = htons(DEFAULT_PORT);
	server.sin_addr.s_addr = inet_addr(serverIP.c_str());
}
void registerUser(string &uname, struct sockaddr_in &serveraddr, SOCKET &serverSocket, int serveraddrSize)
{
	const int cmdBufferSize = 64;
	char cmdBuffer[cmdBufferSize];
	ZeroMemory(cmdBuffer, cmdBufferSize); //clears the buffer of any data
	strncpy_s(cmdBuffer, "reg ", 4); //following build the command to send to the server
	strncat_s(cmdBuffer, uname.c_str(), cmdBufferSize - sizeof("reg "));
	sendto(serverSocket, cmdBuffer, cmdBufferSize, 0, (struct sockaddr *) &serveraddr, serveraddrSize); //sends to the server
}
void sendMessage(struct sockaddr_in &serveraddr, SOCKET &serverSocket, int serveraddrSize, string &uname)
{
	const int bufferSize = 64;
	const int messageBufferSize = 128;
	char messageBuffer[messageBufferSize];
	char mbuf[messageBufferSize];
	char cmdBuffer[bufferSize];
	char serverResponseBuffer[bufferSize];
	const int usernameBufferSize = bufferSize - sizeof("query ");
	char recipientUsername[usernameBufferSize];
	ZeroMemory(recipientUsername, usernameBufferSize);
	cout << "Which user would you like to send too? ";
	cin.getline(recipientUsername, usernameBufferSize);
	ZeroMemory(cmdBuffer, bufferSize);
	strncpy_s(cmdBuffer, "query ", 6);
	strncat_s(cmdBuffer, recipientUsername, usernameBufferSize);
	sendto(serverSocket, cmdBuffer, bufferSize, 0, (struct sockaddr *) &serveraddr, serveraddrSize);
	recvfrom(serverSocket, serverResponseBuffer, bufferSize, 0, (struct sockaddr *) &serveraddr, &serveraddrSize);
	if (strcmp(serverResponseBuffer, "NF") != 0){
		struct sockaddr_in recipientaddr;
		int recipientaddrSize = sizeof(recipientaddr);
		recipientaddr.sin_family = AF_INET;
		recipientaddr.sin_addr.s_addr = inet_addr(serverResponseBuffer);
		recipientaddr.sin_port = htons(CLIENT_PORT);
		cout << "Begin writing your message: ";
		cin.getline(mbuf, messageBufferSize);
		strncpy_s(messageBuffer, uname.c_str(), sizeof(uname.c_str()));
		strncat_s(messageBuffer, ": ", 2);
		strncat_s(messageBuffer, mbuf, 100);
		sendto(serverSocket, messageBuffer, messageBufferSize, 0, (struct sockaddr *) &recipientaddr, recipientaddrSize);
	}
	else{
		cout << "User doesn't exist." << endl;
	}
}

void messageReceivingWorker(vector<string> &m, bool &cont)
{
	SOCKET rSocket;
	rSocket = INVALID_SOCKET;
	rSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (rSocket == INVALID_SOCKET){
		cout << "Error creating socket" << endl;
		WSACleanup();
	}
	struct sockaddr_in rServer;
	rServer.sin_family = AF_INET;
	rServer.sin_addr.s_addr = INADDR_ANY;
	rServer.sin_port = htons(CLIENT_PORT);
	if ((bind(rSocket, (struct sockaddr *) &rServer, sizeof(rServer))) == SOCKET_ERROR){
		cout << "bind failed" << endl;
		closesocket(rSocket);
		WSACleanup();
	}
	struct timeval tv;
	int numOfSeconds = 5;
	setsockopt(rSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *) numOfSeconds, sizeof(numOfSeconds));
	const int messageBufferSize = 128;
	char messageBuffer[messageBufferSize];
	struct sockaddr_in senderaddr;
	senderaddr.sin_family = AF_INET;
	int senderaddrSize = sizeof(senderaddr);
	while (cont){
		ZeroMemory(messageBuffer, messageBufferSize);
		recvfrom(rSocket, messageBuffer, messageBufferSize, 0, (struct sockaddr *) &senderaddr, &senderaddrSize);
		m.push_back(messageBuffer);
	}
	closesocket(rSocket);
}

void displayMessages(vector<string> &m)
{
	cout << endl;
	cout << "Inbox:\n----------------------------------------------\n";
	for (int i = 0; i < m.size(); i++){
		cout << m[i] << endl;
	}
	cout << "\n----------------------------------------------\n";
}