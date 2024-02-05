#pragma once
#include <module/single thread DU/STUDprotocol.h>
using namespace std;
class STUDDownloader {
private:
	string filedata;
	SOCKET& tcp_socket;
public:
	STUDDownloader(SOCKET& tcp_socket_a) :tcp_socket(tcp_socket_a) {}
	int Download() {
		int Result;
		vector<STUDPacket> stud_packet_vec;
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in addr = CreateSockAddrIn(STUD_PORT);
		Result = bindport(listen_socket, (const sockaddr*)&addr, sizeof(addr));
		if (Result == SOCKET_ERROR) {
			DebugLog("bind failed with error: " + to_string(WSAGetLastError()));
			return -1;
		}
		Result = listen(listen_socket, 5);
		if (Result == SOCKET_ERROR) {
			DebugLog("listen failed with error: " + to_string(WSAGetLastError()));
			return -1;
		}
		sockaddr connect_addr;
		int connect_addr_len = sizeof(connect_addr);
		SOCKET connect_socket = accept(listen_socket, (sockaddr*)&connect_addr, &connect_addr_len);
		char buf[STUD_PACKET_LEN+STUD_HEAD_LEN];
		while (true) {
			memset(buf, 0, sizeof(buf));
			Result = recv(connect_socket, buf, sizeof(buf), 0);
			if (Result == SOCKET_ERROR) {
				DebugLog("recv failed with error: " + to_string(WSAGetLastError()));
				return -1;
			}
			else if (Result == 0) {
				DebugLog(DISCONNECT);
				break;
			}
			vector<string> buf_vec;
			split_string(buf, STUD_DIV_SIGN, buf_vec);

			STUDPacket stud_packet;
			stud_packet.data = buf_vec[0];
			stud_packet.num = atoi(buf_vec[2].c_str());
			int packet_len = atoi(buf_vec[1].c_str());
			if (stud_packet.data.size() < packet_len) {
				DebugLog("损失了 " + to_string(packet_len - stud_packet.data.size()) + "字节");
				Result = tcp::SendBase64(tcp_socket, STUD_DATA_LOSS);
				if (Result <= 0) { return Result; }
			}
			else {
				stud_packet_vec.push_back(stud_packet);
				Result = tcp::SendBase64(tcp_socket, STUD_DATA_COMPLETE);
				if (Result <= 0) { return Result; }
			}
		}
		sort(stud_packet_vec.begin(), stud_packet_vec.end(), STUDPacketCompare);
		filedata.clear();
		for (int i = 0; i < stud_packet_vec.size(); i++) {
			filedata += stud_packet_vec[i].data;
		}
		return 1;
	}
	int SaveFile(string savepath) {
		ofstream ofs(savepath, ios::out | ios::binary);
		if (ofs.is_open()) {
			DebugLog("文件打开成功");
		}
		else {
			DebugLog("文件打开失败");
			return -1;
		}
		ofs.write(filedata.data(), filedata.size());
		ofs.close();
		DebugLog("文件保存成功");
		return 1;
	}
	string& GetFiledata() { return filedata; }
};