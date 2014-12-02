// Decompress binary
// Created by Hanna Schamis 11/4/14

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
void FlipStuff(uint16_t *WordArray, int N) {
	uint16_t Half1, Half2;
	for (int h=0; h<N; h++){
		Half1=(WordArray[h]<<8);
		Half2=(WordArray[h]>>8);
		WordArray[h]=((Half1&0xFF00)|(Half2&0x00FF));
	}
}
void Decompress(uint16_t *CompressedWords, uint16_t *DecompressedWords){
	int Ranges[16] = {0};
	int newCounter;
	for (int i = 0; i<16; i++){
		Ranges[i]=(CompressedWords[(i+24)]&0x3FFF);
	}
	for (int j=0; j<ALL_HEADERS; j++){
		DecompressedWords[j]=CompressedWords[j]|0xC000;
	}
	int Counter = 0;
	int CompPlace = ALL_HEADERS; // This is where it starts
	uint16_t Temp, Temp2, Mask;
	int EnergyPlace = 0;	//This tells us what energy word we are reading right now. 
	for (int k=ALL_HEADERS; k<ALL_WORDS-COE_WORDS; k++){
		EnergyPlace = k - ALL_HEADERS;
		if (((EnergyPlace%16)==0)&&(EnergyPlace!=0)){
			Counter = 0;
			CompPlace++;
		}
		Temp = (CompressedWords[CompPlace]&0x3FFF)>>Counter;
		//if(EnergyPlace<3){cout<<Counter<<"	"<<CompPlace<<"	"<<Temp<<"	"<< (CompressedWords[CompPlace]&0x3FFF) << endl;}
		switch (Ranges[EnergyPlace%16]){
		case 0:
			Mask = 0x0000;
			break;
		case 1:
			Mask = 0x0001;
			break;
		case 2:
			Mask = 0x0003;
			break;
		case 3:
			Mask = 0x0007;
			break;
		case 4: 
			Mask = 0x000F;
			break;
		case 5:
			Mask = 0x001F;
			break;
		case 6:
			Mask = 0x003F;
			break;
		/*case 7:
			Mask = 0x007F;
			break;
		case 8:
			Mask = 0x00FF;
			break;
		case 9:
			Mask = 0x01FF;
			break;
		case 10:
			Mask = 0x03FF;
			break;
		case 11:
			Mask = 0x01FF;
			break;
		case 12:
			Mask = 0x0FFF;
			break;
		case 13:
			Mask = 0x1FFF;
			break;*/
		case 14:
			Mask = 0x3FFF;
			break;
		}

		if (Counter+Ranges[EnergyPlace%16]<=14){
			DecompressedWords[k]=((Temp&Mask)+(CompressedWords[(EnergyPlace%16)+8]&0x3FFF));
			Counter+=Ranges[EnergyPlace%16];
			//if(EnergyPlace<3){cout<<Counter<<"	"<<(Temp&Mask)<<"	"<< (CompressedWords[(EnergyPlace%16)+8]&0x3FFF) << endl;}
			if(Counter==14){Counter=0; CompPlace++;}
			continue;
		}
		
		if (Counter+Ranges[EnergyPlace%16]>14){
			newCounter = Counter + Ranges[EnergyPlace%16] - 14;
			Temp2 = (CompressedWords[CompPlace+1]&0x3FFF) << (newCounter);
			DecompressedWords[k]=((Temp|Temp2)&Mask)+(CompressedWords[(EnergyPlace%16)+8]&0x3FFF);
			CompPlace++;
			Counter = newCounter;
			continue;
		}

	}
		for (int l = ALL_HEADERS; l < ENERGY_WORDS+ALL_HEADERS; l++){
			DecompressedWords[l]=(DecompressedWords[l]&0x3FFF)|(0x8000);
		}
		
		for (int w=0; w<COE_WORDS; w++) {
			DecompressedWords[w+ENERGY_WORDS+ALL_HEADERS]=DecompressedWords[w+CompPlace+ALL_HEADERS];
		}
		
}

int main(){
	//declare arrays
	uint16_t CompressedWords[ALL_WORDS] = {0};
	uint16_t DecompressedWords[ALL_WORDS] = {0};
	uint16_t HeaderArray[ALL_HEADERS]= {0};
	// declare int variables
	int BinPlace = 0;
	int TotalEvents = 8;
	int TotalCompWords;
	int CWords;
	
	// create file variables / filename variables
	char output_file[256]= "/home/Hanna/L2Code/run15703_node5_slot7_decompressed.bin";
	char input_file[256] = "/home/Hanna/L2Code/run15703_node5_slot7_compressed.bin";
	FILE * run_file;
	//FILE * output_packet_file; 
	run_file = fopen(input_file, "rb");
	FILE * out_file;
	out_file = fopen(output_file, "wb");	
	for (int i = 0; i < 1000; i ++){		
	// read the headers to find the number of compressed words to read.
		fread(HeaderArray, sizeof(uint16_t), ALL_HEADERS, run_file); 
		int N= sizeof(HeaderArray)/sizeof(HeaderArray[0]);
		FlipStuff(HeaderArray,N); // Flips the headers
		CWords = (((HeaderArray[2]&0x00FF)<<8)|(HeaderArray[3]&0x00FF))-6-COE_WORDS;
		// read the rest of the array - no need to rewind.
		fread(CompressedWords, sizeof(uint16_t), CWords+COE_WORDS, run_file);
		// Flip endianness: 
		N = CWords+COE_WORDS;
		FlipStuff(CompressedWords,N);
	// Make Room for headers: 
		for (int k = ALL_WORDS; k >= 40; k--){
			CompressedWords[k]=CompressedWords[k-40];
		}
	// Put headers in CompressedWords
		for (int j=0;j<ALL_HEADERS; j++){
			CompressedWords[j]=HeaderArray[j];
			if ((j<40)&&(j>=24)&&((HeaderArray[j]&0x3FFF)>6)){cout<<"Ranges more than 6 at packet #" << i << endl;}
		}
	// decompress
		/*for (int i = 0; i<56; i++){
			if(i%16==8){cout<<endl;}
			cout << (CompressedWords[i]&0x3FFF)<<"	";
		}*/
		Decompress(CompressedWords, DecompressedWords);
		for(int j = 0; j<6; j++){
			DecompressedWords[j+ALL_HEADERS+ENERGY_WORDS]=CompressedWords[j+CWords+ALL_HEADERS];
		}	
	// Flip back
		N = sizeof(DecompressedWords)/sizeof(DecompressedWords[0]);
		FlipStuff(DecompressedWords,N);
		fwrite(DecompressedWords, sizeof(uint16_t),ALL_WORDS, out_file);
	}
	
	return 0;}
