#include <iostream>
#include <time.h>

#include "position_manager.h"
#include "order_manager.h"

#include "../include/trader.h"

using namespace std;
Trader* Trader::instance_ = new Trader();

//static
Trader* Trader::GetTrader()
{
	return Trader::instance_;
}

//constructor
Trader::Trader()
{

	time_t now_sec = time(NULL);
	tm tm_now = *localtime(&now_sec);
	char buffer[256] = {0};
	sprintf(buffer, "./trader_%04d%02d%02d_%02d%02d%02d.log",
			tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday,
			tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
	log_stream.open(buffer, fstream::out|fstream::app);
	log_stream<<"*****************Trader STARTED*****************"<<endl;
	handler = new Handler;
	orderManager = new OrderManager;
	positionManager = new PositionManager;
	startToTrade = false;
}

void Trader::log(const char* msg)
{
	cout<<msg<<std::flush;
	log_stream<<msg<<std::flush;
}
//get handler, the main data center
Handler* Trader::get_handler()
{
	return handler;
}

//collect user input
void * user_command_fn(void* p)
{
	char *str;
	while(1){
		Command cmd;
		str = fgets(cmd.buffer, sizeof(cmd.buffer), stdin);
		Trader::GetTrader()->get_handler()->push(&cmd);
	}
}

//init trader
bool Trader::init(){
	pthread_create(&user_input_thread, NULL, user_command_fn, NULL);
	bool ret = false;
	ret = strategy->on_init();
	if(ret==false) return ret;
	positionManager->GeneratePositionFromQryMatch();
	return true;
}

void Trader::add_instrument_info(InstrumentInfo* info)
{
	string instrumentID = info->InstrumentID;
	if(instrumentInfo.find(instrumentID) == instrumentInfo.end()){
		InstrumentInfo *pInfo = new InstrumentInfo;
		*pInfo = *info;
		instrumentInfo.insert(make_pair(instrumentID, pInfo));
	}
}

//the main loop, dispatch data here
bool Trader::run(Strategy *s){
	strategy = s;

	bool ret = false;
	ret = init();
	if(ret == false)
		return false;

	size_t c=0;
	Data *data = NULL;
	Order* o = NULL;
	log("input :s to start system");
	while(1){
		//cout<<"#"<<c++<<" run running..."<<endl;
		data = handler->pop();
		switch(data->type){
			case E_ORDER_TYPE:
				o = (Order*)data;
				if(orderManager->UpdateOrder(o)){
					handler->back(data);
				}
				if(o->state == E_MATCH){
					UpdatePosition(o->instrument, o->open_close,
									o->long_short, o->match_volume, o->match_price);
				}
				strategy->on_order((Order*)data);
				break;
			case E_QUOTE_TYPE:
				if(startToTrade == false){
					break;
				}
				strategy->on_quote((Quote*)data);
				break;
			case E_ERROR_TYPE:
				strategy->on_error((Error*)data);
				break;
			case E_NOTIFY_TYPE:
				strategy->on_notify((Notify*)data);
				break;
			case E_COMMAND:
				process_command((Command*)data);
				break;
		}
		if(data->type != E_ORDER_TYPE){
			handler->back(data);
		}
	}
	return true;
}

void Trader::UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType pe)
{
	positionManager->UpdatePosition(instrument, oc, ls, volume, price, pe);
}
void Trader::process_command(Command* cmd)
{
	//cout<<"USER COMMAND PROCESSING:"<<cmd->buffer<<endl;
	if(strcmp(cmd->buffer,":q\n") == 0){
		cout<<"EXITING"<<endl;
	}
	else if(strcmp(cmd->buffer,":ls\n") == 0){
		insInfoIter = instrumentInfo.begin();
		for(;insInfoIter!= instrumentInfo.end();insInfoIter++){
			cout<<insInfoIter->first<<endl;	
		}
	}
	else if(strcmp(cmd->buffer,":lso\n") == 0){
		orderManager->ShowOrders();	
	}
	else if(strcmp(cmd->buffer,":lsp\n")==0){
		positionManager->ShowPosition(NULL);
	}
	else if(strcmp(cmd->buffer,":hm\n")==0){
		positionManager->ShowQryMatch();
	}
	else if(strcmp(cmd->buffer,":s\n")==0){
		if(startToTrade == false){
			startToTrade = true;
		}
	}
}

Order* Trader::NewOrder(const char* ins, double price, int volume, EOpenClose oc, ELongShort ls)
{
	Order* o = (Order*)handler->GetPlace(E_ORDER_TYPE);
	return orderManager->FillNewOrder(o, ins, price, volume, oc, ls);
}

InstrumentInfo* Trader::get_instrument_info(const char* ins)
{
	return instrumentInfo[ins];
}

