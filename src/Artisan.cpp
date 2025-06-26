// Artisan.cpp : Defines the entry point for the application.
//

#include "Artisan.h"
#include "UCI.h"
#include "Tuner.h"
using namespace std;

int main()
{
	BB::init();

	//Tuner tuner("quiet-labeled.epd");

	//pawn mappings
	uint8_t b_file[8];
	uint8_t b_rank[8];

	b_rank[1] = 0xFF;
	b_rank[6] = 0xFF;

	for (int i = 0; i < 8; i++) {
		b_file[i] = 0b01000010;
	}

	while (UCI::getInstance()->loop()) {

	}
	return 0;
}
