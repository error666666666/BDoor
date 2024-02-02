#pragma once
#include <module\mtftprotocol.h>
#include<fstream>
class Downloader {
private:
	string data = "";
public:
	int Download(string filepath) {
		vector<Package> package_vec;
		vector<bool> is_end(NUMBEROFTHREAD,false);
		for (int i = 0; i < NUMBEROFTHREAD; i++) {
			thread([&]() {
				SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_port = htons(FIRSTPORT + i);
				addr.sin_addr.s_addr = htons(INADDR_ANY);
				bindport(listen_socket, (const sockaddr*)&addr, sizeof(addr));
				listen(listen_socket, 5);
				sockaddr connect_addr;
				int connect_addr_len = sizeof(connect_addr);
				SOCKET connect_socket = accept(listen_socket, (sockaddr*)&connect_addr, &connect_addr_len);
				char buffer[PACKSIZE];
				int Result;
				while (true) {
					memset(buffer, 0, sizeof(buffer));
					Result = recv(connect_socket, buffer, sizeof(buffer), 0);
					if (Result == SOCKET_ERROR) {
						DebugLog("recv failed with error: " + to_string(WSAGetLastError()));
						return -1;
					}
					else if (Result == 0) {
						DebugLog(DISCONNECT);
						return -1;
					}
					vector<string> buffer_spl;
					split_string(string(buffer), SPLCHAR, buffer_spl);
					Package package;
					package.num = atoi(buffer_spl[0].c_str());
					package.data = buffer_spl[2];
					int realsize = atoi(buffer_spl[1].c_str());
					if (realsize != package.data.size()+1) {
						DebugLog("数据丢失 " + to_string(package.data.size() + 1 - realsize) + " 字节");
						Result = shutdown(connect_socket, SD_BOTH);
						if (Result == SOCKET_ERROR) {
							DebugLog("shutdown failed with error: " + to_string(WSAGetLastError()));
							return -1;
						}
						else if (Result == 0) {
							DebugLog(DISCONNECT);
						}
						if (connect_socket != INVALID_SOCKET) {
							closesocket(connect_socket);
						}
						connect_socket = accept(listen_socket, (sockaddr*)&connect_addr, &connect_addr_len);
					}
					else {
						package_vec.push_back(package);
					}
				}
				is_end[i] = true;
				}).detach();
		}
		while (true){
			for (int i = 0; i < is_end.size(); i++) {
				if (is_end[i] == false) { break; }
			}
			break;
		}
		sort(package_vec.begin(), package_vec.end(), [=](Package p1, Package p2) {
			return p1.num > p2.num;
			});
		data.clear();
		for (int i = 0; i < package_vec.size(); i++) {
			data += package_vec[i].data;
		}
	}
	int Save(string filepath) {
		ofstream ofs;
		ofs.open(filepath.c_str(), ios::out | ios::binary);
		if (ofs.is_open()) {
			DebugLog("文件打开成功");
		}
		else {
			DebugLog("文件打开失败");
		}
		ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
		DebugLog("文件写入成功");
		ofs.close();
	}
	string GetData() { return data; }
};