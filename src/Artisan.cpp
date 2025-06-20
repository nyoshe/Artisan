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


	while (UCI::getInstance()->loop()) {

	}
	return 0;
}
