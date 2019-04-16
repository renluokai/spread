#include <algorithm>    // std::for_each
#include <iostream>
#include "order_manager.h"
#include "../include/trader.h"
using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))
Order* OrderManager::FillNewOrder(Order* o, const char* instrument, double price, int volume, EOpenClose oc, ELongShort ls)
{
	strncpy(o->instrument, instrument, sizeof(o->instrument));
	InstrumentInfo *insInfo;
	insInfo = Trader::GetTrader()->get_instrument_info(instrument);
	STRCPY(o->exchange_id, insInfo->ExchangeID.c_str());
	o->submit_price = price;
	o->submit_volume = volume;
	o->open_close = oc;
	o->long_short = ls;
	o->canceled_volume = 0;
	o->state = E_ORIGINAL;
	o->canceled_volume = 0;
	o->order_local_id = order_local_id++;

	return o;
}
bool OrderManager::ReportOrderState(Order* o)
{
	Order *tmp;
	//tmp = instrument_order_info[o->instrument].orders[ocls[o->order_local_id].oc][ocls[o->order_local_id].ls][o->order_local_id];
	tmp->state = o->state;
	STRCPY(tmp->state_msg, o->state_msg);
	return true;
}

bool OrderManager::UpdateOrder(Order* o)
{
	Order *tmp = NULL;
	int id=0;
	switch(o->state){
		case E_ORIGINAL:
			instrument_order_info[o->instrument].orders[o->open_close][o->long_short].insert(make_pair(o->order_local_id, o));
			ocls.insert(make_pair(o->order_local_id, Ocls(o->open_close,o->long_short)));
		break;
		case E_INSERT:

		break;
		case E_REJECT:
			id = o->order_local_id;
			tmp = instrument_order_info[o->instrument].orders[ocls[id].oc][ocls[id].ls][id];
			tmp->state = E_REJECT;
			STRCPY(tmp->state_msg, o->state_msg);
		break;
		case E_CANCEL:
		break;
		case E_MATCH:
		break;
	}
}
void ShowOrder(std::pair<int, Order*> io)
{
	cout<<io.first<<"\t";io.second->ShowOrder();
}
void ShowOrdersInfo(std::pair<string, OrderManager::Orders> os)
{
	int oc;
	int ls;	
	for(oc=E_OPEN;oc<E_OPENCLOSE;oc+=1){
		for(ls=E_LONG;ls<E_LONGSHORT;ls+=1){
			for_each(os.second.orders[oc][ls].begin(),
					os.second.orders[oc][ls].end(),
					ShowOrder);
		}
	}
}
void OrderManager::ShowOrders(const char *instrument)
{
	if(instrument == NULL){
		for_each(instrument_order_info.begin(),
				instrument_order_info.end(),
				ShowOrdersInfo);
	}else{
	}
}
