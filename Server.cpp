#include "stdafx.h"

#include "stdio.h"
#include "conio.h"
#include "string.h"
#include "ws2tcpip.h"
#include "winsock2.h"
#include "process.h"
#include <stdlib.h>
#pragma comment (lib, "Ws2_32.lib")
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define MAX_CLIENT 640
#define MAX_ACCOUNT 2050
#define MAX_THREAD 10

#define ENDING_DELIMITER "\r\n"

typedef struct Client {
	//SOCKET s;
	char clientIP[INET_ADDRSTRLEN];
	int clientPort;
	char username[BUFF_SIZE];
}Client;


typedef struct Account {
	char userName[BUFF_SIZE];
	int status;

}Account;

CRITICAL_SECTION critical;
Client arrClient[MAX_THREAD][MAX_CLIENT];
Account arrAccount[MAX_ACCOUNT];
char QueueBuff[MAX_THREAD][WSA_MAXIMUM_WAIT_EVENTS][11 * BUFF_SIZE];
SOCKET		socks[MAX_THREAD][WSA_MAXIMUM_WAIT_EVENTS];
WSAEVENT	events[MAX_THREAD][WSA_MAXIMUM_WAIT_EVENTS];
DWORD		nEvents[MAX_THREAD];

int cnt = -1;



void processData(char *buff, char *out, int id, int in) {
	//memcpy(out, in, BUFF_SIZE);
	char Protocol[BUFF_SIZE], ToSendBuff[BUFF_SIZE];
	if (strlen(buff) > 3) {
		strncpy_s(Protocol, buff, 5);
		if (!strcmp(Protocol, "USER ")) {

			if (strcmp(arrClient[id][in].username, "")) {

				strcpy_s(ToSendBuff, "14");
			}
			else {
				char username[BUFF_SIZE];
				memset(username, 0, sizeof(username));
				strncpy_s(username, buff + 5, strlen(buff) - 5);

				int Account_ID;
				for (Account_ID = 0; Account_ID < MAX_ACCOUNT; Account_ID++) {
					if (!strcmp(username, arrAccount[Account_ID].userName)) {
						break;
					}
				}
				if (Account_ID == MAX_ACCOUNT) {
					strcpy_s(ToSendBuff, "12");
				}
				else if (arrAccount[Account_ID].status == 1) strcpy_s(ToSendBuff, "11");
				else {
					int i;
					for (i = 0; i < MAX_CLIENT; i++) {
						if (!strcmp(username, arrClient[id][i].username)) break;
					}
					if (i == MAX_CLIENT) {
						strcpy_s(arrClient[id][in].username, username);
						strcpy_s(ToSendBuff, "10");
					}
					else {
						strcpy_s(ToSendBuff, "13");
					}
				}
			}

		}
		else if (!strcmp(Protocol, "POST ")) {
			if (!strcmp(arrClient[id][in].username, "")) {
				strcpy_s(ToSendBuff, "21");
			}
			else {
				strcpy_s(ToSendBuff, "20");
			}
		}
		else strcpy_s(ToSendBuff, "99");
	}
	else {
		if (strcmp(buff, "BYE") == 0) {
			if (!strcmp(arrClient[id][in].username, "")) {
				strcpy_s(ToSendBuff, "21");
			}
			else {
				strcpy_s(arrClient[id][in].username, "");
				strcpy_s(ToSendBuff, "30");
			}
		}
		else strcpy_s(ToSendBuff, "99");
	}

	memcpy(out, ToSendBuff, BUFF_SIZE);
}


/* The recv() wrapper function */

int Receive(SOCKET s, char *buff, int size, int flags) {
	int n;

	n = recv(s, buff, size, flags);
	if (n == SOCKET_ERROR)
		printf("Error %d: Cannot receive data.\n", WSAGetLastError());
	else if (n == 0)
		printf("Client disconnects.\n");
	return n;
}

/* The send() wrapper function*/
int Send(SOCKET s, char *buff, int size, int flags) {
	int n;

	n = send(s, buff, size, flags);
	if (n == SOCKET_ERROR)
		printf("Error %d: Cannot send data.\n", WSAGetLastError());

	return n;
}

