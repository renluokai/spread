#include "forecast.h"
#include <iostream>
using namespace std;
//↑ ↓,←↑→
double Forecast::volumeRatio=0.0;
bool Forecast::
OrderWillSuccess(double price, Quote *qt, EOpenClose oc, ELongShort ls)
{
	return true;
	cout<<qt->InstrumentID<<" "<<qt->BidPrice1<<" "<<qt->BidVolume1<<" "<<qt->AskPrice1<<" "<<qt->AskVolume1<<endl;
	if(oc==E_OPEN){//open order
		if(ls==E_LONG){
			goto BUY;
		}else{
			goto SELL;
		}
	}else{//close order
		if(ls==E_LONG){
			goto SELL;
		}else{
			goto BUY;
		}
	}
BUY:
	if(price>=qt->AskPrice1 || price>qt->BidPrice1){
		return true;
	}
	if(price == qt->BidPrice1 
	&&qt->BidVolume1 < (int(qt->AskVolume1*volumeRatio)+1))
	{
		return true;
	}
	return false;
SELL:
	if(price<=qt->BidPrice1 || price<qt->AskPrice1){
		return true;
	}
	if(price == qt->AskPrice1
	&&qt->AskVolume1 < (int(qt->BidVolume1*volumeRatio)+1))
	{
		return true;
	}
	return false;
}
void Forecast::
QuoteDirection(Quote &qt)
{


}
