#ifndef HANDLER_H__
#define HANDLER_H__ 
#include "safe_list.h"
#include "data_types.h"
class Handler{
public:
	Handler();
public:
	Data *pop();
	void push(Data *);
	void back(Data *);
	Data* GetPlace(EDataType type); 
private:
	SafeList<Data> *main_data_;	

private:
	SafeList<Order>	*order_pool_;	
	SafeList<Error> *error_pool_;	
	SafeList<Quote> *quote_pool_;	
	SafeList<Notify> *notify_pool_;	
	SafeList<Command> *command_pool_;
};
#endif /* HANDLER_H__ */
