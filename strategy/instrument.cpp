#include <fstream>
#include <cmath>
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
list<MatchInfo> 	Instrument::secondOpenMatch;
list<MatchInfo> 	Instrument::secondCloseMatch;

bool  				Instrument::triggerStopLoss = false;
Instrument*			Instrument::firstOpenIns = NULL;
Instrument*			Instrument::firstCloseIns = NULL;
Instrument*			Instrument::secondOpenIns = NULL;
Instrument*			Instrument::secondCloseIns = NULL;
Instrument*			Instrument::mainIns = NULL;
EDirection			Instrument::direction = E_DIR_INVALID;
InsType				Instrument::openWith = E_INS_INVALID;
InsType				Instrument::closeWith = E_INS_INVALID; 
InsType				Instrument::triggerSpread = E_INS_INVALID; 
StopLoss			Instrument::stopLossType = E_STOPLOSS_NO;
vector<int>			Instrument::openTime;
vector<int>			Instrument::closeTime;
double 				Instrument::askSpread = 0.0;
double 				Instrument::bidSpread = 0.0;

double 				Instrument::last_bidSpread = 0.0;
double 				Instrument::last_askSpread = 0.0;
double				Instrument::openThreshold = 0.0;
double				Instrument::closeThreshold = 0.0;
int					Instrument::stopLoss = 0;
int					Instrument::maxPosition = 0;
int					Instrument::openCount=0;
int					Instrument::submitMax = 0;
bool				Instrument::loop = true;
bool				Instrument::needToStopLoss=false;
int					Instrument::secondPx1VolBase = 0;

int Instrument::forecast_score_openlow=0;
int Instrument::forecast_score_openhigh=0;
int Instrument::forecast_score_closelow=0;
int Instrument::forecast_score_closehigh=0;


int Instrument::firstOpenInsSubmit=0;
int Instrument::firstOpenInsMatch=0;
int Instrument::secondOpenInsSubmit=0;
int Instrument::secondOpenInsMatch=0;

int Instrument::firstCloseInsSubmit=0;
int Instrument::firstCloseInsMatch=0;
int Instrument::secondCloseInsSubmit=0;
int Instrument::secondCloseInsMatch=0;

int Instrument::secondOpenRadicalScore=0;
int Instrument::secondCloseRadicalScore=0;

bool Instrument::has_stop_lossed = false;
Instrument::Instrument(char *ins_name, int vf, int mf)
{
	STRCPY(name, ins_name);
	volumeForecastBase = vf;
	matchForecastBase = mf;
	reached = false;
	trader = Trader::GetTrader();
	priceTick = 0.0;

	forecastByVolume =  E_FORECAST_NONE;
	volumeForecastCnt = 0;
	volumeForecastWin = 0;
	forecastByMatch = E_FORECAST_NONE;
	matchForecastCnt = 0;
	matchForecastWin = 0;
}

void Instrument::ShowState()
{
}

void Instrument::ProcessOpenLong(int lockedPosition)
{
	if(IsOpenLong()){
		//full the open condition
		FullOpenLong(lockedPosition);
	}
	else{
		//don't full open condition, cancel open orders
		DoNotFullOpenLong();
	}
}

void Instrument::ProcessOpenShort(int lockedPosition)
{
	if(IsOpenShort()){
		//full the open condition
		FullOpenShort(lockedPosition);
	}
	else{
		//don't full open condition, cancel open orders
		DoNotFullOpenShort();
	}
}

void Instrument::on_quote(shared_ptr<Quote> q)
{
	//save the last quote
	previousQuote = currentQuote;
	currentQuote = q;

	if(previousQuote.get()){
		if(previousQuote->AskPrice1 == currentQuote->AskPrice1
		&& previousQuote->BidPrice1 == currentQuote->BidPrice1){
			rangeFirst = false;
		}else{
			rangeFirst = true;
		}
	}
	if(reached == false){
		reached = true;
	}
	if(relativeIns->reached == false){
		return;
	}
	static bool firstCalcSpread=true;
	if(insType == triggerSpread || firstCalcSpread == true){
		firstCalcSpread=false;
		if(insType == E_INS_FORWARD){
			CalcSpread(false);
		}else{
			CalcSpread();
		}
	}
	
	ShowQuote();
	if(direction == E_DIR_UP)
	{
		//open long
		int lockedPosition = CalcLockedPosition();
		{
			char buffer[256]={0};
			
			sprintf(buffer, "Locked position is %d\n", lockedPosition);
			trader->log(buffer);
		}
		if(lockedPosition != 0){
			//full the close condition
			if(IsCloseLong(lockedPosition))
			{
				FullCloseLong(lockedPosition);
			}else{
				//don't full close condition, cancel close orders
				DoNotFullCloseLong();
			}
		}
		ProcessOpenLong(lockedPosition);
	}else if(direction == E_DIR_DOWN){
		//open short
		int lockedPosition = CalcLockedPosition();
		{
			char buffer[256]={0};
			sprintf(buffer, "Locked position is %d\n", lockedPosition);
			trader->log(buffer);
		}
		if(lockedPosition !=0){
			//full the close condition
			if(IsCloseShort(lockedPosition))
			{
				FullCloseShort(lockedPosition);
			}else{
				DoNotFullCloseShort();
			}
		}
		ProcessOpenShort(lockedPosition);
	}
}

