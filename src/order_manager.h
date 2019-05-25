#ifndef ORDER_MANAGE__
#define ORDER_MANAGE__
#include <map>
#include <vector>
#include <string>
#include "../include/data_types.h"
#include "../include/safe_list.h"

using namespace std;
class OrderManager
{
public:
	OrderManager(){
		order_local_id = 0;
	}
	Order* FillNewOrder(Order* o, const char* instrument, double price, int volume, EOpenClose oc, ELongShort ls);

	bool UpdateOrder(Order* o);
	bool HaveOrder(const char* ins);

	void ShowOrders(const char* ins=NULL);
	void GetOrder(const char* ins, EOpenClose oc, ELongShort ls, vector<Order*> &ods);

	int GetVolume(const char* ins, EOpenClose oc, ELongShort ls);
	int GetOrderLocalId();

	struct Orders{
		map<int, Order*> orders[E_OPENCLOSE][E_LONGSHORT];
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
