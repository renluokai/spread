#include <iostream>
#include <string.h>
#include <vector>

#include "instrument.h"
#include "../include/data_types.h"
using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))

Instrument*	Instrument::firstOpenIns = NULL;
Instrument*	Instrument::firstCloseIns = NULL;
Instrument*	Instrument::secondOpenIns = NULL;
Instrument*	Instrument::secondCloseIns = NULL;
Instrument*	Instrument::mainIns = NULL;
double 		Instrument::askSpread = 0.0;
double 		Instrument::bidSpread = 0.0;
double		Instrument::openThreshold = 0.0;
double		Instrument::closeThreshold = 0.0;
InsType		Instrument::openWith = E_INS_INVALID;
InsType		Instrument::closeWith = E_INS_INVALID; 

StopLoss	Instrument::stopLossType = E_STOPLOSS_NO;
int			Instrument::stopLoss = 0;
EDirection	Instrument::direction = E_DIR_INVALID;
int			Instrument::maxPosition = 0;
int			Instrument::submitMax = 0;
bool		Instrument::loop = true;

Instrument::Instrument(char *ins_name)
{
	STRCPY(name, ins_name);
	reached = false;
	lastQuote = new Quote;
	trader = Trader::GetTrader();
}

void Instrument::ShowState()
{
}

void Instrument::on_quote(Quote *q)
{
	//save the last quote
	*lastQuote = *q;
	if(reached == false){
		reached = true;
	}
	if(relativeIns->reached == false){
		return;
	}
	if(insType == E_INS_FORWARD){
		CalcSpread(false);
	}else{
		CalcSpread();
	}
	ShowQuote();

	if(direction == E_DIR_UP)
	{
		int lockedPosition = CalcLockedPosition(mainIns->name, mainIns->relativeIns->name, direction);
		{
			char buffer[256]={0};
			
printf(buffer, "Locked position is %d\n", lockedPosition);
			trader->log(buffer);
		}
		if(lockedPosition ==0){
			if(bidSpread <= openThreshold){
				//full the open condition
				FullOpenLong(lockedPosition);
			}
			else{
				//don't full open condition, cancel open orders
				DoNotFullOpenLong();
			}
		}
		else{
			//has locked position
			
			//full the close condition
			if(askSpread >= closeThreshold)
			{
				FullCloseLong(lockedPosition);
			}
			//full the open condition
			if(bidSpread <= openThreshold)
			{
				FullOpenLong(lockedPosition);
			}
		}
	}
}
void Instrument::ShowQuote()
{
	char buffer[256]={0};
	sprintf(buffer,"Q %s %s %.5f %d %.5f %d %.5f %.5f\n",
			insType==E_INS_FORWARD?"F":"R",
			lastQuote->InstrumentID,
			lastQuote->AskPrice1,
			lastQuote->AskVolume1,
			lastQuote->BidPrice1,
			lastQuote->BidVolume1,
			askSpread,
			bidSpread);
	trader->log(buffer);
}
void Instrument::CalcSpread(bool rct)
{
	if(rct==false){
		bidSpread = lastQuote->BidPrice1 - relativeIns->lastQuote->BidPrice1;
		askSpread = lastQuote->AskPrice1 - relativeIns->lastQuote->AskPrice1;
	}else{
		bidSpread = relativeIns->lastQuote->BidPrice1 - lastQuote->BidPrice1;
		askSpread = relativeIns->lastQuote->AskPrice1 - lastQuote->AskPrice1;
	}
}
void Instrument::on_match(Order* o)
{
	if(o->open_close == E_OPEN){
	//open 
		if(firstOpenIns == this){
			//first leg, need to lock position
			const char* nm = secondOpenIns->name;
			double px = o->long_short == E_LONG ? secondOpenIns->lastQuote->BidPrice1 :secondOpenIns->lastQuote->AskPrice1;
			int vol = o->match_volume;
			Order* o = trader->NewOrder(nm, px, vol, E_OPEN, o->long_short == E_LONG ? E_SHORT : E_LONG);
			trader->submit_order(o);
		}else{
			//second leg, do nothing
		}
	}else{
	//close
		if(firstCloseIns == this){
			//first leg, need to lock position
			const char* nm = secondCloseIns->name;
			double px = o->long_short == E_LONG ? secondCloseIns->lastQuote->AskPrice1 :secondCloseIns->lastQuote->BidPrice1;
			int vol = o->match_volume;
			//TODO
			//need to calc today position and yesterday position
			Order* o = trader->NewOrder(nm, px, vol, E_CLOSE, o->long_short == E_LONG ? E_SHORT : E_LONG);
			trader->submit_order(o);
		}else{
			//second leg, do nothing
		}
	}
}

