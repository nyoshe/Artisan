// Artisan.cpp : Defines the entry point for the application.
//

#include "Artisan.h"
#include "UCI.h"
#include "Tuner.h"
using namespace std;

int main()
{

	//Tuner tuner("quiet-labeled.epd");
	BB::init();
	while (true) {
		UCI::getInstance()->loop();
	}
	return 0;
}
