// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "string.h"
#include "stdlib.h"
#include "winsock2.h"
#include "ws2tcpip.h"

#define ENDING_DELIMITER "\r\n"
#define BUFF_SIZE 2048
#define ENDING_DELIMITER "\r\n"
#pragma comment(lib,"Ws2_32.lib")


int main(int argc, char* argv[])
{
	int isLogin = 0;
	const int SERVER_PORT = atoi(argv[2]);
	const char* SERVER_ADDR = argv[1];
	//Step 1: Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}
	//Step 2: Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	// Set time-out for receiving
	int tv = 10000;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return 0;
	}
	printf("Connected server!\n");

	//Step 5: Communicate with server
	char sbuff[BUFF_SIZE], rbuff[BUFF_SIZE], content[BUFF_SIZE];
	int ret, messageLen;
	while (1) {
		memset(sbuff, 0, sizeof(sbuff));
		memset(rbuff, 0, sizeof(rbuff));
		memset(content, 0, sizeof(content));
		// Process login
		if (!isLogin) {
			printf("Enter your account: ");
			gets_s(content, sizeof(content));
			if (strlen(content) > 0) {
				strcpy_s(sbuff, "USER ");
				strcat_s(sbuff, content);
			}
			else {
				break;
			}
		}
		else {
			printf("Enter 1 to POST, Enter 2 to Logout: ");
			char buff[BUFF_SIZE];
			memset(buff, 0, sizeof(buff));
			gets_s(buff);
			//Process Post
			if (!strcmp(buff, "1")) {
				printf("Enter your content: ");
				gets_s(content, sizeof(content));
				strcpy_s(sbuff, "POST ");
				strcat_s(sbuff, content);
			}
			//Process logout
			else if (!strcmp(buff, "2")) {
				strcpy_s(sbuff, "BYE");
			}
			else if (strlen(buff) == 0) break;
			else {
				printf("Invalid syntax\n");
				continue;
			}
		}
		strcat_s(sbuff, ENDING_DELIMITER);
		ret = send(client, sbuff, strlen(sbuff), 0);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot send data.", WSAGetLastError());
		}

		ret = recv(client, rbuff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				printf("Time-out!");
			}
			else printf("Error %d: Cannot receive data\n", WSAGetLastError());
		}
		else if (strlen(rbuff) > 0) {
			rbuff[ret] = 0;


			if (!strcmp(rbuff, "10")) isLogin = 1;
			if (!strcmp(rbuff, "30")) isLogin = 0;
			char res[BUFF_SIZE];
			memset(res, BUFF_SIZE, 0);
			if (!strcmp(rbuff, "10")) strcpy_s(res, "Login successful");
			else if (!strcmp(rbuff, "11")) strcpy_s(res, "Account is locked");
			else if (!strcmp(rbuff, "12")) strcpy_s(res, "Account is not existed");
			else if (!strcmp(rbuff, "13")) strcpy_s(res, "Login fail! Account has logined");
			else if (!strcmp(rbuff, "14")) strcpy_s(res, "Login fail! You have to logout first");
			else if (!strcmp(rbuff, "20")) strcpy_s(res, "POST successful");
			else if (!strcmp(rbuff, "21")) strcpy_s(res, "You haven't login");
			else if (!strcmp(rbuff, "30")) strcpy_s(res, "Logout successful");
			else if (!strcmp(rbuff, "99")) strcpy_s(res, "Invalid syntax!");
			printf("Receive from server: %s\n", res);

		}


	}
	//Step 6: Close socket
	closesocket(client);
	//Step 7: Terminate Winsock
	WSACleanup();


	return 0;
}





