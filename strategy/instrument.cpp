#include <iostream>
#include <string.h>
#include <vector>

#include "instrument.h"
#include "../include/data_types.h"
using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))

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
		// open with ins
		if(insType == openWith)
		{
			int lockedPosition = CalcLockedPosition(name, relativeIns->name, direction);
			//has no locked position
			if(lockedPosition ==0)
			{
				//full the open condition
				if(bidSpread <= openThreshold){
					vector<Order*> ods;
					trader->GetOrder(relativeIns->name, E_OPEN, E_SHORT, ods);
					if(ods.size()!=0){
						return;
					}
					trader->GetOrder(name, E_OPEN, E_LONG, ods);
					if(ods.size()>0){
						vector<Order*>::iterator iter = ods.begin();
						for(; iter != ods.end(); iter++){
							if((*iter)->submit_price < lastQuote->BidPrice1 
							&& (*iter)->canceling == false){
								trader->cancel_order(*iter);
							}
						}
					}else{
						if(maxPosition<=0)
						{
							cout<<"Don't open new position"<<endl;
						}
						else{
							if(maxPosition/submitMax >= 1){
								Order* o = trader->NewOrder(q->InstrumentID, q->BidPrice1, submitMax, E_OPEN, E_LONG);
								trader->submit_order(o);
							}else{
								int vol = maxPosition%submitMax;
								if(vol==0) return;
								Order* o = trader->NewOrder(q->InstrumentID, q->BidPrice1, vol , E_OPEN, E_LONG);
								trader->submit_order(o);
							}
						}
					}
				}else{
				}
			}
			//has locked position
			else
			{
			}
		}
		//
		if(insType == closeWith)
		{
		}
	}
	if(direction == E_DIR_DOWN)
	{
		if(insType == openWith)
		{}
		if(insType == closeWith)
		{}
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
	cout<<__FUNCTION__<<": "<<name<<endl;
	if(insType==openWith)
	{
		switch(o->open_close){
		case E_OPEN:
			if(o->long_short==E_LONG){
				const char* nm = relativeIns->name;
				double px = relativeIns->lastQuote->BidPrice1;
				int vol = o->match_volume;
				Order* o = trader->NewOrder(nm, px, vol, E_OPEN, E_SHORT);
				trader->submit_order(o);
			}else{
				const char* nm = relativeIns->name;
				double px = relativeIns->lastQuote->AskPrice1;
				int vol = o->match_volume;
				Order* o = trader->NewOrder(nm, px, vol, E_OPEN, E_LONG);
				trader->submit_order(o);
			}
		break;	
		}
	}
	if(insType==closeWith)
	{
		switch(o->open_close){
		case E_CLOSE:
		case E_CLOSE_T:
		case E_CLOSE_Y:
			if(o->long_short==E_LONG){
				const char* nm = relativeIns->name;
				double px = relativeIns->lastQuote->AskPrice1;
				int vol = o->match_volume;
				Order* o = trader->NewOrder(nm, px, vol, E_CLOSE, E_SHORT);
				trader->submit_order(o);
			}else{
				const char* nm = relativeIns->name;
				double px = relativeIns->lastQuote->BidPrice1;
				int vol = o->match_volume;
				Order* o = trader->NewOrder(nm, px, vol, E_CLOSE, E_LONG);
				trader->submit_order(o);
			}
		break;
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