unsigned __stdcall echoThread(void *param) {
	char ToSendBuff[BUFF_SIZE], ToReceiveBuff[BUFF_SIZE];
	int ret;
	DWORD index, time = 100;
	WSANETWORKEVENTS sockEvent;
	int id = (int)param;
	char buff[BUFF_SIZE];
	while (1) {
		//wait for network events on all socket in this thread
		//EnterCriticalSection(&critical);
		memset(ToReceiveBuff, 0, sizeof(ToReceiveBuff));
		memset(ToSendBuff, 0, sizeof(ToSendBuff));
		memset(buff, 0, sizeof(buff));
		index = WSAWaitForMultipleEvents(nEvents[id], events[id], FALSE, time, FALSE);
		//LeaveCriticalSection(&critical);
		if (index == WSA_WAIT_TIMEOUT) continue;
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}

		index = index - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(socks[id][index], events[id][index], &sockEvent);




		if (sockEvent.lNetworkEvents & FD_READ) {
			//Receive message from client
			if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
				printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				break;
			}

			ret = Receive(socks[id][index], ToReceiveBuff, BUFF_SIZE, 0);
			printf("da nhan %s", ToReceiveBuff);
			//Release socket and event if an error occurs
			if (ret <= 0) {
				closesocket(socks[id][index]);
				socks[id][index] = 0;
				WSACloseEvent(events[id][index]);
				nEvents[id]--;

			}
			else {									//echo to client

				ToReceiveBuff[ret] = 0;
				strcat_s(QueueBuff[id][index], ToReceiveBuff);
				char *temp = strstr(QueueBuff[id][index], ENDING_DELIMITER);
				if (!temp) continue;
				else {
					int l1 = strlen(QueueBuff[id][index]), l2 = strlen(temp);
					for (int j = 0; j < l1 - l2; j++) {
						buff[j] = QueueBuff[id][index][j];
					}
					buff[l1 - l2] = 0;
					strcpy_s(QueueBuff[id][index], temp + 2);
					printf("noi dung xu ly:%s\n", buff);
					processData(buff, ToSendBuff, id, index);
					printf("\nnoi dung gui lai %s\n", ToSendBuff);
					Send(socks[id][index], ToSendBuff, strlen(ToSendBuff), 0);
					temp = strstr(QueueBuff[id][index], ENDING_DELIMITER);

				}

				//reset event
				WSAResetEvent(events[id][index]);
			} 

		}


		if (sockEvent.lNetworkEvents & FD_WRITE) {
			char *temp = strstr(QueueBuff[id][index], ENDING_DELIMITER);
			if (!temp) continue;
			else {
				int l1 = strlen(QueueBuff[id][index]), l2 = strlen(temp);
				for (int j = 0; j < l1 - l2; j++) {
					buff[j] = QueueBuff[id][index][j];
				}
				buff[l1 - l2] = 0;
				strcpy_s(QueueBuff[id][index], temp + 2);
				printf("noi dung xu ly:%s\n", buff);
				processData(buff, ToSendBuff, id, index);
				printf("\nnoi dung gui lai %s\n", ToSendBuff);
				Send(socks[id][index], ToSendBuff, strlen(ToSendBuff), 0);
				temp = strstr(QueueBuff[id][index], ENDING_DELIMITER);

			} */

			//reset event
			WSAResetEvent(events[id][index]);
		}
		if (sockEvent.lNetworkEvents & FD_CLOSE) {

			if (sockEvent.iErrorCode[FD_CLOSE_BIT] != 0) {
				printf("FD_CLOSE failed with error %d\n", sockEvent.iErrorCode[FD_CLOSE_BIT]);
				break;
			}


			closesocket(socks[id][index]);
			socks[id][index] = 0;
			WSACloseEvent(events[id][index]);
			nEvents[id]--;
		}

	}

	return 0;
}

int main(int argc, char* argv[])
{
	//const int SERVER_PORT = atoi(argv[1]);
	const int SERVER_PORT = 5500;
	bool check = true;

	FILE *stream;
	char position[BUFF_SIZE];
	memset(position, 0, sizeof(position));

	printf("Enter position of account.txt: ");
	gets_s(position, sizeof(position));
	//errno_t err = fopen_s(&stream, "account.txt", "r");
	errno_t err = fopen_s(&stream, position, "r");
	if (err) {
		printf_s("The file account.txt was not opened\n");
		return 0;
	}
	else
	{


		for (int i = 0; i < MAX_ACCOUNT; i++) {
			fscanf_s(stream, "%s", arrAccount[i].userName, _countof(arrAccount[i].userName));
			fscanf_s(stream, "%d", &arrAccount[i].status);
		}

		// Output data readed:
		printf("List accounts:\n");
		for (int i = 0; i < MAX_ACCOUNT; i++) {
			printf("%s %d\n", arrAccount[i].userName, arrAccount[i].status);
		}

		fclose(stream);
	}



	for (int i = 0; i < MAX_THREAD; i++) {
		for (int j = 0; j < WSA_MAXIMUM_WAIT_EVENTS; j++) {
			socks[i][j] = 0;
			//arrClient[i][j].s = 0;
			strcpy_s(arrClient[i][j].username, "");
		}
		nEvents[i] = 0;
	}

	memset(QueueBuff, 0, sizeof(QueueBuff));

	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	//Step 4: Listen request from client

	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.");
		_getch();
		return 0;
	}

	printf("Server started!\n");

	//Step 5: Communicate with client
	SOCKET connSock;
	sockaddr_in clientAddr;
	char clientIP[INET_ADDRSTRLEN];
	int clientAddrLen = sizeof(clientAddr), clientPort;

	InitializeCriticalSection(&critical);
	while (1) {
		connSock = accept(listenSock, (sockaddr *)& clientAddr, &clientAddrLen);
		if (connSock == SOCKET_ERROR)
			printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
		else {
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
			int i, j, check = false;

			for (i = 0; i < MAX_THREAD; i++) {
				for (j = 0; j < WSA_MAXIMUM_WAIT_EVENTS; j++) {
					EnterCriticalSection(&critical);
					if (socks[i][j] == 0) {
						socks[i][j] = connSock;
						strcpy_s(arrClient[i][j].clientIP, clientIP);
						arrClient[i][j].clientPort = clientPort;
						events[i][j] = WSACreateEvent();
						WSAEventSelect(socks[i][j], events[i][j], FD_READ | FD_CLOSE | FD_WRITE);
						nEvents[i]++;
						check = true;
						break;
					}
					LeaveCriticalSection(&critical);
				}
				EnterCriticalSection(&critical);

				if (i > cnt) {
					cnt = i;
					printf("____________________________\nTao moi thread \n");
					_beginthreadex(0, 0, echoThread, (void *)cnt, 0, 0); //start thread
				}
				LeaveCriticalSection(&critical);
				if (check) break;

			}

			if (i == MAX_THREAD) {
				printf("Error: Cannot response more client.\n");
				closesocket(connSock);
			}

		}
	}
	DeleteCriticalSection(&critical);


	//close socket
	closesocket(listenSock);
	//terminate socket
	WSACleanup();

	return 0;
}