#include <iostream>
#include <cstddef>
#include "../include/handler.h"
using namespace std;
Handler::Handler()
{
	main_data_   = new SafeList<Data>(0);

	order_pool_  = new SafeList<Order>(100);

	error_pool_  = new SafeList<Error>(100);

	quote_pool_  = new SafeList<Quote>(100);

	notify_pool_ = new SafeList<Notify>(100);

	command_pool_ = new SafeList<Command>(10);
}

//get data from main data list
Data* Handler::pop()
{
	return main_data_->pop();
}

//get a new place for specified data type
Data* Handler::GetPlace(EDataType type)
{
	Data* dt = NULL;
	switch(type){
		case E_ORDER_TYPE:
			dt = order_pool_->pop();
			break;
		case E_QUOTE_TYPE:
			dt = quote_pool_->pop();
			break;
		case E_ERROR_TYPE:
			dt = error_pool_->pop();
			break;
		case E_NOTIFY_TYPE:
			dt = notify_pool_->pop();
			break;
		case E_COMMAND:
			dt = command_pool_->pop();
			break;
	}
	return dt;
}

void Handler::back(Data *pdata)
{
	switch(pdata->type){
		case E_ORDER_TYPE:
			order_pool_->push((Order*)pdata);
			break;
		case E_QUOTE_TYPE:
			quote_pool_->push((Quote*)pdata);
			break;
		case E_ERROR_TYPE:
			error_pool_->push((Error*)pdata);
			break;
		case E_NOTIFY_TYPE:
			notify_pool_->push((Notify*)pdata);
			break;
		case E_COMMAND:
			command_pool_->push((Command*)pdata);
			break;
	}
}
void Handler::push(Data *pdata)
{
	Data* pd = NULL;
	switch(pdata->type){
		case E_ORDER_TYPE:
			pd = order_pool_->pop();
			*(Order*)pd = *(Order*)pdata;
			break;
		case E_QUOTE_TYPE:
			pd = quote_pool_->pop();
			*(Quote*)pd = *(Quote*)pdata;
			break;
		case E_ERROR_TYPE:
			pd = error_pool_->pop();
			*(Error*)pd = *(Error*)pdata;
			break;
		case E_NOTIFY_TYPE:
			pd = notify_pool_->pop();
			*(Notify*)pd = *(Notify*)pdata;
			break;
		case E_COMMAND:
			pd = command_pool_->pop();
			*(Command*)pd = *(Command*)pdata;	
			break;
		default:
			break;
	}

	if(pd != NULL){
		main_data_->push(pd);
	}
}
