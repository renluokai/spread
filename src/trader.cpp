#include <iostream>

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
	handler = new Handler;
	orderManager = new OrderManager;
	positionManager = new PositionManager;
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
	return ret;
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

void Trader::UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price)
{
	positionManager->UpdatePosition(instrument, oc, ls, volume, price);
}
void Trader::process_command(Command* cmd)
{
	cout<<"USER COMMAND PROCESSING:"<<cmd->buffer<<endl;
	if(strcmp(cmd->buffer,":q\n") == 0){
		cout<<"EXITING"<<endl;
	}
	else if(strcmp(cmd->buffer,":ls\n") == 0){
		insInfoIter = instrumentInfo.begin();
		for(;insInfoIter!= instrumentInfo.end();insInfoIter++){
			cout<<insInfoIter->first<<endl;	
		}
	}
	else if(strcmp(cmd->buffer,":ls o\n") == 0){
		orderManager->ShowOrders();	
	}
	else if(strcmp(cmd->buffer,":lsp\n")==0){
		positionManager->ShowPosition(NULL);
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

bool Trader::submit_order(Order* o, int channel_id)
{
	bool ret = tradeChannels[channel_id]->submit(o);
	//TODO	
	if(ret == true){
		orderManager->UpdateOrder(o);
	}
	return ret;
}

bool Trader::cancel_order(Order* o, int channel_id)
{
	bool ret = tradeChannels[channel_id]->cancel(o);
	//TODO
	return  ret;
}
