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
Direction	Instrument::direction = E_DIR_INVALID;
int			Instrument::maxPosition = 0;
int			Instrument::submitMax = 0;
bool		Instrument::loop = true;

Instrument::Instrument(char *ins_name)
{
	STRCPY(name, ins_name);
	reached = false;
	lastQuote = new Quote;
	trader_ = Trader::GetTrader();
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
		//memset(UpdateTime, 0, sizeof(UpdateTime));
		//memset(LocalTime, 0, sizeof(LocalTime));
	}else{
		CalcSpread();
	}
	ShowQuote();

	//has position
	if(stopLossType == E_STOPLOSS_NO){
	}else{
		if(stopLossType == E_STOPLOSS_AVERAGE){
		}else{
		}
	}

	//has no position
	if(direction == E_DIR_UP){
		if(insType == openWith){
			//if relativins has order, that means the openwith's order has matched, so just pass;
			//if relativeins has no order, we should update the openwith's order 
			vector<Order*>	ods;
			trader_->GetOrder(relativeIns->name, E_OPEN, E_SHORT, ods);
			if(ods.size()!=0){
				return;
			}
			trader_->GetOrder(name, E_OPEN, E_LONG, ods);
			//full the open condition
			if(bidSpread <= openThreshold){
				if(ods.size()==0){
					Order* o = trader_->NewOrder(q->InstrumentID, q->AskPrice1, submitMax, E_OPEN, E_LONG);
					trader_->submit_order(o);	
				}else{
					//TODO
				}
			}else{
				//not satisfy the open condition, we should cancel the order
				if(ods.size()>0){
					vector<Order*>::iterator iter=ods.begin();
					for(; iter != ods.end(); iter++){
						if((*iter)->canceling == false){
							trader_->cancel_order(*iter);
						}
					}
				}
			}
		}
	}else if(direction == E_DIR_DOWN){
	}
}
void Instrument::ShowQuote()
{
	printf("Q %s %s %.5f %d %.5f %d %.5f %.5f %.5f\n",
			insType==E_INS_FORWARD?"F":"R",
			lastQuote->InstrumentID,
			lastQuote->AskPrice1,
			lastQuote->AskVolume1,
			lastQuote->BidPrice1,
			lastQuote->BidVolume1,
			lastQuote->AveragePrice,
			askSpread,
			bidSpread);
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
void Instrument::on_match(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
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
