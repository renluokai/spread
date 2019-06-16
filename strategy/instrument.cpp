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
bool		Instrument::needToStopLoss=false;

Instrument::Instrument(char *ins_name)
{
	STRCPY(name, ins_name);
	reached = false;
	lastQuote = new Quote;
	trader = Trader::GetTrader();
	priceTick = 0.0;
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
			//check stoploss and do respective action
			CheckStopLoss();	
			//full the close condition
			if(askSpread >= closeThreshold)
			{
				FullCloseLong(lockedPosition);
			}else{
				//don't full close condition, cancel close orders
				DoNotFullCloseLong();
			}
			//full the open condition
			if(bidSpread <= openThreshold)
			{
				FullOpenLong(lockedPosition);
			}else{
				//don't full open condition, cancel open orders
				DoNotFullOpenLong();
			}
		}
	}else if(direction == E_DIR_DOWN){
		int lockedPosition = CalcLockedPosition(mainIns->name, mainIns->relativeIns->name, direction);
		{
			char buffer[256]={0};
			printf(buffer, "Locked position is %d\n", lockedPosition);
			trader->log(buffer);
		}
		if(lockedPosition ==0){
			if(askSpread >= openThreshold){
				//full the open condition
				FullOpenShort(lockedPosition);
			}
			else{
				//don't full open condition, cancel open orders
				DoNotFullOpenShort();
			}
		}
		else{
			//has locked position
			//check stoploss and do respective action
			CheckStopLoss();	
			//full the close condition
			if(bidSpread <= closeThreshold)
			{
				FullCloseShort(lockedPosition);
			}else{
				DoNotFullCloseLong();
			}
			//full the open condition
			if(askSpread >= openThreshold)
			{
				FullOpenShort(lockedPosition);
			}else{
				DoNotFullOpenShort();
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
			lastQuote->BidPrice1,
			lastQuote->BidVolume1,
			lastQuote->AskPrice1,
			lastQuote->AskVolume1,
			bidSpread,
			askSpread);
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
			Order* new_order = trader->NewOrder(nm, px, vol, E_OPEN, o->long_short == E_LONG ? E_SHORT : E_LONG);
			trader->submit_order(new_order);
		}else{
			//second leg, do nothing
		}
	}else{
	//close
		if(firstCloseIns == this){
			//first leg, need to lock position
			trader->log("***************************************************\n");
			trader->log("first close leg matched\n");
			trader->log("***************************************************\n");
			o->ShowOrder();
			const char* nm = secondCloseIns->name;
			double px = o->long_short == E_LONG ? secondCloseIns->lastQuote->AskPrice1 :secondCloseIns->lastQuote->BidPrice1;
			int vol = o->match_volume;
			//TODO
			//need to calc today position and yesterday position
			Order* new_order = trader->NewOrder(nm, px, vol, o->open_close, o->long_short == E_LONG ? E_SHORT : E_LONG);
			trader->submit_order(new_order);
		}else{
			//second leg, do nothing
		}
	}
}

