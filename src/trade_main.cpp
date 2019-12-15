#include <iostream>
#include "../include/trader.h"
#include "../strategy/dong_0_strategy.h"
using namespace std;

int main(int argc, char **argv)
{
	if(argc != 2){
		cout<<"****************************************\n";
		cout<<"Usage:\n\t"<<argv[0]<<" <config-file> !\n";
		cout<<"****************************************"<<endl;
		return 1;
	}
	cout<<"Let's go!"<<endl;
	//freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
	Trader *my_trader = Trader::GetTrader();
	Dong0Strategy dong_0_strategy(argc, argv);
	my_trader->run(&dong_0_strategy);

	return 0;
}
