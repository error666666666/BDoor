#pragma once
#include<module\mtftprotocol.h>
#include<fstream>
#include<other\tools.h>
#include <protocol\BDoorProtocol.h>
class Uploader {
private:
	string data;
	string ip;
	//vector<Package> package_data;
public:
	Uploader(string ip_m) {
		ip = ip_m;
	}
	int LoadFile(string filepath) {
		data.clear();
		ifstream ifs;
		ifs.open(filepath.c_str(), ios::in | ios::binary);
		if (!ifs.is_open()) {
			DebugLog("文件打开失败");
		}
		else {
			DebugLog("文件打开成功");
		}
		vector<char> c_data(istreambuf_iterator<char>(ifs), {});
		ifs.close();
		for (int i = 0; i < c_data.size(); i++) { data += c_data[i]; }
		return 1;
	}
	int LoadFile(IStream* pStream) {
		data.clear();
		HGLOBAL hGlobal = NULL;
		GetHGlobalFromStream(pStream, &hGlobal); // 获取 IStream 的内存句柄
		LPBYTE pBits = (LPBYTE)GlobalLock(hGlobal);
		data = (char *)pBits;
	}
	void SetIP(string ip_m) {
		ip = ip_m;
	}
	vector<Package> AssignData() {
		vector<Package> package_vec = AssignData();
		int num = 0;
		for (int i = 0; i < data.size(); i++) {
			Package package;
			if (data.size() - i >= PACKSIZE) {
				package.data = data.substr(i, PACKSIZE);
				i += PACKSIZE - 1;
			}
			else {
				package.data = data.substr(i, data.size() - i);
				i += data.size() - 1;
			}
			package.num = num;
			package_vec.push_back(package);
			num++;
		}
		return package_vec;
	}
	int Upload() {
		vector<Package>package_vec(AssignData());
		int quotient = package_vec.size() / NUMBEROFTHREAD;
		int remainder = package_vec.size() % NUMBEROFTHREAD;
		int package_vec_pos = 0;
		for (int i = 0; i < NUMBEROFTHREAD; i++) {
			int packagenum = quotient;
			if (remainder > 0) {
				packagenum++;
				remainder--;
			}
			thread([=]() {
				SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_port = htons(FIRSTPORT + i);
				addr.sin_addr.s_addr = inet_addr(ip.c_str());
				connect(connect_socket, (const sockaddr*)&addr, sizeof(addr));
				int Result;
				for (int j = 0; j < packagenum; j++) {
					Package package = package_vec[package_vec_pos + j];
					string sendmess = package.num + SPLCHAR + to_string(package.data.size() + 1) + SPLCHAR + package.data;
					Result = send(connect_socket,sendmess.c_str(),sendmess.size()+1, 0);
					if (Result == SOCKET_ERROR) {
						DebugLog("send failed with error: " + to_string(WSAGetLastError()));
						return -1;
					}
					else if (Result == 0) {
						DebugLog(DISCONNECT);
						j--;
						connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
						connect(connect_socket, (const sockaddr*)&addr, sizeof(addr));
					}
				}
				
				Result = shutdown(connect_socket, SD_BOTH);
				if (Result == SOCKET_ERROR) {
					DebugLog("shutdown failed with error: " + to_string(WSAGetLastError()));
					return -1;
				}
				else if (Result == 0) {
					DebugLog(DISCONNECT);
					return -1;
				}
				if (connect_socket != INVALID_SOCKET) {
					closesocket(connect_socket);
				}
				}).detach();
			package_vec_pos += packagenum;
		}
	}
};