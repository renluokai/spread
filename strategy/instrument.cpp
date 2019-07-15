#include <fstream>
#include <iostream>
#include <string.h>
#include <vector>

#include "instrument.h"
#include "forecast.h"
#include "../include/data_types.h"
using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))

list<LockedSpread> 	Instrument::lockedSpreadY;
list<LockedSpread> 	Instrument::lockedSpreadT;
list<MatchInfo> 	Instrument::firstOpenMatch;
list<MatchInfo> 	Instrument::firstCloseMatch;
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
InsType		Instrument::triggerSpread = E_INS_INVALID; 

StopLoss	Instrument::stopLossType = E_STOPLOSS_NO;
int			Instrument::stopLoss = 0;
EDirection	Instrument::direction = E_DIR_INVALID;
int			Instrument::maxPosition = 0;
int			Instrument::openCount=0;
int			Instrument::submitMax = 0;
bool		Instrument::loop = true;
bool		Instrument::needToStopLoss=false;
vector<int>	Instrument::openTime;
vector<int>	Instrument::closeTime;

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

	if(insType == triggerSpread){
		if(insType == E_INS_FORWARD){
			CalcSpread(false);
		}else{
			CalcSpread();
		}
	}
	ShowQuote();

	if(direction == E_DIR_UP)
	{
		int lockedPosition = CalcLockedPosition();
		{
			char buffer[256]={0};
			
			sprintf(buffer, "Locked position is %d\n", lockedPosition);
			trader->log(buffer);
		}
		if(lockedPosition ==0){
			if(IsOpenLong()){
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
			if(IsCloseLong())
			{
				FullCloseLong(lockedPosition);
			}else{
				//don't full close condition, cancel close orders
				DoNotFullCloseLong();
			}
			//full the open condition
			if(IsOpenLong())
			{
				FullOpenLong(lockedPosition);
			}else{
				//don't full open condition, cancel open orders
				DoNotFullOpenLong();
			}
		}
	}else if(direction == E_DIR_DOWN){
		int lockedPosition = CalcLockedPosition();
		{
			char buffer[256]={0};
			sprintf(buffer, "Locked position is %d\n", lockedPosition);
			trader->log(buffer);
		}
		if(lockedPosition ==0){
			if(IsOpenShort()){
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
			if(IsCloseShort())
			{
				FullCloseShort(lockedPosition);
			}else{
				DoNotFullCloseShort();
			}
			//full the open condition
			if(IsOpenShort())
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
			MatchInfo matchInfo;
			matchInfo.date = o->date;
			matchInfo.volume = o->match_volume;
			matchInfo.price = o->match_price;
			firstOpenMatch.push_back(matchInfo);
			openCount += o->match_volume;
		}else{
			//second leg, need to update lockedSpread
			const char *targetFile="position.cfg";
			FILE *tmp=fopen(targetFile,"a");

			LockedSpread lockedSpread;
			int reminder=o->match_volume;
			while(reminder>0){
				if(firstOpenMatch.front().volume > reminder){
					firstOpenMatch.front().volume -= reminder;
					lockedSpread.date = o->date;
					lockedSpread.volume = reminder;
					double spread=0.0;
					if(insType==E_INS_FORWARD){
						spread = o->match_price - firstOpenMatch.front().price;
					}else{
						spread = firstOpenMatch.front().price - o->match_price;
					}
					lockedSpread.spread = spread;
					lockedSpreadT.push_back(lockedSpread);
				}else{
					reminder -= firstOpenMatch.front().volume;
					lockedSpread.date= o->date;
					lockedSpread.volume = firstOpenMatch.front().volume;
					double spread=0.0;
					if(insType==E_INS_FORWARD){
						spread = o->match_price - firstOpenMatch.front().price;
					}else{
						spread = firstOpenMatch.front().price - o->match_price;
					}
					lockedSpread.spread = spread;
					lockedSpreadT.push_back(lockedSpread);
					char buffer[128]={0};
					sprintf(buffer,"%d %f %d\n",lockedSpread.date, lockedSpread.spread, lockedSpread.volume);
					trader->log(buffer);
					fwrite(buffer, strlen(buffer),1,tmp);
					fflush(tmp);
					firstOpenMatch.pop_front();	
				}
			}
			fclose(tmp);
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

			Order* new_order = trader->NewOrder(nm, px, vol, o->open_close, o->long_short == E_LONG ? E_SHORT : E_LONG);
			new_order->stop_loss = o->stop_loss;
			if(o->stop_loss == true){
				openCount += o->match_volume;
			}
			trader->submit_order(new_order);
			MatchInfo matchInfo;
			matchInfo.date = o->date;
			matchInfo.volume = o->match_volume;
			matchInfo.price = o->match_price;
			firstCloseMatch.push_back(matchInfo);
		}else{
			//second leg, need to update lockedSpread
			LockedSpread lockedSpread;
			int reminder=o->match_volume;
			while(reminder>0){
				if(firstCloseMatch.front().volume > reminder){
					firstCloseMatch.front().volume -= reminder;
					lockedSpread.date = o->date;
					lockedSpread.volume = reminder;
					double spread=0.0;
					if(insType==E_INS_FORWARD){
						spread = o->match_price - firstCloseMatch.front().price;
					}else{
						spread = firstCloseMatch.front().price - o->match_price;
					}
					lockedSpread.spread = spread;
				}else{
					reminder -= firstCloseMatch.front().volume;
					lockedSpread.date= o->date;
					lockedSpread.volume = firstCloseMatch.front().volume;
					double spread=0.0;
					if(insType==E_INS_FORWARD){
						spread = o->match_price - firstCloseMatch.front().price;
					}else{
						spread = firstCloseMatch.front().price - o->match_price;
					}
					lockedSpread.spread = spread;
					firstCloseMatch.pop_front();	
				}
				UpdateLockedSpread(lockedSpread, 
								o->stop_loss?true:false,
								o->open_close==E_CLOSE_Y?false:true);
			}

			int lockedPosition = CalcLockedPosition();
			if(needToStopLoss==true && lockedPosition==0){
				needToStopLoss=false;
			}
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
		new_order->stop_loss = o->stop_loss;
		trader->submit_order(new_order);
	}

}

void Instrument::on_insert(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

int Instrument::CalcLockedPosition()
{
	list<LockedSpread>::iterator iter;

	int y=0;
	int t=0;
	for(iter=lockedSpreadY.begin();
		iter!=lockedSpreadY.end();
		iter++)
	{
		y+=iter->volume;
	}

	for(iter=lockedSpreadT.begin();
		iter!=lockedSpreadT.end();
		iter++)
	{
		t+=iter->volume;
	}
	return y+t;
}
int Instrument::CalcLockedPositionYesterday()
{
	list<LockedSpread>::iterator iter;

	int y=0;
	for(iter=lockedSpreadY.begin();
		iter!=lockedSpreadY.end();
		iter++)
	{
		y+=iter->volume;
	}
	return y;
}

int Instrument::CalcLockedPositionToday()
{
	list<LockedSpread>::iterator iter;

	int t=0;
	for(iter=lockedSpreadT.begin();
		iter!=lockedSpreadT.end();
		iter++)
	{
		t+=iter->volume;
	}
	return t;
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
	trader->log(__FUNCTION__);trader->log("\n");
	if(E_INS_FORWARD==firstOpenIns->insType){
		//open from forward instrument
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

		if(needToStopLoss==true && lockedPosition>0){
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
			//int remaindVolume = maxPosition - lockedPosition;
			int remaindVolume = maxPosition - openCount;
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
				bool forecast=true;
				if(IsForecast(E_OPEN,E_LONG)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->lastQuote, E_OPEN, E_LONG);
				}
				if(forecast){
					Order* o = trader->NewOrder(nm, price, vol, E_OPEN, E_LONG);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//open from recent instrument
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

		if(needToStopLoss==true && lockedPosition>0){
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
			//int remaindVolume = maxPosition - lockedPosition;
			int remaindVolume = maxPosition - openCount;
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
				bool forecast=true;
				if(IsForecast(E_OPEN,E_SHORT)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->lastQuote, E_OPEN, E_SHORT);
				}
				if(forecast){
					Order* o =trader->NewOrder(nm, price, vol, E_OPEN, E_SHORT);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}
}

void Instrument::DoNotFullOpenLong()
{
	if(E_INS_FORWARD==firstOpenIns->insType){
		vector<Order*> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}
	}
}

void Instrument::FullCloseLong(int lockedPosition)
{
	if(E_INS_FORWARD==firstCloseIns->insType){
		//close from forward instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()!=0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
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
				int lockedPositionYesterday = CalcLockedPositionYesterday();
				int lockedPositionToday = CalcLockedPositionToday();
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
				bool forecast=true;
				if(IsForecast(oc,E_LONG)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstCloseIns->lastQuote, oc, E_LONG);
				}
				if(forecast){
					Order* o =trader->NewOrder(nm, price, vol, oc, E_LONG);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//close from recent instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()!=0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
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
				int lockedPositionYesterday = CalcLockedPositionYesterday();
				int lockedPositionToday = CalcLockedPositionToday();
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
				bool forecast=true;
				if(IsForecast(oc,E_SHORT)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstCloseIns->lastQuote, oc, E_SHORT);
				}
				if(forecast){
					Order* o = trader->NewOrder(nm, price, vol, oc, E_SHORT);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}
}
void Instrument::DoNotFullCloseLong()
{
	if(E_INS_FORWARD==firstCloseIns->insType){
		vector<Order*> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}
}

void Instrument::FullOpenShort(int lockedPosition)
{
	trader->log(__FUNCTION__);trader->log("\n");
	if(E_INS_FORWARD==firstOpenIns->insType){
		//open from forward instrument
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

		if(needToStopLoss==true && lockedPosition>0){
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
			//int remaindVolume = maxPosition - lockedPosition;
			int remaindVolume = maxPosition - openCount;
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
				bool forecast=true;	
				if(IsForecast(E_OPEN,E_SHORT)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->lastQuote, E_OPEN, E_SHORT);
				}
				if(forecast){
					Order* o = trader->NewOrder(nm, price, vol, E_OPEN, E_SHORT);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//open from recent instrument
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

		if(needToStopLoss==true && lockedPosition>0){
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
			//int remaindVolume = maxPosition - lockedPosition;
			int remaindVolume = maxPosition - openCount;
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
				bool forecast=true;
				if(IsForecast(E_OPEN,E_LONG)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->lastQuote, E_OPEN, E_LONG);
				}
				if(forecast){
					Order* o =trader->NewOrder(nm, price, vol, E_OPEN, E_LONG);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}
}

void Instrument::DoNotFullOpenShort()
{
	if(E_INS_FORWARD==firstOpenIns->insType){
		vector<Order*> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()==0){
			return;
		}else{
			CancelOrders(ods);
		}
	}
}

void Instrument::FullCloseShort(int lockedPosition)
{
	if(E_INS_FORWARD==firstCloseIns->insType){
		//close from forward instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()!=0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->lastQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
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
				int lockedPositionYesterday = CalcLockedPositionYesterday();
				int lockedPositionToday = CalcLockedPositionToday();
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
				bool forecast=true;
				if(IsForecast(oc,E_SHORT)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstCloseIns->lastQuote, oc, E_SHORT);
				}
				if(forecast){
					Order* o =trader->NewOrder(nm, price, vol, oc, E_SHORT);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//close from recent instrument
		vector<Order*> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()!=0){
			vector<Order*>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->lastQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
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
				int lockedPositionYesterday = CalcLockedPositionYesterday();
				int lockedPositionToday = CalcLockedPositionToday();
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
				bool forecast=true;
				if(IsForecast(oc,E_LONG)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstCloseIns->lastQuote, oc, E_LONG);
				}
				if(forecast){
					Order* o = trader->NewOrder(nm, price, vol, oc, E_LONG);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}
}
void Instrument::DoNotFullCloseShort()
{
	if(E_INS_FORWARD==firstCloseIns->insType){
		vector<Order*> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}else{
		vector<Order*> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()==0){
		}else{
			CancelOrders(ods);
		}			
	}
}
void Instrument::CheckStopLoss()
{
	int lockedPositionYesterday = CalcLockedPositionYesterday();
	int lockedPositionToday = CalcLockedPositionToday();
	double tradedSpread = 0.0;
	if(stopLossType == E_STOPLOSS_AVERAGE){
		tradedSpread = GetAverageSpread();
	}else if(stopLossType == E_STOPLOSS_TICKBYTICK){
		tradedSpread = GetBadSpread();
	}else{
		return;
	}

	vector<Order*> ods;
	vector<Order*>::iterator iter;
	if(direction == E_DIR_UP){
		if(IsStopLoss(tradedSpread)){
			trader->log("let's stop loss");
			needToStopLoss = true;

			if(E_INS_FORWARD == firstOpenIns->insType){
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
			}else{
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
			}
			if(ods.size()>0){
				trader->log("Wait for second leg match\n");
				return;
			}
			if(E_INS_FORWARD == firstOpenIns->insType){
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
				if(E_INS_FORWARD == firstCloseIns->insType){
					ls = E_LONG;
					px = firstCloseIns->lastQuote->AskPrice1;
				}else{
					ls = E_SHORT;
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
				//if there is no close order,just submit it
				if(ods.size()==0){
					if(vol>0){
						Order* new_order = trader->NewOrder(nm, px, vol, oc, ls);
						trader->submit_order(new_order);
						return;
					}
				}else{
					//check to update the close orders' price
					for(iter=ods.begin(); iter!=ods.end();iter++){
						if(E_INS_FORWARD == firstCloseIns->insType){
							if((*iter)->submit_price > firstCloseIns->lastQuote->AskPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}else{
							if((*iter)->submit_price < firstCloseIns->lastQuote->BidPrice1
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
				if(E_INS_FORWARD == firstCloseIns->insType){
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
		//do E_DIR_DOWN stop loss check
		if(IsStopLoss(tradedSpread)){
			trader->log("let's stop loss");
			needToStopLoss = true;
			vector<Order*> ods;
			vector<Order*>::iterator iter;

			if(E_INS_RECENT == firstOpenIns->insType){
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
			}else{
				trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
			}
			if(ods.size()>0){
				trader->log("Wait for second leg match\n");
				return;
			}
			//check if has open order, if have cancel
			if(E_INS_FORWARD == firstOpenIns->insType){
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
				if(E_INS_FORWARD == firstCloseIns->insType){
					ls = E_SHORT;
					px = firstCloseIns->lastQuote->BidPrice1;
				}else{
					ls = E_LONG;
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
					for(iter=ods.begin(); iter!=ods.end();iter++){
						if(E_INS_FORWARD == firstCloseIns->insType){
							if((*iter)->submit_price < firstCloseIns->lastQuote->BidPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}else{
							if((*iter)->submit_price > firstCloseIns->lastQuote->AskPrice1
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
				if(E_INS_FORWARD == firstCloseIns->insType){
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

void Instrument::ShowLockedSpread()
{
	list<LockedSpread>::iterator iter;
	lockedSpreadY.sort();
	lockedSpreadT.sort();
	Trader::GetTrader()->log("Yesterday locked position:\n");
	for(iter=lockedSpreadY.begin();
		iter!=lockedSpreadY.end();
		iter++)
	{
		char buffer[128]={0};
		sprintf(buffer,"%d %f %d\n",iter->date, iter->spread, iter->volume);
		Trader::GetTrader()->log(buffer);
	}

	Trader::GetTrader()->log("Today locked position:\n");
	for(iter=lockedSpreadT.begin();
		iter!=lockedSpreadT.end();
		iter++)
	{
		char buffer[128]={0};
		sprintf(buffer,"%d %f %d\n",iter->date, iter->spread, iter->volume);
		Trader::GetTrader()->log(buffer);
	}
}

void Instrument::UpdateLockedSpread(LockedSpread &lockedSpread, bool isStopLoss, bool isToday)
{

	list<LockedSpread>* lkp = isToday?&lockedSpreadT:&lockedSpreadY;
	if((direction == E_DIR_UP && isStopLoss==true)
	|| 	direction == E_DIR_DOWN && isStopLoss==false ){
			(*lkp).sort();
			(*lkp).reverse();
	}
	int reminder = lockedSpread.volume;
	
	while(reminder>0){
		if((*lkp).front().volume > reminder){
			(*lkp).front().volume-=reminder;
		}else{
			reminder -= (*lkp).front().volume;
			(*lkp).pop_front();	
		}
	}
	const char *tmpFile="./tmp.cfg";
	const char *targetFile="position.cfg";
	fstream tmp;
	tmp.open(tmpFile,fstream::out);
	list<LockedSpread>::iterator iter=(*lkp).begin();
	for(;iter!=(*lkp).end();iter++){
		tmp<<iter->date<<" "<<iter->spread<<" "<<iter->volume<<endl;
	}
	tmp.close();
	int status=0;
	status = rename(tmpFile,targetFile);
	if(status !=0){
		trader->log("Update position file failed");
		perror("Update position file failed");
		exit(1);
	}
}

double Instrument::GetAverageSpread()
{
	double totalSpread=0.0;
	double totalVolume=0.0;

	list<LockedSpread>::iterator iter;
	for(iter=lockedSpreadY.begin();
		iter!=lockedSpreadY.end();
		iter++)
	{
		totalSpread += iter->spread*iter->volume;
		totalVolume += iter->volume;
	}

	for(iter=lockedSpreadT.begin();
		iter!=lockedSpreadT.end();
		iter++)
	{
		totalSpread += iter->spread*iter->volume;
		totalVolume += iter->volume;
	}
	return totalSpread/totalVolume;
}

double Instrument::GetBadSpread()
{
	double spread=0.0;
	lockedSpreadT.sort();
	lockedSpreadY.sort();
	list<LockedSpread> tmp;
	list<LockedSpread> tmpT=lockedSpreadT;
	list<LockedSpread> tmpY=lockedSpreadY;
	
	tmp.merge(tmpT);
	tmp.merge(tmpY);

	if(direction==E_DIR_UP){
		tmp.reverse();
	}
	return tmp.front().spread;
}

bool Instrument::IsOpenLong()
{
	if(firstOpenIns->insType==E_INS_FORWARD){
		if(bidSpread<=openThreshold){
			return true;
		}
	}else{
		if(askSpread<=openThreshold){
			return true;
		}
	}
	return false;
}
bool Instrument::IsOpenShort()
{
	if(firstOpenIns->insType==E_INS_FORWARD){
		if(askSpread>=openThreshold){
			return true;
		}
	}else{
		if(bidSpread>=openThreshold){
			return true;
		}
	}
	return false;
}
bool Instrument::IsCloseLong()
{
	if(firstCloseIns->insType==E_INS_FORWARD){
		if(askSpread>=closeThreshold){
			return true;
		}
	}else{
		if(bidSpread>=closeThreshold){
			return true;
		}
	}
	return false;
}
bool Instrument::IsCloseShort()
{
	if(firstCloseIns->insType==E_INS_FORWARD){
		if(bidSpread<=closeThreshold){
			return true;
		}
	}else{
		if(askSpread<=closeThreshold){
			return true;
		}
	}
	return false;
}
bool Instrument::IsStopLoss(double tradedSpread)
{
	if(direction==E_DIR_UP){
		if(firstCloseIns->insType==E_INS_FORWARD){
			if((askSpread+stopLoss*priceTick) <= tradedSpread){
				return true;
			}else{
				return false;
			}
		}else{
			if((bidSpread+stopLoss*priceTick) <= tradedSpread){
				return true;
			}else{
				return false;
			}
		}
	}else{
		if(firstCloseIns->insType==E_INS_FORWARD){
			if((bidSpread-stopLoss*priceTick) >= tradedSpread){
				return true;
			}else{
				return false;
			}
		}else{
			if((askSpread-stopLoss*priceTick) >= tradedSpread){
				return true;
			}else{
				return false;
			}
		}
	}	
}

bool Instrument::IsForecast(EOpenClose oc, ELongShort ls)
{
	if(direction==E_DIR_UP){
		if(oc==E_OPEN){//open order
			if(ls==E_LONG){
				if(bidSpread<openThreshold){
					return false;
				}
			}else{
				if(askSpread<openThreshold){
					return false;
				}
			}
		}else{//close order
			if(ls==E_LONG){
				if(askSpread>closeThreshold){
					return false;
				}
			}else{
				if(bidSpread>closeThreshold){
					return false;
				}
			}
		}
	}else{
		if(oc==E_OPEN){//open order
			if(ls==E_LONG){
				if(bidSpread>openThreshold){
					return false;
				}
			}else{
				if(askSpread>openThreshold){
					return false;
				}
			}
		}else{//close order
			if(ls==E_LONG){
				if(askSpread<closeThreshold){
					return false;
				}
			}else{
				if(bidSpread<closeThreshold){
					return false;
				}
			}
		}
	}
	return true;
}