void Instrument::ShowQuote()
{
	char buffer[256]={0};
	static const char* forecastStr[]={
		"-",
		"U",
		"D",
	};

	switch(forecastByVolume){
		case E_FORECAST_NONE:
			break;

		case E_FORECAST_UP:
			if(previousQuote.get()){
				if(currentQuote->AskPrice1 > previousQuote->AskPrice1){
					volumeForecastWin++;	
					forecastByVolume = E_FORECAST_NONE;
				}else if(currentQuote->AskPrice1 < previousQuote->AskPrice1){
					forecastByVolume = E_FORECAST_NONE;
				}
			}
			break;

		case E_FORECAST_DOWN:
			if(previousQuote.get()){
				if(currentQuote->BidPrice1 < previousQuote->BidPrice1){
					volumeForecastWin++;
					forecastByVolume = E_FORECAST_NONE;
				}else if(currentQuote->AskPrice1 > previousQuote->AskPrice1){
					forecastByVolume = E_FORECAST_NONE;
				}
			}
			break;
	}

	switch(forecastByMatch){
		case E_FORECAST_NONE:
			break;

		case E_FORECAST_UP:
			if(previousQuote.get() && currentQuote->BidPrice1 > previousQuote->BidPrice1){
				matchForecastWin++;	
				forecastByMatch = E_FORECAST_NONE;
			}
			break;

		case E_FORECAST_DOWN:
			if(previousQuote.get() && currentQuote->AskPrice1 < previousQuote->AskPrice1){
				matchForecastWin++;	
				forecastByMatch = E_FORECAST_NONE;
			}
			break;
	}
	volumeScore = 0;
	if(rangeFirst == false){
		double rate = currentQuote->AskVolume1*1.0
			/ (currentQuote->BidVolume1 + currentQuote->AskVolume1);
		volumeScore = 400 * (rate-0.5)*(rate-0.5);
		if(forecastByVolume == E_FORECAST_NONE){
			//400*(x-0.5)^2
			if(volumeScore >= volumeForecastBase){
				if(currentQuote->BidVolume1 >= currentQuote->AskVolume1){
					forecastByVolume = E_FORECAST_UP;
					volumeForecastCnt++;
				}else if(currentQuote->BidVolume1 < currentQuote->AskVolume1){
					forecastByVolume = E_FORECAST_DOWN;
					volumeForecastCnt++;
				}
			}
		}

		if(previousQuote){
			int v = currentQuote->TotalVolume - previousQuote->TotalVolume;	
			char buffer[64] = {0};
			const char* flag="-";
			if(currentQuote->LastPrice >= previousQuote->AskPrice1){
				flag = "U";
			}else if(currentQuote->LastPrice <= previousQuote->BidPrice1){
				flag = "D";
			}
			sprintf(buffer, "mv %s %s %d",name, flag, v);
			trader->log(buffer);
		}			
	}else{
			
	}

	// f/r time name bidpx1 bv1 askpx1 av1 bids asks
	sprintf(buffer,"Q %s %s %s %.2f %d %.2f %d %.2f %.2f V%d-%s-%d/%d M%s%d/%d",
			insType==E_INS_FORWARD?"F":"R",
			currentQuote->UpdateTime,
			currentQuote->InstrumentID,
			currentQuote->BidPrice1,
			currentQuote->BidVolume1,
			currentQuote->AskPrice1,
			currentQuote->AskVolume1,
			bidSpread,
			askSpread,
			volumeScore,
			forecastStr[forecastByVolume],
			volumeForecastWin,
			volumeForecastCnt,
			forecastStr[forecastByMatch],
			matchForecastWin,	
			matchForecastCnt);
	trader->log(buffer);
}
void Instrument::CalcSpread(bool rct)
{
	last_bidSpread = bidSpread;
	last_askSpread = askSpread;
		
	if(rct==false){
		bidSpread = currentQuote->BidPrice1 - relativeIns->currentQuote->BidPrice1;
		askSpread = currentQuote->AskPrice1 - relativeIns->currentQuote->AskPrice1;
	}else{
		bidSpread = relativeIns->currentQuote->BidPrice1 - currentQuote->BidPrice1;
		askSpread = relativeIns->currentQuote->AskPrice1 - currentQuote->AskPrice1;
	}
}
void Instrument::on_match(shared_ptr<Order> o)
{

	const char *targetFile="position.cfg";
	FILE *tmp=fopen(targetFile,"a");

	if(o->open_close == E_OPEN){
	//open 
		if(firstOpenIns == this){
			//first leg, need to lock position
			firstOpenInsMatch += o->match_volume;

			if(secondOpenInsSubmit == 0){
				const char* nm = secondOpenIns->name;
				double px = o->long_short == E_LONG ? secondOpenIns->currentQuote->BidPrice1 :secondOpenIns->currentQuote->AskPrice1;
				int vol = o->match_volume;
				shared_ptr<Order> new_order = trader->NewOrder(nm, px, vol, E_OPEN, o->long_short == E_LONG ? E_SHORT : E_LONG);
				trader->submit_order(new_order);
			}

			if(firstOpenInsMatch == firstOpenInsSubmit
			&& firstOpenInsMatch == secondOpenInsMatch){
				firstOpenInsSubmit = secondOpenInsSubmit = 0;
				firstOpenInsMatch = secondOpenInsMatch = 0;
			}
			openCount += o->match_volume;

			if(secondOpenMatch.size()==0){
				MatchInfo matchInfo;
				matchInfo.date = o->date;
				matchInfo.volume = o->match_volume;
				matchInfo.price = o->match_price;
				firstOpenMatch.push_back(matchInfo);
			}else{
				LockedSpread lockedSpread;
				int reminder=o->match_volume;
				while(reminder>0){
					if(secondOpenMatch.front().volume > reminder){
						secondOpenMatch.front().volume -= reminder;
						lockedSpread.date = o->date;
						lockedSpread.volume = reminder;
						double spread=0.0;
						if(insType==E_INS_FORWARD){
							spread = o->match_price - secondOpenMatch.front().price;
						}else{
							spread = secondOpenMatch.front().price - o->match_price;
						}
						lockedSpread.spread = spread;
						lockedSpreadT.push_back(lockedSpread);
					}else{
						reminder -= secondOpenMatch.front().volume;
						lockedSpread.date= o->date;
						lockedSpread.volume = secondOpenMatch.front().volume;
						double spread=0.0;
						if(insType==E_INS_FORWARD){
							spread = o->match_price - secondOpenMatch.front().price;
						}else{
							spread = secondOpenMatch.front().price - o->match_price;
						}
						lockedSpread.spread = spread;
						lockedSpreadT.push_back(lockedSpread);
						char buffer[128]={0};
						sprintf(buffer,"%d %f %d\n",lockedSpread.date, lockedSpread.spread, lockedSpread.volume);
						trader->log(buffer);
						fwrite(buffer, strlen(buffer),1,tmp);
						fflush(tmp);
						secondOpenMatch.pop_front();	
						if(reminder>0 && secondOpenMatch.size()==0){
							MatchInfo matchInfo;
							matchInfo.date = o->date;
							matchInfo.volume = o->match_volume;
							matchInfo.price = o->match_price;
							firstOpenMatch.push_back(matchInfo);
						}
					}
				}
				fclose(tmp);
			}
		}else{
			//second leg, need to update lockedSpread
			secondOpenInsMatch += o->match_volume;

			if(firstOpenInsMatch == firstOpenInsSubmit
			&& firstOpenInsMatch == secondOpenInsMatch){
				firstOpenInsSubmit = secondOpenInsSubmit = 0;
				firstOpenInsMatch = secondOpenInsMatch = 0;
			}


			if(firstOpenMatch.size()==0){
				MatchInfo matchInfo;
				matchInfo.date = o->date;
				matchInfo.volume = o->match_volume;
				matchInfo.price = o->match_price;
				secondOpenMatch.push_back(matchInfo);
			}else{
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
						if(reminder>0 && firstOpenMatch.size()==0){
							MatchInfo matchInfo;
							matchInfo.date = o->date;
							matchInfo.volume = o->match_volume;
							matchInfo.price = o->match_price;
							secondOpenMatch.push_back(matchInfo);
						}
					}
				}
				fclose(tmp);
			}
		}
	}else{
	//close
		if(firstCloseIns == this){
			//first leg, need to lock position
			trader->log("***************************************************\n");
			trader->log("first close leg matched\n");
			trader->log("***************************************************\n");
			o->ShowOrder();
			if(o->stop_loss == true){
				has_stop_lossed = true;
			}

			firstCloseInsMatch += o->match_volume;
			
			if(secondCloseInsSubmit ==0){
				const char* nm = secondCloseIns->name;
				double px = o->long_short == E_LONG ? secondCloseIns->currentQuote->AskPrice1 :secondCloseIns->currentQuote->BidPrice1;
				int vol = o->match_volume;

				shared_ptr<Order> new_order = trader->NewOrder(nm, px, vol, o->open_close, o->long_short == E_LONG ? E_SHORT : E_LONG);
				new_order->stop_loss = o->stop_loss;
				if(o->stop_loss == true){
					openCount += o->match_volume;
				}
				trader->submit_order(new_order);
			}

			if(firstCloseInsMatch == firstCloseInsSubmit
			&& firstCloseInsMatch == secondCloseInsMatch){
				firstCloseInsSubmit = secondCloseInsSubmit = 0;
				firstCloseInsMatch = secondCloseInsMatch = 0;
			}

			if(secondCloseMatch.size()==0){
				MatchInfo matchInfo;
				matchInfo.date = o->date;
				matchInfo.volume = o->match_volume;
				matchInfo.price = o->match_price;
				firstCloseMatch.push_back(matchInfo);
			}else{	
				LockedSpread lockedSpread;
				int reminder=o->match_volume;
				while(reminder>0){
					if(secondCloseMatch.front().volume > reminder){
						secondCloseMatch.front().volume -= reminder;
						lockedSpread.date = o->date;
						lockedSpread.volume = reminder;
						double spread=0.0;
						if(insType==E_INS_FORWARD){
							spread = o->match_price - secondCloseMatch.front().price;
						}else{
							spread = secondCloseMatch.front().price - o->match_price;
						}
						lockedSpread.spread = spread;
					}else{
						reminder -= secondCloseMatch.front().volume;
						lockedSpread.date= o->date;
						lockedSpread.volume = secondCloseMatch.front().volume;
						double spread=0.0;
						if(insType==E_INS_FORWARD){
							spread = o->match_price - secondCloseMatch.front().price;
						}else{
							spread = secondCloseMatch.front().price - o->match_price;
						}
						lockedSpread.spread = spread;
						secondCloseMatch.pop_front();	
						if(reminder>0 && secondCloseMatch.size()==0){
							MatchInfo matchInfo;
							matchInfo.date = o->date;
							matchInfo.volume = reminder;
							matchInfo.price = o->match_price;
							firstCloseMatch.push_back(matchInfo);
							break;
						}
					}
					UpdateLockedSpread(lockedSpread, 
								o->stop_loss?true:false,
								o->open_close==E_CLOSE_Y?false:true);
				}
			}
		}else{
			//second leg, need to update lockedSpread
			secondCloseInsMatch += o->match_volume;
			if(firstCloseInsMatch == firstCloseInsSubmit
			&& firstCloseInsMatch == secondCloseInsMatch){
				firstCloseInsSubmit = secondCloseInsSubmit = 0;
				firstCloseInsMatch = secondCloseInsMatch = 0;
			}


#if 0
			MatchInfo matchInfo;
			matchInfo.date = o->date;
			matchInfo.volume = o->match_volume;
			matchInfo.price = o->match_price;
			firstCloseMatch.push_back(matchInfo);
#endif

			if(firstCloseMatch.size()==0){
				MatchInfo matchInfo;
				matchInfo.date = o->date;
				matchInfo.volume = o->match_volume;
				matchInfo.price = o->match_price;
				secondCloseMatch.push_back(matchInfo);
			}else{	
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
						if(reminder>0 && firstCloseMatch.size()==0){
							MatchInfo matchInfo;
							matchInfo.date = o->date;
							matchInfo.volume = reminder;
							matchInfo.price = o->match_price;
							secondCloseMatch.push_back(matchInfo);
							break;
						}
					}
					UpdateLockedSpread(lockedSpread, 
								o->stop_loss?true:false,
								o->open_close==E_CLOSE_Y?false:true);
				}
			}

			int lockedPosition = CalcLockedPosition();
			if(needToStopLoss==true && lockedPosition==0){
				needToStopLoss=false;
			}
		}
	}
}

