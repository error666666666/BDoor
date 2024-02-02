#pragma once
/*Multi-threaded file transfer protocol*/
#define FIRSTPORT 1100
#define NUMBEROFTHREAD 32
#define PACKSIZE 60000
#define SPLCHAR "|"
struct Package
{
	string data;
	int num;
};