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
	o->order_local_id = GetOrderLocalId();

	return o;
}

bool OrderManager::UpdateOrder(Order* o)
{
	Order *tmp = NULL;
	int id=0;
	switch(o->state){
		case E_ORIGINAL:
		{
			if(instrument_order_info.count(o->instrument) == 0){
				Orders* orders = new Orders;
				instrument_order_info.insert(make_pair(o->instrument, orders));
			}
			
			instrument_order_info[o->instrument]->orders[o->open_close][o->long_short].insert(make_pair(o->order_local_id, o));
			ocls.insert(make_pair(o->order_local_id, Ocls(o->open_close,o->long_short)));
			break;
		}
		case E_INSERT:
			if(o->order_local_id==-1){
				id = GetOrderLocalId();
				o->order_local_id = id;
				if(instrument_order_info.count(o->instrument) == 0){
					Orders* orders = new Orders;
					instrument_order_info.insert(make_pair(o->instrument, orders));
				}
				instrument_order_info[o->instrument]->orders[o->open_close][o->long_short].insert(make_pair(id, o));
				ocls.insert(make_pair(o->order_local_id, Ocls(o->open_close,o->long_short)));
			}
			id = o->order_local_id;
			tmp = instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls][id];
			tmp->state = E_INSERT;
			STRCPY(tmp->order_system_id, o->order_system_id);
			STRCPY(tmp->state_msg, o->state_msg);
		break;
		case E_REJECT:
			id = o->order_local_id;
			tmp = instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls][id];
			tmp->state = E_REJECT;
			STRCPY(tmp->state_msg, o->state_msg);
			instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls].erase(id);
			cout<<"AFTER RECECT:"<<instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls].size()<<endl;
			return true;
		case E_CANCEL:
			id = o->order_local_id;
			instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls].erase(id);
			cout<<"AFTER CANCEL:"<<instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls].size()<<endl;
			return true;
		break;
		case E_MATCH:
			id = o->order_local_id;
			tmp = instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls][id];
			tmp->total_matched += o->match_volume;
			o->stop_loss = tmp->stop_loss;
			if(tmp->total_matched == tmp->submit_volume){
				instrument_order_info[o->instrument]->orders[ocls[id].oc][ocls[id].ls].erase(id);
				return true;
			}
		break;
	}
	return false;
}
void ShowOrder(std::pair<int, Order*> io)
{
	cout<<io.first<<"\t";io.second->ShowOrder();
}
void ShowOrdersInfo(std::pair<string, OrderManager::Orders*> os)
{
	int oc;
	int ls;	
	for(oc=E_OPEN;oc<E_OPENCLOSE;oc+=1){
		for(ls=E_LONG;ls<E_LONGSHORT;ls+=1){
			for_each(os.second->orders[oc][ls].begin(),
					os.second->orders[oc][ls].end(),
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

bool OrderManager::HaveOrder(const char* ins)
{
	if(instrument_order_info.count(ins)==1){
		return true;
	}
	return false;
}
int OrderManager::GetVolume(const char* ins, EOpenClose oc, ELongShort ls)
{
	int volume = 0;
	if(HaveOrder(ins)){
		Orders* ods = instrument_order_info[ins];
		map<int, Order*>::iterator odIter = ods->orders[oc][ls].begin();	
		for(; odIter != ods->orders[oc][ls].end(); odIter++){
			Order* tmp = odIter->second;
			volume += (tmp->submit_volume - tmp->total_matched);
		}
	}
	return volume;
}
void OrderManager::GetOrder(const char* ins, EOpenClose oc, ELongShort ls, vector<Order*>& odVec)
{
	if(HaveOrder(ins)){
		Orders* ods = instrument_order_info[ins];
		map<int, Order*>::iterator odIter = ods->orders[oc][ls].begin();
		for(; odIter != ods->orders[oc][ls].end(); odIter++){
			Order* tmp = odIter->second;
			if(tmp==NULL)continue;
			odVec.push_back(tmp);
			{
				char buffer[256]={0};
				sprintf(buffer,"order pointer is:%p\n",tmp);
				Trader::GetTrader()->log(buffer);
			}
		}
	}
}

int OrderManager::GetOrderLocalId()
{
	return order_local_id++;
}
