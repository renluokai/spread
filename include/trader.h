#ifndef TRADER_H__
#define TRADER_H__
#include <map>
#include <vector>
#include <string>
#include "istrategy.h"
#include "handler.h"
#include "data_types.h"
#include "../include/itrade.h"
#include "../include/iquote.h"
using namespace std;

class OrderManager;
class PositionManager;

class Trader{
public:
	static Trader* GetTrader();
public:
	Trader();
	Handler* get_handler();
public:
	bool run(Strategy *s);
	bool init();
	bool submit_order(Order*, int channel_id=0);
	bool cancel_order(Order*, int channel_id=0);
	bool RegisterQuoteChannel(QuoteChannel*, int id);
	bool RegisterTradeChannel(TradeChannel*, int id);

	void GetOrder(const char* ins, EOpenClose oc, ELongShort ls, vector<Order*>& odVec);

	Order* NewOrder(const char* instrument, double price, int volume, EOpenClose oc, ELongShort ls);
public:
	void add_instrument_info(InstrumentInfo*);
	void UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price);
	InstrumentInfo* get_instrument_info(const char* ins); 
private:
	void process_command(Command*);

private:
	//manage all order
	OrderManager *orderManager;
	//manage all position
	PositionManager *positionManager;
	//data pool
	Handler		*handler;
private:
	Strategy	*strategy;
private:
	map<string, InstrumentInfo*> instrumentInfo;
	map<string, InstrumentInfo*>::iterator insInfoIter;
	map<int, QuoteChannel*> quoteChannels;
	map<int, TradeChannel*> tradeChannels;
private:
	pthread_t user_input_thread;	
private:
	static Trader* instance_;
};
#endif /* TRADER_H_  */
