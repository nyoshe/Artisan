// Artisan.cpp : Defines the entry point for the application.
//

#include "Artisan.h"
#include "UCI.h"
using namespace std;

int main()
{
	BB::init();
	while (true) {
		UCI::getInstance()->loop();
	}
	return 0;
}
