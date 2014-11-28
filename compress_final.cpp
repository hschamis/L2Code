//  compress_final.cpp
//  Created by Hanna Schamis & Jessica Micallef
//
// Last edit by Jessica Micallef on 11/21/2014
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>

#define ENERGY_WORDS 1024
#define COE_WORDS 6
#define ADC_HEADERS 8
#define MINRANGE 32
#define ALL_HEADERS 40
#define ALL_WORDS 1070
#define WORDS_WITHOUT_MINRANGE 1038

using namespace std;

////////////////// Flip Endianness /////////////
void FlipStuff(uint16_t *WordArray){
	uint16_t Half1, Half2;
	for (int h=0; h<ENERGY_WORDS+ADC_HEADERS+COE_WORDS; h++){
		Half1=(WordArray[h]<<8);
		Half2=(WordArray[h]>>8);
		WordArray[h]=((Half1&0xFF00)|(Half2&0x00FF));
	}
}
/////////////// Flip Endianness Back ////////
void FlipBack(uint16_t *WordArray){
	uint16_t Half1, Half2;
	for (int k=0; k<ALL_WORDS; k++){
		Half1=(WordArray[k]<<8);
		Half2=(WordArray[k]>>8);
		WordArray[k]=((Half1&0xFF00)|(Half2&0x00FF));
	}
}
//////////////// Function that finds the minimum and the ranges of uncompressed file///////////
void FindMinRange(uint16_t *WordArray){
	uint16_t MINIMUMS[16] = {0};
	uint16_t MAXIMUMS[16]= {0};
	uint16_t RANGE[16] = {0};

	//Take the first 16 energy words of the file and set those as the minimums and maximums	
	for (int i = 8; i < 24; i++){
		MINIMUMS[i-8]=(WordArray[i]&0x3FFF);
		MAXIMUMS[i-8]=(WordArray[i]&0x3FFF);
	}
	//Check each of the 64 samples against its own channel (%16 assigns each check to be made on the coresponding channel)
	uint16_t Temporary;
	for (int i = 24; i < ENERGY_WORDS+ADC_HEADERS; i++){
		Temporary = (WordArray[i]&0x3FFF);
		if (Temporary>MAXIMUMS[(i-8)%16]){
			MAXIMUMS[(i-8)%16]=(WordArray[i]&0x3FFF);
		}

		if(Temporary<MINIMUMS[(i-8)%16]){
			MINIMUMS[(i-8)%16]=(WordArray[i]&0x3FFF);
		}
	}
	//Fill all energy words into temporary array
	uint16_t WordArrayEnergyTemp[ENERGY_WORDS+COE_WORDS] = {0};
	for (int b = 0; b < ENERGY_WORDS+COE_WORDS; b++){
		WordArrayEnergyTemp[b] = WordArray[b+8];
	}
	//Move energy words to start at spot 40 to make room for minimums and ranges:
	for (int j=40; j< (40+ENERGY_WORDS+COE_WORDS); j++){
		WordArray[j]=WordArrayEnergyTemp[j-40];
	}
	//Put in minimums after the 8 headers
	for (int k=8; k<ADC_HEADERS+16; k++){
		WordArray[k]=MINIMUMS[(k-8)%16];
	}
	// Calculate differences;
	for (int j=0; j<16; j++){
		RANGE[j]=MAXIMUMS[j]-MINIMUMS[j];		
	}


	for(int i = 0; i < 16; i++){
				for (int j=1; j<=7; j++){
			if (j==7){
								WordArray[i+24]=14;     //Range
								WordArray[i+8]=0;       //Minimum
							cout << "Ranges more than 7! At packet " << (WordArray[1]&0x000F) << " event " << (WordArray[1]&0x00F0) << " - channel " << i << endl;
			}
						if (RANGE[i]<pow(2,j)){              /* Changed to >= so if you have 4, 8, 16, etc, bits it'll round up. */
								WordArray[i+24]=j;
								break;
						}
				}
		}

}
////////////// Function that compresses a single packet ///////////
int Compress(uint16_t *decompWords, uint16_t *CompWords){
	// Center of Energy
	uint16_t CenterOfEnergy[6]={0};
	for (int i = 0; i<COE_WORDS; i++){
		CenterOfEnergy[i]=decompWords[ALL_HEADERS+ENERGY_WORDS+i];
	}	
	// Range
	uint16_t Range[16]={0};
	for (int i=24; i<40; i++){
		Range[i-24]=decompWords[i];
	}
	// declare the new array (where compressed data will go)
	for (int i = 0; i<ALL_HEADERS;i++){
		CompWords[i]=(decompWords[i]|0xC000);
	}
	//calculate differences
	uint16_t Diffs[ENERGY_WORDS] = {0};
	for (int i=0; i<ENERGY_WORDS; i++){
		Diffs[i]=(decompWords[i+40]&0x3FFF)-decompWords[(i%16)+8];
	}
	//calculate number of rows that 16 channels will compress into (sum of the ranges, round up to integer)
	int Rows=0;
	int TotalBits = 0;
	for (int i = 0; i < 16; i ++){
		TotalBits = TotalBits + Range[i];
	}
	Rows = TotalBits/14;
	if(TotalBits%14 != 0){
		Rows=Rows+1;
	}
	//Create binary compressed words
	int Counter = 0;			//Size of range in bits as running sum for the row
	uint16_t Binary[1024] = {0}; 		//use 1024 because that's the maximum rows (binary words) that it could have.
	int BinPlace = 0;   			//indicates which row it's doing binary for.
	uint16_t Temp = 0;
	int DecompPlace;
	for (int i = 0; i<ENERGY_WORDS; i++){
		if ((i%16==0)&&(i!=0)){
			Counter = 0;
			BinPlace++;
		}
		Temp=Diffs[i]<<Counter;
		DecompPlace = (i%16)+24;
		if (Counter+Range[i%16] <=14){
			Binary[BinPlace]=((Binary[BinPlace]|Temp)&0x3FFF);
			Counter += Range[i%16];
			if (Counter == 14){
				Counter=0; // reset counter
				if(BinPlace!=Rows){
					BinPlace++; // next "row"
				}
			}
		}
		else {
			Binary[BinPlace]=((Binary[BinPlace]|Temp)&0x3FFF);
			Binary[BinPlace+1]=((Diffs[i]>>(14-Counter))&0x3FFF);
			BinPlace++;
			Counter = Range[i%16] - (14-Counter);
		}
	}
	BinPlace = BinPlace+1;		//Round up the last row to make sure last compressed word included
		
	//Use the number of rows to keep only the compressed data
	for (int k = 0; k < BinPlace; k++){
		CompWords[40+k]=(Binary[k]|0x8000);
	}
	//Add COE words to the end of the compressed array
	int CompVar;
	for (int i = 0; i < COE_WORDS; i++){
		CompVar = i+40+BinPlace;
		CompWords[CompVar] = CenterOfEnergy[i];
	}
	//Changing headers 3 and 4 
	uint16_t NWords=BinPlace+6+COE_WORDS;	// Total number of compressed words + COE words
	CompWords[2]=((CompWords[2]&0xFF00)|(NWords>>8));
	CompWords[3]=((CompWords[3]&0xFF00)|(NWords&0x00FF));

	return BinPlace;		//Return BinPlace to use to find total number of compressed words in array
}
	