void Instrument::on_reject(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

void Instrument::on_cancel(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

void Instrument::on_insert(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

int Instrument::CalcLockedPosition(const char* main, const char* second, EDirection dir)
{
	int m=0,s=0;
	if(dir == E_DIR_UP)	{
		m = trader->GetLongPosition(main);
		s = trader->GetShortPosition(second);
		return min(m,s);
	}else{
		m = trader->GetShortPosition(main);
		s = trader->GetLongPosition(second);
		return min(m,s);
	}
}

void Instrument::CancelOrders(vector<Order*> &ods)
{
	vector<Order*>::iterator iter = ods.begin();
	for(; iter != ods.end(); iter++){
		if((*iter)->canceling == false){
			trader->cancel_order(*iter);
		}
	}
}

void Instrument::FullOpenLong(int lockedPosition)
{
	if(mainIns==firstOpenIns){
		//open from main instrument
		vector<Order*> ods;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstOpenIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			int remaindVolume = maxPosition - lockedPosition;
			if(remaindVolume<=0){
				trader->log("Don't open new position\n");
			}else{
				const char* nm = firstOpenIns->name;
				double price = firstOpenIns->lastQuote->BidPrice1;
				int vol = remaindVolume/submitMax>=1?submitMax:remaindVolume%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				Order* o = trader->NewOrder(nm, price, vol, E_OPEN, E_LONG);
				trader->submit_order(o);
			}
		}
	}else{
		//open from second instrument
		vector<Order*> ods;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstOpenIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			int remaindVolume = maxPosition - lockedPosition;
			if(remaindVolume<=0){
				trader->log("Don't open new position\n");
			}else{
				const char* nm = firstOpenIns->name;
				double price = firstOpenIns->lastQuote->AskPrice1;
				int vol = remaindVolume/submitMax>=1?submitMax:remaindVolume%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				Order* o =trader->NewOrder(nm, price, vol, E_OPEN, E_SHORT);
				trader->submit_order(o);
			}
		}
	}
}

void Instrument::DoNotFullOpenLong()
{
	if(mainIns==firstOpenIns){
		vector<Order*> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}
	}
}

void Instrument::FullCloseLong(int lockedPosition)
{
	if(mainIns==firstCloseIns){
		//close from main instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE, E_SHORT, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE, E_LONG, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			if(lockedPosition<=0){
				trader->log("Has no position to close\n");
			}else{
				const char* nm = firstCloseIns->name;
				double price = firstCloseIns->lastQuote->AskPrice1;
				int vol = lockedPosition/submitMax>=1?submitMax:lockedPosition%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				Order* o =trader->NewOrder(nm, price, vol, E_CLOSE, E_LONG);
				trader->submit_order(o);
			}
		}
	}else{
		//close from second instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE, E_LONG, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE, E_SHORT, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			if(lockedPosition<=0){
				trader->log("Has no position to close\n");
			}else{
				const char* nm = firstCloseIns->name;
				double price = firstCloseIns->lastQuote->BidPrice1;
				int vol = lockedPosition/submitMax>=1?submitMax:lockedPosition%submitMax;
				if(vol==0){
					trader->log("Can't open volume 0 close order\n");
					return;
				}
				Order* o = trader->NewOrder(nm, price, vol, E_CLOSE, E_SHORT);
				trader->submit_order(o);
			}
		}
	}
}