void Instrument::on_reject(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

void Instrument::on_cancel(Order* o)
{
	trader->log(__FUNCTION__);trader->log("\n");
	if(secondOpenIns == this
	|| secondCloseIns == this){
		o->ShowOrder();
		double price = 0.0; 
		if(o->open_close == E_OPEN){
			if(o->long_short == E_LONG){
				price = lastQuote->AskPrice1;
			}else{
				price = lastQuote->BidPrice1;
			}
		}else{
			if(o->long_short == E_LONG){
				price = lastQuote->BidPrice1;
			}else{
				price = lastQuote->AskPrice1;
			}
		}
		Order* new_order = trader->NewOrder(name, price, o->canceled_volume, o->open_close, o->long_short);
		trader->submit_order(new_order);
	}

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
int Instrument::CalcLockedPositionYesterday(const char* main, const char* second, EDirection dir)
{
	int m=0,s=0;
	if(dir == E_DIR_UP)	{
		m = trader->GetLongPositionYesterday(main);
		s = trader->GetShortPositionYesterday(second);
		return min(m,s);
	}else{
		m = trader->GetShortPositionYesterday(main);
		s = trader->GetLongPositionYesterday(second);
		return min(m,s);
	}
}

int Instrument::CalcLockedPositionToday(const char* main, const char* second, EDirection dir)
{
	int m=0,s=0;
	if(dir == E_DIR_UP)	{
		m = trader->GetLongPositionToday(main);
		s = trader->GetShortPositionToday(second);
		return min(m,s);
	}else{
		m = trader->GetShortPositionToday(main);
		s = trader->GetLongPositionToday(second);
		return min(m,s);
	}
}
void Instrument::CancelOrders(vector<Order*> &ods)
{
	vector<Order*>::iterator iter = ods.begin();
	for(; iter != ods.end(); iter++){
		if((*iter)->canceling == false
		&& (*iter)->state != E_ORIGINAL){
			trader->cancel_order(*iter);
		}
	}
}

void Instrument::FullOpenLong(int lockedPosition)
{
	if(needToStopLoss==true && lockedPosition>0){
		return;
	}
	trader->log(__FUNCTION__);trader->log("\n");
	if(mainIns==firstOpenIns){
		//open from main instrument
		vector<Order*> ods;
		vector<Order*>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
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
		vector<Order*>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
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
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
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
				int vol = 0;
				EOpenClose oc;
				int lockedPositionYesterday = CalcLockedPositionYesterday(mainIns->name, mainIns->relativeIns->name,direction);
				int lockedPositionToday = CalcLockedPositionToday(mainIns->name, mainIns->relativeIns->name,direction);
				if(lockedPositionToday>0){
					vol = lockedPositionToday/submitMax>=1?submitMax:lockedPositionToday%submitMax;
					oc  = E_CLOSE_T;
				}else if(lockedPositionYesterday>0){
					vol = lockedPositionYesterday/submitMax>=1?submitMax:lockedPositionYesterday%submitMax;
					oc  = E_CLOSE_Y;
				}
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				Order* o =trader->NewOrder(nm, price, vol, oc, E_LONG);
				trader->submit_order(o);
			}
		}
	}else{
		//close from second instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
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
				int vol = 0;
				EOpenClose oc;
				int lockedPositionYesterday = CalcLockedPositionYesterday(mainIns->name, mainIns->relativeIns->name,direction);
				int lockedPositionToday = CalcLockedPositionToday(firstCloseIns->name, firstCloseIns->relativeIns->name,direction);
				if(lockedPositionToday>0){
					vol = lockedPositionToday/submitMax>=1?submitMax:lockedPositionToday%submitMax;
					oc  = E_CLOSE_T;
				}else if(lockedPositionYesterday>0){
					vol = lockedPositionYesterday/submitMax>=1?submitMax:lockedPositionYesterday%submitMax;
					oc  = E_CLOSE_Y;
				}
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				Order* o = trader->NewOrder(nm, price, vol, oc, E_SHORT);
				trader->submit_order(o);
			}
		}
	}
}
void Instrument::DoNotFullCloseLong()
{
	if(mainIns==firstCloseIns){
		vector<Order*> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}
}

void Instrument::FullOpenShort(int lockedPosition)
{
	trader->log(__FUNCTION__);trader->log("\n");
	if(needToStopLoss==true && lockedPosition>0){
		return;
	}
	if(mainIns==firstOpenIns){
		//open from main instrument
		vector<Order*> ods;
		vector<Order*>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
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
				Order* o = trader->NewOrder(nm, price, vol, E_OPEN, E_SHORT);
				trader->submit_order(o);
			}
		}
	}else{
		//open from second instrument
		vector<Order*> ods;
		vector<Order*>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
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
				Order* o =trader->NewOrder(nm, price, vol, E_OPEN, E_LONG);
				trader->submit_order(o);
			}
		}
	}
}

