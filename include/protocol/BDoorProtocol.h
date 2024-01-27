#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <nlohmann/json.hpp>
#include <other/tools.h>
#pragma comment(lib,"ws2_32.lib") 
using json = nlohmann::json;
#define EMPTYCHAR "0"
#define FIN_CHAR "FIN"
#define PROTOCOL "Protocol"
#define PROTOCOLNAME "BDoor"
#define COMMANDNAME "CommandName"
#define CONTENT "Content"
#define UDP_PORT 2410
#define GROUP_PORT 2411
#define GROUP_IP "224.1.1.4" 
#define TCP_PORT 2412
#define REVERSE_PORT 2413
#define NOTCOM 0
#define END 1
#define ENTRANCE 2
#define EXECUTE 3
#define ERROR -1
#define DATAPACKAGE_MAXSIZE 8
#define SEND_TIME 1

int WSAInit() {
	WSADATA wsadata;
	return WSAStartup(MAKEWORD(2, 2), &wsadata);
}

void FormatJson(json& j1) {
	json j2;
	j2[PROTOCOL] = PROTOCOLNAME;
	j2[COMMANDNAME] = "";
	j2[CONTENT] = "";
	j1 = j2;
}
json FormatJson() {
	json j;
	j[PROTOCOL] = PROTOCOLNAME;
	j[COMMANDNAME] = "";
	j[CONTENT] = "";
	return j;
}
json FormatJson(string command_name,string content) {
	json j;
	j[PROTOCOL] = PROTOCOLNAME;
	j[COMMANDNAME] = command_name;
	j[CONTENT] = content;
	return j;
}

namespace udp {
	int SendtoBase64(SOCKET s,sockaddr_in addr, string sendmess) {
		sendmess = EncodeStrInBase64(sendmess);
		int Result = NULL;
		for (int i = 0; i < sendmess.size(); i++) {
			Result = sendto(s, &sendmess[i], sizeof(sendmess[i]), 0,(const sockaddr *)&addr,sizeof(addr));
			if (Result <= 0) { break; }
		}
		if (Result == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (Result == 0) {
			cout << "连接已断开" << endl;
			return 0;
		}
		Result = sendto(s, FIN_CHAR, sizeof(FIN_CHAR), 0,(const sockaddr *)&addr,sizeof(addr));
		if (Result == SOCKET_ERROR) {
			wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (Result == 0) {
			cout << "连接已断开" << endl;
			return 0;
		}
		return 1;
	}
	int RecvfromBase64(SOCKET s,sockaddr_in &sender_addr,string &recvmess) {
		recvmess = "";
		int Result = NULL;
		char buffer[1024];
		int sender_addr_len = sizeof(sender_addr);
		do {
			memset(buffer, 0, sizeof(buffer));
			Result = recvfrom(s, buffer, sizeof(buffer), 0, (sockaddr*)&sender_addr, &sender_addr_len);
			recvmess += buffer;
		} while (string(buffer) != string(FIN_CHAR)&&Result > 0);
		if (Result == SOCKET_ERROR) {
			wprintf(L"recvfrom failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (Result == 0) {
			cout << "连接已断开" << endl;
			return 0;
		}
		recvmess = DecodeStrInBase64(recvmess);
		return 1;
	}
}
namespace tcp {
	int SendBase64(SOCKET s, string sendmess) {
		sendmess = EncodeStrInBase64(sendmess);
		int Result = NULL;
		for (int i = 0; i < sendmess.size(); i++) {
			Result = send(s, &sendmess[i], sizeof(sendmess[i]), 0);
			if (Result <= 0) { break; }
			//Sleep(SEND_TIME);
		}
		if (Result == SOCKET_ERROR) {
			wprintf(L"send failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (Result == 0) {
			cout << "连接已断开" << endl;
			return 0;
		}
		Sleep(SEND_TIME);
		Result = send(s, FIN_CHAR, sizeof(FIN_CHAR), 0);
		if (Result == SOCKET_ERROR) {
			wprintf(L"send failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (Result == 0) {
			cout << "连接已断开" << endl;
			return 0;
		}
		return 1;
	}
	int RecvBase64(SOCKET s, string& recvmess) {
		recvmess = "";
		int Result = NULL;
		char buffer[1024];
		do {
			memset(buffer, 0, sizeof(buffer));
			Result = recv(s, buffer, sizeof(buffer), 0);
			recvmess += buffer;
		} while (string(buffer) != string(FIN_CHAR)&&Result > 0);
		if (Result == SOCKET_ERROR) {
			wprintf(L"recv failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (Result == 0) {
			cout << "连接已断开" << endl;
			return 0;
		}
		recvmess = DecodeStrInBase64(recvmess);
		return 1;
	}
}