bool Trader::RegisterQuoteChannel(QuoteChannel* channel, int id)
{
	if(quoteChannels.count(id) == 0){
		quoteChannels.insert(make_pair(id, channel));
		return true;
	}
	else
		return false;
}

bool Trader::RegisterTradeChannel(TradeChannel* channel, int id)
{
	if(tradeChannels.count(id) == 0){
		tradeChannels.insert(make_pair(id, channel));
		return true;
	}
	else
		return false;
}
int Trader::GetTradingDay(int id)
{
	int tradingDay=0;

	if(tradeChannels.count(id) != 0){
		return tradeChannels[id]->GetTradingDay();
	}
	return tradingDay;
}

bool Trader::submit_order(Order* o, int channel_id)
{
	bool ret = tradeChannels[channel_id]->submit(o);
	char buffer[256]={0};
	const char* ocls = NULL;
	if(o->open_close==E_OPEN){
		if(o->long_short==E_LONG){
			ocls = "OL";
		}else{
			ocls = "OS";
		}
	}else{
		if(o->long_short==E_LONG){
			ocls = "CL";
		}else{
			ocls = "CS";
		}
	}
	sprintf(buffer,"T O %s %s %d %.5f\n", ocls, o->instrument, o->submit_volume, o->submit_price);
	log(buffer);
	if(ret == true){
		orderManager->UpdateOrder(o);
	}
	return ret;
}

bool Trader::cancel_order(Order* o, int channel_id)
{
	char buffer[256]={0};
	const char* ocls = NULL;
	if(o->open_close == E_OPEN){
		if(o->long_short == E_LONG){
			ocls = "OL";
		}else{
			ocls = "OS";
		}
	}else{
		if(o->long_short == E_LONG){
			ocls = "CL";
		}else{
			ocls = "CS";
		}
	}
	sprintf(buffer,"T K %s %s %d\n", ocls, o->instrument, o->submit_volume-o->total_matched);
	log(buffer);
	o->ShowOrder();
	bool ret = tradeChannels[channel_id]->cancel(o);
	if(ret==true){
		o->canceling = true;
	}
	//TODO
	return  ret;
}

void Trader::GetOrder(const char* ins, EOpenClose oc, ELongShort ls, vector<Order*>& odVec)
{
	orderManager->GetOrder(ins, oc, ls, odVec);
}

int Trader::GetPosition(const char* ins, EPositionType posType)
{
	return positionManager->GetPosition(ins, posType);
}

int Trader::GetLongPosition(const char* ins)
{
	int yesterday=0, today=0;
	yesterday = positionManager->GetPosition(ins,P_YESTERDAY_LONG);
	today = positionManager->GetPosition(ins,P_TODAY_LONG);
	return yesterday + today;
}

int Trader::GetShortPosition(const char* ins)
{
	int yesterday=0, today=0;
	yesterday = positionManager->GetPosition(ins,P_YESTERDAY_SHORT);
	today = positionManager->GetPosition(ins,P_TODAY_SHORT);
	return yesterday + today;
}

int Trader::GetLongPositionYesterday(const char* ins)
{
	int yesterday=0;
	yesterday = positionManager->GetPosition(ins,P_YESTERDAY_LONG);
	return yesterday;
}
int Trader::GetShortPositionYesterday(const char* ins)
{
	int yesterday=0;
	yesterday = positionManager->GetPosition(ins,P_YESTERDAY_SHORT);
	return yesterday;
}
int Trader::GetLongPositionToday(const char* ins)
{
	int today=0;
	today = positionManager->GetPosition(ins,P_TODAY_LONG);
	return today;
}
int Trader::GetShortPositionToday(const char* ins)
{
	int today=0;
	today = positionManager->GetPosition(ins,P_TODAY_SHORT);
	return today;
}

void Trader::UpdateYesterdayPosition(string instrument, ELongShort ls, int volume, double price)
{
	positionManager->UpdateYesterdayPosition(instrument, ls, volume, price);
}

void Trader::UpdateQryMatch(string instrument, EOpenClose oc, ELongShort ls, int volume, double price)
{
	positionManager->UpdateQryMatch(instrument, oc, ls,volume, price);
}

double Trader::GetAverageSpread(const char *forward, const char *recent, 
						int volumeYesterday, int volumeToday,
						ELongShort ls, bool forwardIsMain)
{
	double spread=0.0;
	spread = positionManager->GetAverageSpread(forward, recent,
											volumeYesterday,
											volumeToday, ls,
											forwardIsMain);
	return spread;
}
double Trader::GetHeadSpread(const char *forward, const char *recent,
					ELongShort ls, bool forwardIsMain)
{
	double spread=0.0;
	spread = positionManager->GetHeadSpread(forward, recent, ls,
											forwardIsMain);
	return spread;
}
