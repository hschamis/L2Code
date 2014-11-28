// Decompress binary
// Created by Hanna Schamis 11/4/14
// Last edited by Hanna 11/28/14

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
void FindRows (uint16_t *HeaderArray, int &CompWords){
	/*int TotalBits = 0;
	int Rows;
	for (int j = 24; j < 40; j++){
		if((HeaderArray[j]&0x3FFF)!=0){
		TotalBits = TotalBits + HeaderArray[j];
	}
	Rows = TotalBits/14;
	if (TotalBits%14!=0){
		Rows++;
	}}
	CompWords=Rows*64;*/
	//Something different
}
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
	for (int i = 24; i<40; i++){
		Ranges[i-24]=(CompressedWords[(i-8)%16];
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
		case 7:
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
			break;
		case 14:
			Mask = 0x3FFF;
			break;
		}
		if (Counter+Ranges[EnergyPlace%16]<=14){
			DecompressedWords[k]=(Temp&Mask)+(CompressedWords[(EnergyPlace%16)+8]&0x3FFF);
			Counter+=Ranges[EnergyPlace%16];
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
	char output_file[256]= "/sudaq/jessimic/decompressed/run15703_node2_slot5_decompressed.bin";
	char input_file[256] = "/sudaq/jessimic/compressed/run15703_node2_slot5_compressed.bin";
	FILE * run_file;
	//FILE * output_packet_file; 
	run_file = fopen(input_file, "rb");
	FILE * out_file;
	out_file = fopen(output_file, "wb");	
	for (int i = 0; i < 10; i ++){		
	// read the headers to find the number of words to read.
		fread(HeaderArray, sizeof(uint16_t), ALL_HEADERS, run_file); 
		int N= sizeof(HeaderArray)/sizeof(HeaderArray[0]);
		FlipStuff(HeaderArray,N); // Flips the headers
		FindRows(HeaderArray,CWords); //<-- Gets # of words to read.
	// Put headers in CompressedWords
		for (int j=0;j<ALL_HEADERS; j++){
			CompressedWords[j]=HeaderArray[j];
		}
	// read the rest of the array
		rewind(run_file);
		fread(CompressedWords, sizeof(uint16_t), ALL_HEADERS+CWords+COE_WORDS, run_file);// NOT SURE HOW MANY WORDS IT SHOULD READ - does it start from where it left off?
	// Flip endianness: 
		N = ALL_HEADERS+CWords+COE_WORDS;
		FlipStuff(CompressedWords,N);
	// decompress
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
