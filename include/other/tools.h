#pragma once
#include <string> 
#include <vector>
#include <list>
#include <regex>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#define ASCII " !\"#$%&,()*+'-./0123456789:;<=>?@[\\]^_{|}`abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
using namespace std;



//分割字符串 
void split_string(const string str, const string splits, vector<string>& res)
{
	if (str == ""){return;}
	//在字符串末尾也加入分隔符，方便截取最后一段
	string strs = str + splits;
	size_t pos = strs.find(splits);
	int step = splits.size();
 
	// 若找不到内容则字符串搜索函数返回 npos
	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		//去掉已分割的字符串,在剩下的字符串中进行分割
		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
}

void split_string(const string str, const string splits, list<string>& res)
{
	if (str == ""){return;}
	//在字符串末尾也加入分隔符，方便截取最后一段
	string strs = str + splits;
	size_t pos = strs.find(splits);
	int step = splits.size();
 
	// 若找不到内容则字符串搜索函数返回 npos
	while (pos != strs.npos)
	{
		string temp = strs.substr(0, pos);
		res.push_back(temp);
		//去掉已分割的字符串,在剩下的字符串中进行分割
		strs = strs.substr(pos + step, strs.size());
		pos = strs.find(splits);
	}
}

template <typename T>
T list_at(list<T>& res,int index){
	int i = 0;
	for(auto e : res){
		if(i == index){
			return e;
		}else{
			i++;
		}
	}
}

//生成随机数 
int getRand(int min, int max) {
    return ( rand() % (max - min + 1) ) + min ;
}

bool check_ip(string ip){
	
	regex ip_pattern("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
	smatch s;
	if(regex_match(ip,s,ip_pattern)){
		return true;
	}else{
		return false;
	}
						
}

template <class T>
int length(const T arr[]){
	
	return sizeof(arr) / sizeof(arr[0]);

}

bool operator==(const sockaddr_in& addr1, const sockaddr_in& addr2) {

	return addr1.sin_family == addr2.sin_family && addr1.sin_port == addr2.sin_port && addr1.sin_addr.s_addr == addr2.sin_addr.s_addr;

}
int bindport(SOCKET &s, const struct sockaddr* name, int namelen) {
	int ret = ::bind(s, name, namelen);
	return ret;
}

sockaddr_in CreateSockAddrIn(int port) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	return addr;
}
sockaddr_in CreateSockAddrIn(string ip,int port) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip.data());
	return addr;
}
int indexOfCode(const char c) {
	const char* code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	for (unsigned int i = 0; i < 64; i++) {
		if (code[i] == c)
			return i;
	}
	return 0;
}
std::string encodeBase64(string str) {
	unsigned char* input = (unsigned char*)str.c_str();
	int input_len = str.size();
	const char* code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char input_char[3];
	char output_char[5];
	int output_num;
	std::string output_str = "";
	int near_a = input_len % 3;

	for (int i = 0; i < input_len; i += 3) {
		memset(input_char, 0, 3);
		memset(output_char, 61, 5);
		if (i + 3 <= input_len) {
			memcpy(input_char, input + i, 3);
		}
		else {
			// 不够凑成3个byte
			memcpy(input_char, input + i, input_len - i);
			output_num = ((int)input_char[0] << 16) + ((int)input_char[1] << 8) + (int)input_char[2];

			if (near_a == 1) {
				output_char[0] = code[((output_num >> 18) & 0x3f)];
				output_char[1] = code[((output_num >> 12) & 0x3f)];
				output_char[2] = '=';
				output_char[3] = '=';
				output_char[4] = '\0';
			}

			if (near_a == 2) {
				output_char[0] = code[((output_num >> 18) & 0x3f)];
				output_char[1] = code[((output_num >> 12) & 0x3f)];
				output_char[2] = code[((output_num >> 6) & 0x3f)];;
				output_char[3] = '=';
				output_char[4] = '\0';
			}

			output_str.append(output_char);
			break;
		}

		output_num = ((int)input_char[0] << 16) + ((int)input_char[1] << 8) + (int)input_char[2];
		output_char[0] = code[((output_num >> 18) & 0x3f)];
		output_char[1] = code[((output_num >> 12) & 0x3f)];
		output_char[2] = code[((output_num >> 6) & 0x3f)];
		output_char[3] = code[((output_num) & 0x3f)];
		output_char[4] = '\0';
		output_str.append(output_char);
	}

	return output_str;
}

std::string decodeBase64(std::string input) {
	unsigned char input_char[4];
	unsigned char output_char[4];
	int output_num = 0;
	int k = 0;
	std::string output_str = "";

	for (unsigned int i = 0; i < input.size(); i++) {
		input_char[k] = indexOfCode(input[i]);
		k++;
		if (k == 4) {
			output_num = ((int)input_char[0] << 18) + ((int)input_char[1] << 12) + ((int)input_char[2] << 6) + ((int)input_char[3]);
			output_char[0] = (unsigned char)((output_num & 0x00FF0000) >> 16);
			output_char[1] = (unsigned char)((output_num & 0x0000FF00) >> 8);
			output_char[2] = (unsigned char)(output_num & 0x000000FF);
			output_char[3] = '\0';
			output_str.append((char*)output_char);
			k = 0;
		}
	}

	return output_str;
}
string splicing_string(vector<string> &vec_str,const string splits) {
	string str;
	for (int i = 0; i < vec_str.size(); i++) {
		str += vec_str[i];
		if (i != vec_str.size() - 1) { str += splits; }
	}
	return str;
}
string EncodeStrInBase64(string cmd_mess) {
	vector<string>cmd_mess_split_endl;
	split_string(cmd_mess, "\n", cmd_mess_split_endl);
	vector<vector<string>> cmd_mess_split;
	for (int i = 0; i < cmd_mess_split_endl.size(); i++) {
		vector<string> split_space;
		split_string(cmd_mess_split_endl[i], " ", split_space);
		cmd_mess_split.push_back(split_space);
	}
	for (int i = 0; i < cmd_mess_split.size(); i++) {
		for (int j = 0; j < cmd_mess_split[i].size(); j++) {
			cmd_mess_split[i][j] = encodeBase64(cmd_mess_split[i][j]);
		}
	}
	vector<string> encode_ves;
	for (int i = 0; i < cmd_mess_split.size(); i++) {
		encode_ves.push_back(splicing_string(cmd_mess_split[i], " "));
	}
	return splicing_string(encode_ves, "\n");
}
string DecodeStrInBase64(string cmd_mess) {
	vector<string>cmd_mess_split_endl;
	split_string(cmd_mess, "\n", cmd_mess_split_endl);
	vector<vector<string>> cmd_mess_split;
	for (int i = 0; i < cmd_mess_split_endl.size(); i++) {
		vector<string> split_space;
		split_string(cmd_mess_split_endl[i], " ", split_space);
		cmd_mess_split.push_back(split_space);
	}
	for (int i = 0; i < cmd_mess_split.size(); i++) {
		for (int j = 0; j < cmd_mess_split[i].size(); j++) {
			cmd_mess_split[i][j] = decodeBase64(cmd_mess_split[i][j]);
		}
	}
	vector<string> encode_ves;
	for (int i = 0; i < cmd_mess_split.size(); i++) {
		encode_ves.push_back(splicing_string(cmd_mess_split[i], " "));
	}
	return splicing_string(encode_ves, "\n");
}