int main(){

	uint16_t DecompressedWords[ALL_WORDS] = {0};	//Array to store single packet of decompressed words, read in from file
	uint16_t CompressedWords[ALL_WORDS] = {0};		//Array to store final compressed words
	uint16_t CenterOfEnergy[6] = {0};			//Array to store COE information from the file
	int TotalCompWords;					//Total energy words after compression + 8 headers + 16 min + 16 ranges + 6 COE words
	int BinPlace = 0;					//Total energy words after compression
	int RunningCount = 0;				//Sum of the total words written to file 
	int LastEvent = 0;					//Stores the last header's event stamp 
	int TotalInput;					//Total number of bytes in input file
	int PacketMax;					//Total number of packets in input file
	int CheckPacket = 0;				//Integer than goes from 0 to 15 that should match the packet stamp on incoming packets
	int CheckPacket2 = 0;				//Integer than goes from 0 to 15 that should match the packet stamp on outgoing packets

	//Set parameters to read and write files
	char read_file[256];
	char write_file[256];
	FILE * run_file;
	FILE * output_file;

	//Specify the nodes and slots to run through (integers in file names)
	for (int node=5; node<6; node++){
		if((node==103)|(node==106)|(node==111)|(node==113)){continue;}
		for (int slot=7; slot<8;slot++){
			if(slot==12){continue;}
			CheckPacket=0;
			CheckPacket2=0;
			
			//Open files to read from and write to
			sprintf(read_file, "/home/Hanna/CompDecomp/run15703_node%d_slot%d_uncompressed.bin", node, slot);
			sprintf(write_file,"/home/Hanna/L2Code/run15703_node%d_slot%d_compressed.bin", node, slot );
			run_file=fopen64(read_file,"rb");
			output_file = fopen(write_file,"wb");
			//Find how many words in file
			fseek(run_file, -2, SEEK_END);
			TotalInput = ftell(run_file)/2;				//Gives size of file, divided by 2 bytes to find how many words;
			PacketMax = (TotalInput/WORDS_WITHOUT_MINRANGE);	//Divde by the number of words in uncompressed file to find total number of packets
			rewind(run_file);					//set position inside file to beginning
		
			cout << "Working on Node " << node << " Slot " << slot <<"."<<endl;
				
			//Take each Packet individually to compress
			for (int H=0; H<PacketMax; H++){
				if (run_file == NULL){cout << "Run File Null at " << H << endl;}
				//Read words into Decompressed Array
				fread(DecompressedWords, sizeof(uint16_t),ADC_HEADERS+ENERGY_WORDS+COE_WORDS, run_file);

				//Flip the endianess of the energy words;
				FlipStuff(DecompressedWords);
					
				if((DecompressedWords[1]&0x00F0)==LastEvent && (DecompressedWords[1]&0x000F)==0){
					//cout << "SAME EVENT AFTER 16 PACKETS AT " << H << endl;
				}

				//Checks done for Decompressed Array
				//cout << "Decomp Event Number = " << (DecompressedWords[1]&0x00F0) << " packet = " << (DecompressedWords[1]&0x000F) << endl; 
				if((DecompressedWords[1]&0x000F) != CheckPacket){	//compares packet stamp in header to check packet to see if packet skipped
					cout << "SKIPPED A PACKET IN DECOMP PACKET " << H << " Event Number = " << (DecompressedWords[1]&0x00F0) << " packet = " << (DecompressedWords[1]&0x000F) << endl;
				}
				if (CheckPacket < 15){
					CheckPacket++;			//Increase CheckPacket number by one, if packet dropped this will not match
				}
				else{
					CheckPacket = 0;		//reset CheckPacket once at 15 since the next packet should have stamp 0
				}	
				if((DecompressedWords[1]&0x000F) == 15){
					LastEvent = (DecompressedWords[1]&0x00F0);
				}
				if((DecompressedWords[1]&0x00F0)==LastEvent && (DecompressedWords[1]&0x000F)==0){	//Check Event is new after 16 packets 
					//cout << "SAME EVENT AFTER 16 PACKETS AT " << H << endl;
				}                          

				//Insert COE words into the last 6 places of the Decompressed Words Array
				for (int i = 0; i < COE_WORDS; i++){
					CenterOfEnergy[i]=DecompressedWords[ENERGY_WORDS+ADC_HEADERS+i];
				}

				//Find the Min & Max in uncompressed file, use to create Min & Range headers
				FindMinRange(DecompressedWords);

				//Compression Function
				BinPlace = Compress(DecompressedWords, CompressedWords);	//Call compress function, return number of compressed energy words
				TotalCompWords = BinPlace + ALL_HEADERS + COE_WORDS;				//find total words in compress packet including headers, min, range, energy words, and COE words

				//Checks done on Compressed Word Array
				//cout << "Comp Event Number = " << (CompressedWords[1]&0x00F0) << " packet = " << (CompressedWords[1]&0x000F) << endl;  
				if((CompressedWords[1]&0x000F) != CheckPacket2){	//compares packet stamp in header to running integer
					cout << "SKIPPED A PACKET IN COMP PACKET " << H << " Event Number = " << (CompressedWords[1]&0x00F0) << " packet = " << (CompressedWords[1]&0x000F) << endl;
				}
				if (CheckPacket2 < 15){
					CheckPacket2++;			//Increase CheckPacket number by one, if packet dropped this will not match
				}
				else{
					CheckPacket2 = 0;		//reset CheckPacket once at 15 since the next packet should have stamp 0
				}
				//Change Endianness again for output
				FlipBack(CompressedWords);
				//Write Compressed packet to output file
				fwrite(CompressedWords, sizeof(uint16_t), TotalCompWords, output_file);
				RunningCount = RunningCount + TotalCompWords;			//Sum of total words written to file, used for checks
				BinPlace = 0;							//reset the compressed word count
			}
			cout << "Node #" << node <<" , Slot #" << slot << " done!" << endl;
			fclose(run_file);
			fclose(output_file);
		}

	}

	return 0;
}