void Instrument::on_reject(shared_ptr<Order>)
{
	trader->log("***************************************\n");
	trader->log("*  下单被拒绝，请检查配置或者资金     *\n");
	trader->log("*  退出中...                          *\n");
	trader->log("***************************************\n");
	trader->quit();
	cout<<("***************************************\n");
	cout<<("*  下单被拒绝，请检查配置或者资金     *\n");
	cout<<("*  退出中...                          *\n");
	cout<<("***************************************\n");
	exit(1);
}

void Instrument::on_cancel(shared_ptr<Order> o)
{
	trader->log(__FUNCTION__);trader->log("\n");
	if(secondOpenIns == this
	|| secondCloseIns == this){
		o->ShowOrder();
		double price = 0.0; 
		if(o->open_close == E_OPEN){
			if(o->long_short == E_LONG){
				price = currentQuote->BidPrice1+priceTick;
			}else{
				price = currentQuote->AskPrice1-priceTick ;
			}
		}else{
			if(o->long_short == E_LONG){
				price = currentQuote->AskPrice1-priceTick ;
			}else{
				price = currentQuote->BidPrice1+priceTick;
			}
		}
		shared_ptr<Order> new_order = trader->NewOrder(name, price, o->canceled_volume, o->open_close, o->long_short);
		new_order->stop_loss = o->stop_loss;
		trader->submit_order(new_order);
	}else{
		if(secondOpenInsSubmit==0
		&&secondCloseInsSubmit==0){
			if(o->open_close==E_OPEN){
				firstOpenInsSubmit -= o->canceled_volume;
			}else{
				firstCloseInsSubmit -= o->canceled_volume;
			}
			return;
		}else{
			double price = 0.0; 
			if(o->open_close == E_OPEN){
				if(o->long_short == E_LONG){
					price = currentQuote->BidPrice1+priceTick;
				}else{
					price = currentQuote->AskPrice1-priceTick;
				}
			}else{
				if(o->long_short == E_LONG){
					price = currentQuote->AskPrice1-priceTick;
				}else{
					price = currentQuote->BidPrice1+priceTick;
				}
			}
			shared_ptr<Order> new_order = trader->NewOrder(name, price, o->canceled_volume, o->open_close, o->long_short);
			new_order->stop_loss = o->stop_loss;
			trader->submit_order(new_order);
		}
	}
}

