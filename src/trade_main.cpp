#include <iostream>
#include <string>
#include <unistd.h>
#include <locale.h>
#include "../include/trader.h"
#include "../strategy/dong_0_strategy.h"
using namespace std;

int main(int argc, char **argv)
{
	setlocale(LC_ALL,"");

	if(argc != 2){
		cout<<"Usage: "<<argv[0]<<" <config-file> !";
		pause();
		return 1;
	}
	Trader *my_trader = Trader::GetTrader();
	my_trader->log("Let's go!");
	freopen("/dev/null", "w", stderr);
	Dong0Strategy dong_0_strategy(argc, argv);
	my_trader->run(&dong_0_strategy);

	return 0;
}
