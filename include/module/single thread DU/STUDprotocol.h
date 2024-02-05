#pragma once
#include <sstream>
#include <other/tools.h>
#include <protocol/BDoorProtocol.h>
#define STUD_PORT 1040
#define STUD_PACKET_LEN 60000
#define STUD_HEAD_LEN 128
#define STUD_DIV_SIGN "&"
#define STUD_SUCCESS "success"
#define STUD_FAILED "failed"
#define STUD_DATA_COMPLETE "data complete"
#define STUD_DATA_LOSS "data loss"
#define STUD_PACKET_LOST "packet lost"
struct STUDPacket
{
	string data;
	int num;
};
bool STUDPacketCompare(STUDPacket& studp1, STUDPacket& studp2) {
	return studp1.num > studp2.num;
}