void Instrument::on_insert(shared_ptr<Order>)
{
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
void Instrument::CancelOrders(vector<shared_ptr<Order>> &ods)
{
	vector<shared_ptr<Order>>::iterator iter = ods.begin();
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
	if(E_INS_FORWARD == firstOpenIns->insType){
		//open from forward instrument
		vector<shared_ptr<Order>> ods;
		vector<shared_ptr<Order>>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->currentQuote->AskPrice1 + secondOpenIns->priceTick; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}

		{
			char buffer[256]={0};
			sprintf(buffer,"needToStopLoss=%s\n",needToStopLoss?"true":"false");
		}
		if(needToStopLoss==true && lockedPosition>0){
			return;
		}

		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->currentQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			int remaindVolume = maxPosition - lockedPosition;
			//int remaindVolume = maxPosition - openCount;
			if(remaindVolume<=0){
				trader->log("Don't open new position\n");
			}else{
				const char* nm = firstOpenIns->name;
				double price = firstOpenIns->currentQuote->BidPrice1;
				int vol = remaindVolume/submitMax>=1?submitMax:remaindVolume%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				bool forecast=true;
				if(IsForecast(E_OPEN,E_LONG)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->currentQuote, E_OPEN, E_LONG);
				}
				if(forecast){
					shared_ptr<Order> o = trader->NewOrder(nm, price, vol, E_OPEN, E_LONG);
					trader->submit_order(o);
					firstOpenInsSubmit = vol;
					if(mainIns->volumeScore >= forecast_score_openhigh){
						shared_ptr<Order> o = trader->NewOrder(secondOpenIns->name,
										secondOpenIns->currentQuote->BidPrice1,
										vol, E_OPEN, E_SHORT);
						trader->submit_order(o);
						secondOpenInsSubmit = vol;
					}
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//open from recent instrument
		vector<shared_ptr<Order>> ods;
		vector<shared_ptr<Order>>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->currentQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}

		{
			char buffer[256]={0};
			sprintf(buffer,"needToStopLoss=%s\n",needToStopLoss?"true":"false");
		}
		if(needToStopLoss==true && lockedPosition>0){
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->currentQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			int remaindVolume = maxPosition - lockedPosition;
			//int remaindVolume = maxPosition - openCount;
			if(remaindVolume<=0){
				trader->log("Don't open new position\n");
			}else{
				const char* nm = firstOpenIns->name;
				double price = firstOpenIns->currentQuote->AskPrice1;
				int vol = remaindVolume/submitMax>=1?submitMax:remaindVolume%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				bool forecast=true;
				if(IsForecast(E_OPEN,E_SHORT)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->currentQuote, E_OPEN, E_SHORT);
				}
				if(forecast){
					shared_ptr<Order> o =trader->NewOrder(nm, price, vol, E_OPEN, E_SHORT);
					trader->submit_order(o);
					firstOpenInsSubmit = vol;
					if(mainIns->volumeScore >= forecast_score_openhigh){
						shared_ptr<Order> o =trader->NewOrder(secondOpenIns->name, 
											secondOpenIns->currentQuote->AskPrice1,
											vol, E_OPEN, E_LONG);
						trader->submit_order(o);
						secondOpenInsSubmit = vol;
					}
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
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()==0){
			return;
		}else{
			if(secondOpenInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price < firstOpenIns->currentQuote->BidPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}			
	}else{
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()==0){
			return;
		}else{
			if(secondOpenInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price > firstOpenIns->currentQuote->AskPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}
	}
}

void Instrument::FullCloseLong(int lockedPosition)
{
	if(E_INS_FORWARD==firstCloseIns->insType){
		//close from forward instrument
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()!=0){
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->currentQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()>0){
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->currentQuote->AskPrice1; 
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
				double price = firstCloseIns->currentQuote->AskPrice1;
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
					firstCloseIns->currentQuote, oc, E_LONG);
				}
				if(forecast){
					shared_ptr<Order> o =trader->NewOrder(nm, price, vol, oc, E_LONG);
					trader->submit_order(o);
					firstCloseInsSubmit = vol;
					if(mainIns->volumeScore >= forecast_score_closehigh){
						shared_ptr<Order> o =trader->NewOrder(secondCloseIns->name,
											secondCloseIns->currentQuote->AskPrice1,
											vol, oc, E_SHORT);
						trader->submit_order(o);
						secondCloseInsSubmit = vol;
					}
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//close from recent instrument
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()!=0){
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->currentQuote->BidPrice1; 
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
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->currentQuote->BidPrice1; 
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
				double price = firstCloseIns->currentQuote->BidPrice1;
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
					firstCloseIns->currentQuote, oc, E_SHORT);
				}
				if(forecast){
					shared_ptr<Order> o = trader->NewOrder(nm, price, vol, oc, E_SHORT);
					trader->submit_order(o);
					firstCloseInsSubmit = vol;
					if(mainIns->volumeScore >= forecast_score_closehigh){
						shared_ptr<Order> o =trader->NewOrder(secondCloseIns->name,
											secondCloseIns->currentQuote->BidPrice1,
											vol, oc, E_LONG);
						trader->submit_order(o);
						secondCloseInsSubmit = vol;
					}
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
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()==0){
		}else{
			if(secondCloseInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price > firstCloseIns->currentQuote->AskPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}			
	}else{
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			if(secondCloseInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price < firstCloseIns->currentQuote->BidPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}			
	}
}