void Instrument::DoNotFullOpenShort()
{
	if(mainIns==firstOpenIns){
		vector<Order*> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}
	}
}

void Instrument::FullCloseShort(int lockedPosition)
{
	if(mainIns==firstCloseIns){
		//close from main instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
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
				int vol = 0;
				EOpenClose oc;
				int lockedPositionYesterday = CalcLockedPositionYesterday(mainIns->name, mainIns->relativeIns->name,direction);
				int lockedPositionToday = CalcLockedPositionToday(mainIns->name, mainIns->relativeIns->name,direction);
				if(lockedPositionToday){
					vol = lockedPositionToday/submitMax>=1?submitMax:lockedPositionToday%submitMax;
					oc = E_CLOSE_T;
				}else if(lockedPositionYesterday){
					vol = lockedPositionYesterday/submitMax>=1?submitMax:lockedPositionYesterday%submitMax;
					oc = E_CLOSE_Y;
				}
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				Order* o =trader->NewOrder(nm, price, vol, oc, E_SHORT);
				trader->submit_order(o);
			}
		}
	}else{
		//close from second instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()!=0){
			return;
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()>0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
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
				int vol = 0;
				EOpenClose oc;
				int lockedPositionYesterday = CalcLockedPositionYesterday(mainIns->name, mainIns->relativeIns->name,direction);
				int lockedPositionToday = CalcLockedPositionToday(mainIns->name, mainIns->relativeIns->name,direction);
				if(lockedPositionToday){
					vol = lockedPositionToday/submitMax>=1?submitMax:lockedPositionToday%submitMax;
					oc = E_CLOSE_T;
				}else if(lockedPositionYesterday){
					vol = lockedPositionYesterday/submitMax>=1?submitMax:lockedPositionYesterday%submitMax;
					oc = E_CLOSE_Y;
				}
				if(vol==0){
					trader->log("Can't open volume 0 close order\n");
					return;
				}
				Order* o = trader->NewOrder(nm, price, vol, oc, E_LONG);
				trader->submit_order(o);
			}
		}
	}
}
void Instrument::DoNotFullCloseShort()
{
	if(mainIns==firstCloseIns){
		vector<Order*> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}
}
void Instrument::CheckStopLoss()
{
	int lockedPositionYesterday = CalcLockedPositionYesterday(mainIns->name, mainIns->relativeIns->name,direction);
	int lockedPositionToday = CalcLockedPositionToday(mainIns->name, mainIns->relativeIns->name,direction);
	double tradedSpread = 0.0;

	const char *f, *r;
	bool forwardIsMain;
	ELongShort ls;
	if(insType==E_INS_FORWARD){
		f = this->name;
		r = this->relativeIns->name;
		if(this==mainIns){
			forwardIsMain = true;
		}else{
			forwardIsMain = false;
		}
	}else{
		r = this->name;
		f = this->relativeIns->name;
		if(this==mainIns){
			forwardIsMain = false;
		}else{
			forwardIsMain = true;
		}
	}
	ls = direction==E_DIR_UP?E_LONG:E_SHORT;

	if(lockedPositionYesterday>0){
		tradedSpread = 
		trader->GetAverageSpread(f, r,
								lockedPositionYesterday, lockedPositionToday, 
								ls, forwardIsMain);
	}else{
		tradedSpread = trader->GetHeadSpread(f, r, ls, forwardIsMain);
	}

	vector<Order*> ods;
	vector<Order*>::iterator iter;
	if(direction == E_DIR_UP){
		if((bidSpread + stopLoss*priceTick) <= tradedSpread){
			trader->log("let's stop loss");
			needToStopLoss = true;

			if(mainIns == firstOpenIns){
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
			}else{
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
			}
			if(ods.size()>0){
				trader->log("Wait for second leg match\n");
				return;
			}
			if(mainIns == firstOpenIns){
				trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
			}else{
				trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
			}
			if(ods.size()==0){
				const char* nm = firstCloseIns->name;
				double px = 0.0;
				EOpenClose oc;
				ELongShort ls;
				int vol = 0;
				if(mainIns == firstCloseIns){
					ls = E_LONG;
					px = firstCloseIns->lastQuote->BidPrice1;
				}else{
					ls = E_SHORT;
					px = firstCloseIns->lastQuote->AskPrice1;
				}
				if(lockedPositionToday > 0){
					oc = E_CLOSE_T;
					vol = lockedPositionToday/submitMax>0?submitMax:lockedPositionToday%submitMax;
				}else if(lockedPositionYesterday>0){
					oc = E_CLOSE_Y;
					vol = lockedPositionYesterday/submitMax>0?submitMax:lockedPositionYesterday%submitMax;
				}
				trader->GetOrder(firstCloseIns->name, oc, ls, ods);
				if(ods.size()==0){
					if(vol>0){
						Order* new_order = trader->NewOrder(nm, px, vol, oc, ls);
						trader->submit_order(new_order);
						return;
					}
				}else{
					//check to update price
					for(iter==ods.begin(); iter!=ods.end();iter++){
						if(mainIns == firstCloseIns){
							if((*iter)->submit_price > firstCloseIns->lastQuote->BidPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}else{
							if((*iter)->submit_price < firstCloseIns->lastQuote->AskPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}
					}
				}
			}else{
				//cancel open orders to prepare to stoploss	
				CancelOrders(ods);
			}			
		}else{
			if(needToStopLoss == true){
				if(mainIns == firstCloseIns){
					trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
					trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
					CancelOrders(ods);
				}else{
					trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
					trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
					CancelOrders(ods);
				}
			}
			needToStopLoss = false;
		}
	}else{
		if((askSpread - stopLoss*priceTick) >= tradedSpread){
			trader->log("let's stop loss");
			needToStopLoss = true;
			vector<Order*> ods;
			vector<Order*>::iterator iter;

			if(mainIns == firstOpenIns){
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
			}else{
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
			}
			if(ods.size()>0){
				trader->log("Wait for second leg match\n");
				return;
			}
			//check if has open order, if have cancel
			if(mainIns == firstOpenIns){
				trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
			}else{
				trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
			}
			if(ods.size()==0){
				const char* nm = firstCloseIns->name;
				double px = 0.0;
				EOpenClose oc;
				ELongShort ls;
				int vol = 0;
				if(mainIns == firstCloseIns){
					ls = E_SHORT;
					px = firstCloseIns->lastQuote->AskPrice1;
				}else{
					ls = E_LONG;
					px = firstCloseIns->lastQuote->BidPrice1;
				}
				if(lockedPositionToday > 0){
					oc = E_CLOSE_T;
					vol = lockedPositionToday/submitMax>0?submitMax:lockedPositionToday%submitMax;
				}else if(lockedPositionYesterday>0){
					oc = E_CLOSE_Y;
					vol = lockedPositionYesterday/submitMax>0?submitMax:lockedPositionYesterday%submitMax;
				}
				trader->GetOrder(firstCloseIns->name, oc, ls, ods);
				if(ods.size()==0){
					if(vol>0){
						Order* new_order = trader->NewOrder(nm, px, vol, oc, ls);
						trader->submit_order(new_order);
						return;
					}
				}else{
					//check to update price
					for(iter==ods.begin(); iter!=ods.end();iter++){
						if(mainIns == firstCloseIns){
							if((*iter)->submit_price < firstCloseIns->lastQuote->AskPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}else{
							if((*iter)->submit_price > firstCloseIns->lastQuote->BidPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}
					}
				}
			}else{
				//cancel open orders to prepare to stoploss	
				CancelOrders(ods);
			}			
		}else{
			if(needToStopLoss == true){
				if(mainIns == firstCloseIns){
					trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
					trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
					CancelOrders(ods);
				}else{
					trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
					trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
					CancelOrders(ods);
				}
			}
			needToStopLoss = false;
		}
	}
}
