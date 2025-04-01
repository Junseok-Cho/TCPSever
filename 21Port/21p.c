#include<mongoc/mongoc.h>
#include<bson/bson.h>
#include<winsock2.h>
#include<string.h>
#include<ws2tcpip.h>
#include<stdio.h>

WSADATA wsaData;

struct addrinfo* result = NULL, * ptr = NULL, hints;
#define PORT_NUM "5001"
char buff[10] = "Hello!\x1A";
char recvbuff[202] = { 0 };
char offbuf[100] = { 0 };
int iresult;
int socketoff;
int Total = 0;

float dist;
int temp, humi;
int rssi;
float temperror, humierror, disterror;

int main()
{
	mongoc_uri_t* uri = NULL;
	mongoc_client_t* client = NULL;
	mongoc_database_t* database = NULL;
	mongoc_collection_t* collection = NULL; 
	bson_t* ping = NULL, reply = BSON_INITIALIZER;
	bson_error_t error;

	mongoc_init();

		client = mongoc_client_new("mongodb://127.0.0.1:27017");
		database = mongoc_client_get_database(client, "loger");
		ping = BCON_NEW("ping", BCON_INT32(1));
		collection = mongoc_client_get_collection(client, "loger", "Data");

		if (!mongoc_client_command_simple(client, "Dataloger", ping, NULL, &reply, &error))
		{
			fprintf(stderr, "%s\n", error.message);

			bson_destroy(&reply);
			bson_destroy(ping);
			mongoc_database_destroy(database);
			mongoc_client_destroy(client);
			mongoc_uri_destroy(uri);
			mongoc_collection_destroy(collection);
			mongoc_cleanup();

			return 1;
		}
		printf("Server Start\n");

		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0)
		{
			printf("WSAtartup faild: %d\n", iResult);
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		iResult = getaddrinfo(NULL, PORT_NUM, &hints, &result);

		if (iResult != 0)
		{
			printf("getaddrinfo failed: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		SOCKET ListenSocket = INVALID_SOCKET;
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

		if (ListenSocket == INVALID_SOCKET)
		{
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			printf("Listen faild with error: %ld\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		SOCKET ClientSocket = INVALID_SOCKET;

		ClientSocket = accept(ListenSocket, NULL, NULL);

 		if (ClientSocket == INVALID_SOCKET)
		{
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		recvbuff[iresult] = '\0';

		int keepalive = 1;
		int keepidle = 660;
	    int keepintvl = 5;
		int keepcnt = 5;

		setsockopt(ClientSocket, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive, sizeof(keepalive));
		setsockopt(ClientSocket, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepidle, sizeof(keepidle));
		setsockopt(ClientSocket, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&keepintvl, sizeof(keepintvl));
		setsockopt(ClientSocket, IPPROTO_TCP, TCP_KEEPCNT, (void*)&keepcnt, sizeof(keepcnt));
		
		while (1)
		{
			do
			{
				iresult = recv(ClientSocket, recvbuff, 202, 0);

				if (iresult > 0)
				{
					break;
				}
				else if (iresult == 0)
				{
					closesocket(ClientSocket);

					int reuse = 1;
					setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

					ClientSocket = accept(ListenSocket, NULL, NULL);
					if (ClientSocket == INVALID_SOCKET);
					{
						printf("accept failed: %d\n", WSAGetLastError());
						continue;
					}
				}
				else if(iresult == SOCKET_ERROR)
				{
					closesocket(ClientSocket);

					int reuse = 1;
					setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

					ClientSocket = accept(ListenSocket, NULL, NULL);
					if (ClientSocket == INVALID_SOCKET)
					{
						printf("accept failed: %d\n", WSAGetLastError());
						continue;
					}
				}

			} while (1);

			printf("%s\n", recvbuff);

			if (strstr(recvbuff, "OFF") != NULL)
			{
				break;
			}

			bson_t* modem = bson_new();
			
			char timedate[37] = { '\0' };

			Total++;

			sscanf_s(recvbuff, "%19s %02d %02d %f %02d %f %f %f", timedate,(unsigned)_countof(timedate),&temp,&humi,&dist,&rssi,&disterror,&temperror,&humierror);
			printf("timedate: %s\n", timedate);
			printf("dist: %.2f temp: %d humi : %d, Rssi : %d\n",dist,temp,humi,rssi);
			printf("DistError : %.1f%% , TempError : %.1f%% , HumiError : %.1f%%\n", disterror, temperror, humierror);
			printf("-%d-\n\n", Total);

			BSON_APPEND_UTF8(modem, "timedate", timedate);
			BSON_APPEND_DOUBLE(modem, "distaver", dist);
			BSON_APPEND_INT32(modem, "temp",temp);
			BSON_APPEND_INT32(modem, "humi", humi);
			BSON_APPEND_DOUBLE(modem, "disterror", disterror);
			BSON_APPEND_DOUBLE(modem, "temperror", temperror);
			BSON_APPEND_DOUBLE(modem, "humierror", humierror);
			BSON_APPEND_INT32(modem, "Total", Total);

			if ((const char*)strstr(timedate, "23:50:00") != NULL)
			{
				Total = 0;
			}

			mongoc_collection_insert_one(collection, modem, NULL, NULL, &error);
			
			bson_destroy(modem);
		}

		bson_destroy(&reply);
		bson_destroy(ping);
		mongoc_database_destroy(database);
		mongoc_client_destroy(client);
		mongoc_uri_destroy(uri);
		mongoc_collection_destroy(collection);
		mongoc_cleanup();

		closesocket(ClientSocket);
		closesocket(ListenSocket);

		WSACleanup();

	return 0;
}