void Instrument::FullOpenShort(int lockedPosition)
{
	trader->log(__FUNCTION__);trader->log("\n");
	if(E_INS_FORWARD==firstOpenIns->insType){
		//open from forward instrument
		vector<shared_ptr<Order>> ods;
		vector<shared_ptr<Order>>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->currentQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}
		{
			char buffer[256]={0};
			sprintf(buffer,"needToStopLoss=%s\n",needToStopLoss?"true":"false");
		}
		if(needToStopLoss==true && lockedPosition>0){
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->currentQuote->AskPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			int remaindVolume = maxPosition - lockedPosition;
			//int remaindVolume = maxPosition - openCount;
			if(remaindVolume<=0){
				trader->log("Don't open new position\n");
			}else{
				const char* nm = firstOpenIns->name;
				double price = firstOpenIns->currentQuote->AskPrice1;
				int vol = remaindVolume/submitMax>=1?submitMax:remaindVolume%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				bool forecast=true;	
				if(IsForecast(E_OPEN,E_SHORT)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->currentQuote, E_OPEN, E_SHORT);
				}
				if(forecast){
					shared_ptr<Order> o = trader->NewOrder(nm, price, vol, E_OPEN, E_SHORT);
					trader->submit_order(o);
					firstOpenInsSubmit = vol;
					if(mainIns->volumeScore >= forecast_score_openhigh){
						shared_ptr<Order> o = trader->NewOrder(secondOpenIns->name,
									secondOpenIns->currentQuote->AskPrice1,
									vol, E_OPEN, E_LONG);
						trader->submit_order(o);
						secondOpenInsSubmit = vol;
					}
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//open from recent instrument
		vector<shared_ptr<Order>> ods;
		vector<shared_ptr<Order>>::iterator iter;
		trader->GetOrder(secondOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()!=0){
			iter = ods.begin();
			double newPrice = secondOpenIns->currentQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}

		{
			char buffer[256]={0};
			sprintf(buffer,"needToStopLoss=%s\n",needToStopLoss?"true":"false");
		}
		if(needToStopLoss==true && lockedPosition>0){
			return;
		}
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()>0){
			iter = ods.begin();
			double newPrice = firstOpenIns->currentQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price < newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}else{
			int remaindVolume = maxPosition - lockedPosition;
			//int remaindVolume = maxPosition - openCount;
			if(remaindVolume<=0){
				trader->log("Don't open new position\n");
			}else{
				const char* nm = firstOpenIns->name;
				double price = firstOpenIns->currentQuote->BidPrice1;
				int vol = remaindVolume/submitMax>=1?submitMax:remaindVolume%submitMax;
				if(vol==0){
					trader->log("Cann't open 0 volume order\n");
					return;
				}
				bool forecast=true;
				if(IsForecast(E_OPEN,E_LONG)==true){
					forecast = Forecast::OrderWillSuccess(price, 
					firstOpenIns->currentQuote, E_OPEN, E_LONG);
				}
				if(forecast){
					shared_ptr<Order> o =trader->NewOrder(nm, price, vol, E_OPEN, E_LONG);
					trader->submit_order(o);
					firstOpenInsSubmit = vol;
					if(mainIns->volumeScore >= forecast_score_openhigh){
						shared_ptr<Order> o = trader->NewOrder(secondOpenIns->name,
									secondOpenIns->currentQuote->BidPrice1,
									vol, E_OPEN, E_SHORT);
						trader->submit_order(o);
						secondOpenInsSubmit = vol;
					}
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
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_SHORT, ods);
		if(ods.size()==0){
			return;
		}else{
			if(secondOpenInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price > firstOpenIns->currentQuote->AskPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}			
	}else{
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstOpenIns->name, E_OPEN, E_LONG, ods);
		if(ods.size()==0){
			return;
		}else{
			if(secondOpenInsSubmit==9){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price < firstOpenIns->currentQuote->BidPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}
	}
}

void Instrument::FullCloseShort(int lockedPosition)
{
	if(E_INS_FORWARD==firstCloseIns->insType){
		//close from forward instrument
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()!=0){
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->currentQuote->BidPrice1; 
			for(; iter != ods.end(); iter++){
				if((*iter)->submit_price > newPrice 
				&& (*iter)->state != E_ORIGINAL
				&& (*iter)->canceling == false){
					trader->cancel_order(*iter);
				}
			}
		}
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()>0){
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->currentQuote->BidPrice1; 
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
				double price = firstCloseIns->currentQuote->BidPrice1;
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
					firstCloseIns->currentQuote, oc, E_SHORT);
				}
				if(forecast){
					shared_ptr<Order> o =trader->NewOrder(nm, price, vol, oc, E_SHORT);
					trader->submit_order(o);
				}else{
					trader->log("Forecast the order will not success, don't submit it\n");
				}
			}
		}
	}else{
		//close from recent instrument
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(secondCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(secondCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()!=0){
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = secondCloseIns->currentQuote->AskPrice1; 
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
			vector<shared_ptr<Order>>::iterator iter = ods.begin();
			double newPrice = firstCloseIns->currentQuote->AskPrice1; 
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
				double price = firstCloseIns->currentQuote->AskPrice1;
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
					firstCloseIns->currentQuote, oc, E_LONG);
				}
				if(forecast){
					shared_ptr<Order> o = trader->NewOrder(nm, price, vol, oc, E_LONG);
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
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_SHORT, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_SHORT, ods);
		if(ods.size()==0){
		}else{
			if(firstCloseInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price < firstOpenIns->currentQuote->BidPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}			
	}else{
		vector<shared_ptr<Order>> ods;
		trader->GetOrder(firstCloseIns->name, E_CLOSE_T, E_LONG, ods);
		trader->GetOrder(firstCloseIns->name, E_CLOSE_Y, E_LONG, ods);
		if(ods.size()==0){
		}else{
			if(firstCloseInsSubmit==0){
				CancelOrders(ods);
			}else{
				vector<shared_ptr<Order>>::iterator iter = ods.begin();
				for(; iter != ods.end(); iter++){
					if((*iter)->canceling == false
					&& (*iter)->state != E_ORIGINAL
					&& (*iter)->submit_price > firstOpenIns->currentQuote->AskPrice1){
						trader->cancel_order(*iter);
					}
				}
			}
		}			
	}
}
void Instrument::CheckStopLoss()
{
	trader->log(__FUNCTION__);
	trader->log("\n");
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

	vector<shared_ptr<Order>> ods;
	vector<shared_ptr<Order>>::iterator iter;
	if(direction == E_DIR_UP){
		if(IsStopLoss(tradedSpread)){
			trader->log("let's stop loss\n");
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
					px = firstCloseIns->currentQuote->AskPrice1;
				}else{
					ls = E_SHORT;
					px = firstCloseIns->currentQuote->BidPrice1;
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
						shared_ptr<Order> new_order = trader->NewOrder(nm, px, vol, oc, ls);
						trader->submit_order(new_order);
						return;
					}
				}else{
					//check to update the close orders' price
					for(iter=ods.begin(); iter!=ods.end();iter++){
						if(E_INS_FORWARD == firstCloseIns->insType){
							if((*iter)->submit_price > firstCloseIns->currentQuote->AskPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}else{
							if((*iter)->submit_price < firstCloseIns->currentQuote->BidPrice1
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
			trader->log("let's stop loss\n");
			needToStopLoss = true;
			vector<shared_ptr<Order>> ods;
			vector<shared_ptr<Order>>::iterator iter;

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
					px = firstCloseIns->currentQuote->BidPrice1;
				}else{
					ls = E_LONG;
					px = firstCloseIns->currentQuote->AskPrice1;
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
						shared_ptr<Order> new_order = trader->NewOrder(nm, px, vol, oc, ls);
						trader->submit_order(new_order);
						return;
					}
				}else{
					//check to update price
					for(iter=ods.begin(); iter!=ods.end();iter++){
						if(E_INS_FORWARD == firstCloseIns->insType){
							if((*iter)->submit_price < firstCloseIns->currentQuote->BidPrice1
							&& (*iter)->canceling == false
							&& (*iter)->state != E_ORIGINAL){
								trader->cancel_order(*iter);
							}
						}else{
							if((*iter)->submit_price > firstCloseIns->currentQuote->AskPrice1
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
	if(has_stop_lossed == true)
	{
		trader->log("已经触发过止损，不再开新仓");
		return false;
	}
	shared_ptr<Quote> mqt = mainIns->currentQuote;
	shared_ptr<Quote> sqt = mainIns->relativeIns->currentQuote;
	if(mqt->AskPrice1 - mqt->BidPrice1 > (MAIN_INS_GAP*mainIns->priceTick)){
		trader->log("主力合约盘口价差超过一跳");
		return false;
	}	
	if(sqt->AskPrice1 - sqt->BidPrice1 
		> SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick){

		char buffer[256]={0};
		sprintf(buffer,"%f-%f=%f > %f*%f=%f",sqt->AskPrice1,sqt->BidPrice1, sqt->AskPrice1 - sqt->BidPrice1,
			SECOND_MAIN_INS_GAP , mainIns->relativeIns->priceTick,
			SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick);
		trader->log(buffer);
		trader->log("次主力合约盘口价差超过两跳");
		return false;
	}	
	if(firstOpenIns->insType==E_INS_FORWARD){
		if(fabs(bidSpread - last_bidSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}else{
		if(fabs(askSpread - last_askSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}

	if(firstOpenIns->insType == E_INS_FORWARD){
		if(bidSpread == openThreshold){
			if(mainIns->volumeScore >= forecast_score_openlow
			&& mainIns->currentQuote->BidVolume1 < mainIns->currentQuote->AskVolume1){
				if(secondOpenIns->currentQuote->BidVolume1 >= secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(bidSpread < openThreshold){
			trader->log("价差优于开仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}else{
		if(askSpread == openThreshold){
			if(mainIns->volumeScore >= forecast_score_openlow
			&& mainIns->currentQuote->BidVolume1 > mainIns->currentQuote->AskVolume1){
				if(secondOpenIns->currentQuote->AskVolume1 >= secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if( askSpread < openThreshold){
			trader->log("价差优于开仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}
}
bool Instrument::IsOpenShort()
{
	if(has_stop_lossed == true){
		trader->log("已经出发过止损，不再开新仓");
		return false;
	}
	shared_ptr<Quote> mqt = mainIns->currentQuote;
	shared_ptr<Quote> sqt = mainIns->relativeIns->currentQuote;
	if(mqt->AskPrice1 - mqt->BidPrice1 > MAIN_INS_GAP*mainIns->priceTick){
		trader->log("主力合约盘口价差超过一跳");
		return false;
	}	
	if(sqt->AskPrice1 - sqt->BidPrice1 
	> SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick){
		char buffer[256]={0};
		sprintf(buffer,"%f-%f=%f > %f*%f=%f",sqt->AskPrice1,sqt->BidPrice1, sqt->AskPrice1 - sqt->BidPrice1,
			SECOND_MAIN_INS_GAP , mainIns->relativeIns->priceTick,
			SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick);
		trader->log(buffer);
		trader->log("次主力合约盘口价差超过两跳");
		return false;
	}	
	if(firstOpenIns->insType==E_INS_FORWARD){
		if(fabs(askSpread - last_askSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}else{
		if(fabs(bidSpread - last_bidSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}

	if(firstOpenIns->insType == E_INS_FORWARD){
		if(askSpread==openThreshold){
			if(mainIns->volumeScore >=forecast_score_openlow
			&& mainIns->currentQuote->AskVolume1 < mainIns->currentQuote->BidVolume1){
				if(secondOpenIns->currentQuote->AskVolume1 > secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(askSpread > openThreshold){
			trader->log("价差优于开仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}else{
		if(bidSpread == openThreshold){
			if(mainIns->volumeScore >= forecast_score_openlow
			&& mainIns->currentQuote->AskVolume1 > mainIns->currentQuote->BidVolume1){
				if(secondOpenIns->currentQuote->BidVolume1 > secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(bidSpread > openThreshold){
			trader->log("价差优于开仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}
}

bool Instrument::IsCloseLong(int &postion)
{

	shared_ptr<Quote> mqt = mainIns->currentQuote;
	shared_ptr<Quote> sqt = mainIns->relativeIns->currentQuote;
	if(mqt->AskPrice1 - mqt->BidPrice1 > MAIN_INS_GAP*mainIns->priceTick){
		trader->log("主力合约盘口价差超过一跳");
		return false;
	}	
	if(sqt->AskPrice1 - sqt->BidPrice1 
	> SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick){
		char buffer[256]={0};
		sprintf(buffer,"%f-%f=%f > %f*%f=%f",sqt->AskPrice1,sqt->BidPrice1, sqt->AskPrice1 - sqt->BidPrice1,
			SECOND_MAIN_INS_GAP , mainIns->relativeIns->priceTick,
			SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick);
		trader->log(buffer);
		trader->log("次主力合约盘口价差超过两跳");
		return false;
	}	
	if(firstCloseIns->insType==E_INS_FORWARD){
		if(fabs(askSpread - last_askSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}else{
		if(fabs(bidSpread - last_bidSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}

	double tradedSpread = 0.0;
	if(stopLossType == E_STOPLOSS_AVERAGE){
		tradedSpread = GetAverageSpread();
	}else if(stopLossType == E_STOPLOSS_TICKBYTICK){
		tradedSpread = GetBadSpread();
	}

	if(firstCloseIns->insType == E_INS_FORWARD){
		if(stopLossType != E_STOPLOSS_NO){
			if(askSpread == tradedSpread - stopLoss*priceTick){
				if(mainIns->volumeScore >= forecast_score_closelow
				&& mainIns->currentQuote->AskVolume1 < mainIns->currentQuote->BidVolume1){
					if(secondCloseIns->currentQuote->AskVolume1 > secondPx1VolBase){
						trader->log("达到止损线，条件符合");
						triggerStopLoss = true;						
						return true;
					}else{
						trader->log("达到止损线，但第二腿可能打不到");
						triggerStopLoss = false;	
					}
				}else{
					trader->log("达到止损线，但主力评分不足");
					triggerStopLoss = false;	
				}
			}else if(askSpread < tradedSpread - stopLoss*priceTick){
				trader->log("价差超过止损线");
				triggerStopLoss = true;	
				return true;
			}else{
				triggerStopLoss = false;	
			}
		}
		if(askSpread == closeThreshold){
			if(mainIns->volumeScore >= forecast_score_closelow
			&& mainIns->currentQuote->AskVolume1 < mainIns->currentQuote->BidVolume1){
				if(secondCloseIns->currentQuote->AskVolume1 > secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(askSpread > closeThreshold){
			trader->log("价差优于平仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}else{
		if(stopLossType != E_STOPLOSS_NO){
			if(bidSpread == tradedSpread - stopLoss*priceTick){
				if(mainIns->volumeScore >= forecast_score_closelow
				&& mainIns->currentQuote->AskVolume1 > mainIns->currentQuote->BidVolume1){
					if(secondCloseIns->currentQuote->BidVolume1 > secondPx1VolBase){
						trader->log("达到止损线，条件符合");
						triggerStopLoss = true;
						return true;
					}else{
						trader->log("达到止损线，但第二腿可能打不到");
						triggerStopLoss = false;
					}
				}else{
					trader->log("达到止损线，但主力评分不足");
					triggerStopLoss = false;
				}
			}else if(bidSpread < (tradedSpread - stopLoss*priceTick)){
				trader->log("价差超过止损线");
				triggerStopLoss = true;
				return true;
			}else{
				trader->log("未达到止损线");
				triggerStopLoss = false;
			}
		}
		if(bidSpread == closeThreshold){
			if(mainIns->volumeScore >= forecast_score_closelow
			&& mainIns->currentQuote->AskVolume1 > mainIns->currentQuote->BidVolume1){
				if(secondCloseIns->currentQuote->BidVolume1 > secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(bidSpread>closeThreshold){
			trader->log("价差优于平仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}
}
bool Instrument::IsCloseShort(int &position)
{
	shared_ptr<Quote> mqt = mainIns->currentQuote;
	shared_ptr<Quote> sqt = mainIns->relativeIns->currentQuote;
	if(mqt->AskPrice1 - mqt->BidPrice1 > mainIns->priceTick){
		trader->log("主力合约盘口价差超过一跳");
		return false;
	}	
	if(sqt->AskPrice1 - sqt->BidPrice1 > MAIN_INS_GAP * mainIns->relativeIns->priceTick){

		char buffer[256]={0};
		sprintf(buffer,"%f-%f=%f > %f*%f=%f",sqt->AskPrice1,sqt->BidPrice1, sqt->AskPrice1 - sqt->BidPrice1,
			SECOND_MAIN_INS_GAP , mainIns->relativeIns->priceTick,
			SECOND_MAIN_INS_GAP * mainIns->relativeIns->priceTick);
		trader->log(buffer);
		trader->log("次主力合约盘口价差超过两跳");
		return false;
	}	
	if(firstCloseIns->insType==E_INS_FORWARD){
		if(fabs(bidSpread - last_bidSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}else{
		if(fabs(askSpread - last_askSpread) > SECOND_MAIN_INS_GAP*priceTick){
			trader->log("价差与上次价差差额超过两跳");
			return false;
		}
	}

	if(firstCloseIns->insType == E_INS_FORWARD){
		if(bidSpread == closeThreshold){
			if(mainIns->volumeScore >= forecast_score_closelow
			&& mainIns->currentQuote->BidVolume1 < mainIns->currentQuote->AskVolume1){
				if(secondCloseIns->currentQuote->BidVolume1 >= secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(bidSpread<closeThreshold){
			trader->log("价差优于平仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}else{
		if(askSpread==closeThreshold){
			if(mainIns->volumeScore >= forecast_score_closelow
			&& mainIns->currentQuote->BidVolume1 > secondPx1VolBase){
				if(secondCloseIns->currentQuote->AskVolume1 >= secondPx1VolBase){
					trader->log("条件符合");
					return true;
				}else{
					trader->log("第二腿可能打不到");
					return false;
				}
			}else{
				trader->log("主力评分不足");
				return false;
			}
		}else if(askSpread < closeThreshold){
			trader->log("价差优于平仓条件");
			return true;
		}else{
			trader->log("价差不符合");
			return false;
		}
	}
}
bool Instrument::IsStopLoss(double tradedSpread)
{
	if(direction==E_DIR_UP){

		shared_ptr<Quote> mqt = mainIns->currentQuote;
		shared_ptr<Quote> sqt = mainIns->relativeIns->currentQuote;
		if(mqt->AskPrice1 - mqt->BidPrice1 > mainIns->priceTick){
			return false;
		}	
		if(sqt->AskPrice1 - sqt->BidPrice1 > 2 * mainIns->relativeIns->priceTick){
			return false;
		}	
		if(firstCloseIns->insType==E_INS_FORWARD){
			if(fabs(askSpread - last_askSpread) > 2*priceTick){
				return false;
			}
		}else{
			if(fabs(bidSpread - last_bidSpread) > 2*priceTick){
				return false;
			}
		}

		if(firstCloseIns->insType==E_INS_FORWARD){
			if((askSpread+stopLoss*priceTick) <= tradedSpread){
has_stop_lossed=true;
				return true;
			}else{
				return false;
			}
		}else{
			if((bidSpread+stopLoss*priceTick) <= tradedSpread){
has_stop_lossed=true;
				return true;
			}else{
				return false;
			}
		}
	}else{

		shared_ptr<Quote> mqt = mainIns->currentQuote;
		shared_ptr<Quote> sqt = mainIns->relativeIns->currentQuote;
		if(mqt->AskPrice1 - mqt->BidPrice1 > mainIns->priceTick){
			return false;
		}	
		if(sqt->AskPrice1 - sqt->BidPrice1 > 2 * mainIns->relativeIns->priceTick){
			return false;
		}	
		if(firstCloseIns->insType==E_INS_FORWARD){
			if(fabs(bidSpread - last_bidSpread) > 2*priceTick){
				return false;
			}
		}else{
			if(fabs(askSpread - last_askSpread) > 2*priceTick){
				return false;
			}
		}

		if(firstCloseIns->insType==E_INS_FORWARD){
			if((bidSpread-stopLoss*priceTick) >= tradedSpread){
has_stop_lossed=true;
				return true;
			}else{
				return false;
			}
		}else{
			if((askSpread-stopLoss*priceTick) >= tradedSpread){
has_stop_lossed=true;
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
			;	if(bidSpread<closeThreshold){
					return false;
				}
			}
		}
	}
	return true;
}

void Instrument::CalcQuoteDirection()
{

}
