#ifndef ORDER_MANAGE__
#define ORDER_MANAGE__
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "../include/data_types.h"
#include "../include/safe_list.h"

using namespace std;
class OrderManager
{
public:
	OrderManager(){
		order_local_id = 0;
	}
	shared_ptr<Order> FillNewOrder(shared_ptr<Order> o, const char* instrument, double price, int volume, EOpenClose oc, ELongShort ls);

	shared_ptr<Order> UpdateOrder(shared_ptr<Order> o);
	bool HaveOrder(const char* ins);

	void ShowOrders(const char* ins=NULL);
	void GetOrder(const char* ins, EOpenClose oc, ELongShort ls, vector<shared_ptr<Order>> &ods);

	int GetVolume(const char* ins, EOpenClose oc, ELongShort ls);
	int GetOrderLocalId();

	struct Orders{
		map<int, shared_ptr<Order>> orders[E_OPENCLOSE][E_LONGSHORT];
	};
	struct Ocls{
		Ocls(){
			oc = E_OPENCLOSE;
			ls = E_LONGSHORT;
		}
		Ocls(EOpenClose o, ELongShort l){
			oc = o;
			ls = l;
		}
		Ocls(const Ocls &ref){
			oc = ref.oc;
			ls = ref.ls;
		}
		EOpenClose oc;
		ELongShort ls;
	};
private:
	map<string, Orders*> instrument_order_info;
	map<string, Orders*>::iterator order_info_iter;

	map<int, Ocls> ocls;
private:
	int order_local_id;
};
#endif /* ORDER_MANAGE__ */
