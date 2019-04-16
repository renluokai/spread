#include <iostream>
#include "../include/trader.h"
#include "../strategy/dong_0_strategy.h"
using namespace std;

int main(int argc, char **argv)
{
	if(argc != 2){
		cout<<"Usage:\n\t"<<argv[0]<<" <config file> !"<<endl;
	}
	cout<<"Let's go!"<<endl;

	Trader *my_trader = Trader::GetTrader();
	Dong0Strategy dong_0_strategy(argc, argv);
	my_trader->run(&dong_0_strategy);

	return 0;
}
