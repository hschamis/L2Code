#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>

using namespace std;

int main(){

	int node;
	int slot;
	int packetnumber;
	cout << "Input node number to check:" << endl;
	cin >> node;
	cout << "Input slot number to check:" << endl;
	cin >> slot;
	cout << "Type the number of packets you want to check (type -1 for all):" << endl;
	cin >> packetnumber;

	uint16_t ChrisFileWords[1038] = {0};
	uint16_t HannaFileWords[1070] = {0};
	int errorcount = 0;
	int totalerror = 0;
	int TotalInput = 0;
	int PacketMax;
	int random;

	char read_ChrisFileNumber[256];
	char read_HannaFileNumber[256];

	FILE *ChrisFilePoint;
	FILE *HannaFilePoint;

	sprintf(read_ChrisFileNumber, "/mydaq/crymph/uncompressed/run15703_node%d_slot%d_uncompressed.bin", node, slot);
	sprintf(read_HannaFileNumber, "/sudaq/jessimic/decompressed/run15703_node%d_slot%d_decompressed.bin", node, slot );
	ChrisFilePoint = fopen(read_ChrisFileNumber,"rb");
	HannaFilePoint = fopen(read_HannaFileNumber,"rb");

	//Find how many words in file
                fseek(ChrisFilePoint, -2, SEEK_END);
                TotalInput = ftell(ChrisFilePoint)/2;                         //Gives size of file, divided by 2 bytes to find how many words;
                PacketMax = (TotalInput/1038);        //Divde by the number of words in uncompressed file to find total number of packets
                rewind(ChrisFilePoint);                                       //set position inside file to beginning
		if(packetnumber == -1){
			packetnumber = PacketMax;
		}

	for(int i = 0; i < packetnumber; i++){
		fread(ChrisFileWords, sizeof(uint16_t),1038, ChrisFilePoint);
		fread(HannaFileWords, sizeof(uint16_t),1070, HannaFilePoint);
		errorcount = 0;
		
		for(int headers = 0; headers < 8; headers++){
			if(ChrisFileWords[headers] != HannaFileWords[headers]){
				cout << "HEADERS WRONG AT " << headers << endl;
				errorcount++;
			}		
		}

		for(int EWords = 0; EWords < 1030; EWords++){
                	if(ChrisFileWords[EWords+8] != HannaFileWords[EWords+40]){
                        	cout << "EWORDS WRONG AT " << EWords << endl;
                        	errorcount++;
                	}
        	}
		
		if (errorcount>0){
			cout << "ERROR COUNT for packet " << i << " = " << errorcount << endl;
			totalerror++;
		}
	
	}
	if(totalerror >0){
		cout << "THERE WERE ERRORS! CHECK!" << endl;
	}
	else{
		cout << "NO ERRORS! EITHER YOU WIN OR YOU GOT LUCKY, TYPE A RANDOM NUMBER TO FIND OUT WHICH:" << endl;
		cin >> random;
		if(random > 100 || random == 77 || random < 18){
			cout << "Wow, you just got lucky" << endl;
		}
		else{
			cout << "It's due to your intelligence that you've continued to succeed. Keep going, young padawan" << endl;
		}
	}

	return 0;
}

