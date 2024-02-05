#pragma once
#include <module/single thread DU/STUDprotocol.h>
class STUDUploader {
private:
	string filedata;
	SOCKET& tcp_socket;
	sockaddr_in& tcp_addr;
	vector<STUDPacket> AssignPacket() {
		filedata = EncodeBase64(filedata, false);
		vector<STUDPacket> stud_packet_vec;
		int number = 0;
		for (int i = 0; i < filedata.size(); i++) {
			STUDPacket stud_packet;
			if (filedata.size() - i >= STUD_PACKET_LEN) {
				stud_packet.data = filedata.substr(i, STUD_PACKET_LEN);
				i += STUD_PACKET_LEN - 1;
			}
			else {
				stud_packet.data = filedata.substr(i, filedata.size() - i);
				i += filedata.size() - 1;
			}
			stud_packet.num = number;
			stud_packet_vec.push_back(stud_packet);
		}
		return stud_packet_vec;
	}
public:
	STUDUploader(SOCKET& tcp_socket_a,sockaddr_in& tcp_addr_a) :tcp_socket(tcp_socket_a),tcp_addr(tcp_addr_a) {}
	int LoadFile(string filepath) {
		filedata.clear();
		ifstream ifs(filepath, ios::in | ios::binary);
		if (ifs.is_open()) {
			DebugLog("文件打开成功");
		}
		else {
			DebugLog("文件打开失败");
			return -1;
		}
		stringstream ss;
		ss << ifs.rdbuf();
		filedata = ss.str();
		ifs.close();
		return 1;
	}
	int LoadFile(IStream* pStream) {
		filedata.clear();
		HGLOBAL hGlobal = NULL;
		GetHGlobalFromStream(pStream, &hGlobal); // 获取 IStream 的内存句柄
		LPBYTE pBits = (LPBYTE)GlobalLock(hGlobal);
		filedata = (char*)pBits;
	}
	int Upload() {
		int Result;
		vector<STUDPacket> stud_packet_vec = AssignPacket();
		SOCKET connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in addr = CreateSockAddrIn(inet_ntoa(tcp_addr.sin_addr),STUD_PORT );
		Result = connect(connect_socket, (const sockaddr*)&addr, sizeof(addr));
		if (Result == SOCKET_ERROR) {
			DebugLog("connect failed wit error: " + to_string(WSAGetLastError()));
			return -1;
		}
		for (int i = 0; i < stud_packet_vec.size(); i++) {
			string sendmess = stud_packet_vec[i].data + STUD_DIV_SIGN + to_string(stud_packet_vec[i].data.size()) + STUD_DIV_SIGN + to_string(stud_packet_vec[i].num);
			Result = send(connect_socket, sendmess.c_str(), sendmess.size() + 1, 0);
			if (Result == SOCKET_ERROR) {
				DebugLog("send failed with error: " + to_string(WSAGetLastError()));
				return -1;
			}
			else if (Result == 0) {
				DebugLog(DISCONNECT);
				return 0;
			}
			string recvmess;
			Result = tcp::RecvBase64(tcp_socket, recvmess);
			if (Result <= 0) { return Result; }
			if (recvmess == STUD_DATA_COMPLETE) {

			}
			else if (recvmess == STUD_DATA_LOSS) {
				i -= 1;
			}
		}
		Result = shutdown(connect_socket, SD_BOTH);
		if (Result == SOCKET_ERROR)
		{
			DebugLog("shutdown failed with error: " + to_string(WSAGetLastError()));
			return -1;
		}
		if (connect_socket != INVALID_SOCKET) {
			closesocket(connect_socket);
		}
		return 1;
	}
};