#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <iostream>
#include "../utility/tinyxml2.h"
#include "../include/trader.h"
#include "dong_0_strategy.h"

using namespace std;
using namespace tinyxml2;

#define STRCPY(a,b) strncpy((a),(b),sizeof(a))
Dong0Strategy::Dong0Strategy(int argc, char** argv)
{
	argc_ = argc;
	argv_ = argv;

	trader_ = Trader::GetTrader();
	handler_ = trader_->get_handler();

	quote_channel_ = new CtpQuoteChannel();
	trade_channel_ = new CtpTradeChannel();
	quote_cfg_ = new Config;			
	trade_cfg_ = new Config;			
	
	forward_contract_ = new char[32];
	recent_contract_ = new char[32];
	bzero(forward_contract_, 32);
	bzero(recent_contract_, 32);

	open_with_ = 0;
	close_with_ = 0;
	open_threshold_ = 0;
	close_threshold_ = 0;
	forward_cancel_max_ = 0;
	recent_cancel_max_ = 0;
	direction_ = 0;
	submit_max_ = 0;
	max_position_ = 0;
}
bool Dong0Strategy::load_config()
{
#define PARSE_ERROR(m) {cout<<(m)<<" MUST EXIST!\n";return false;}
	XMLDocument doc;
	XMLPrinter printer;
	XMLError xmlerr = doc.LoadFile(argv_[1]);
	if(xmlerr != XML_SUCCESS){
		cout<<doc.ErrorName()<<endl;
		cout<<"Config file load error, please check ..."<<endl;
		return false;
	}
	doc.Print(&printer);
	cout<<"----------"<<endl;
	cout<<printer.CStr();
	cout<<"----------"<<endl;

	XMLElement *root_element, *element, *child_element;

	root_element = doc.RootElement();

	element = root_element->FirstChildElement("forward_contract");
	if(!element)PARSE_ERROR("forward_contract");
	STRCPY(forward_contract_,element->GetText());

	element = root_element->FirstChildElement("recent_contract");
	if(!element)PARSE_ERROR("recent_contract");
	STRCPY(recent_contract_,element->GetText());

	element = root_element->FirstChildElement("open_with");
	if(!element)PARSE_ERROR("open_with");
	open_with_ = atoi(element->GetText());

	element = root_element->FirstChildElement("open_threshold");
	if(!element)PARSE_ERROR("open_threshold");
	open_threshold_ = atoi(element->GetText());

	element = root_element->FirstChildElement("close_with");
	if(!element)PARSE_ERROR("close_with");
	close_with_ = atoi(element->GetText());

	element = root_element->FirstChildElement("close_threshold");
	if(!element)PARSE_ERROR("close_threshold");
	close_threshold_ = atoi(element->GetText());

	element = root_element->FirstChildElement("direction");
	if(!element)PARSE_ERROR("direction");
	direction_ = atoi(element->GetText());

	element = root_element->FirstChildElement("max_postion");
	if(!element)PARSE_ERROR("max_postion");
	max_position_ = atoi(element->GetText());

	element = root_element->FirstChildElement("submit_max");
	if(!element)PARSE_ERROR("submit_max");
	submit_max_ = atoi(element->GetText());

	element = root_element->FirstChildElement("forward_cancel_max");
	if(!element)PARSE_ERROR("forward_cancel_max");
	forward_cancel_max_ = atoi(element->GetText());

	element = root_element->FirstChildElement("recent_cancel_max");
	if(!element)PARSE_ERROR("recent_cancel_max");
	recent_cancel_max_ = atoi(element->GetText());

	//parse the quote channel config
	element = root_element->FirstChildElement("quote_channel");
	if(!element)PARSE_ERROR("quote_channel");
	child_element = element->FirstChildElement("front");
	if(!child_element)PARSE_ERROR("front");
	STRCPY(quote_cfg_->front,child_element->GetText());

	child_element = element->FirstChildElement("user");
	if(!child_element)PARSE_ERROR("user");
	STRCPY(quote_cfg_->user,child_element->GetText());

	child_element = element->FirstChildElement("password");
	if(!child_element)PARSE_ERROR("password");
	STRCPY(quote_cfg_->password,child_element->GetText());

	//parse the trade channel config
	element = root_element->FirstChildElement("trade_channel");
	if(!element)PARSE_ERROR("trade_channel");
	child_element = element->FirstChildElement("front");
	if(!child_element)PARSE_ERROR("front");
	STRCPY(trade_cfg_->front,child_element->GetText());

	child_element = element->FirstChildElement("user");
	if(!child_element)PARSE_ERROR("user");
	STRCPY(trade_cfg_->user,child_element->GetText());

	child_element = element->FirstChildElement("password");
	if(!child_element)PARSE_ERROR("password");
	STRCPY(trade_cfg_->password,child_element->GetText());

	child_element = element->FirstChildElement("broker");
	if(!child_element)PARSE_ERROR("broker");
	STRCPY(trade_cfg_->broker_id,child_element->GetText());
	return true;
}

bool Dong0Strategy::on_init()
{
	bool ret = false;
	ret = load_config();
	ret = quote_channel_->open(quote_cfg_, handler_);	
	if(ret == false) return false;
	ret = trade_channel_->open(trade_cfg_, handler_);	
	if(ret == false) return false;
	trader_->RegisterQuoteChannel(quote_channel_,0);
	trader_->RegisterTradeChannel(trade_channel_,0);

	Instrument *forward_ins = new Instrument(forward_contract_);
	Instrument *recent_ins = new Instrument(recent_contract_);

	forward_ins->insType = E_INS_FORWARD;
	recent_ins->insType = E_INS_RECENT;

	forward_ins->relativeIns = recent_ins;
	recent_ins->relativeIns = forward_ins;

	forward_ins->cancelMax = forward_cancel_max_;
	recent_ins->cancelMax = recent_cancel_max_;
	
	Instrument::openThreshold =	open_threshold_;
	Instrument::closeThreshold = close_threshold_;

	Instrument::openWith = open_with_==0?E_INS_RECENT:E_INS_FORWARD;
	Instrument::closeWith = close_with_==0?E_INS_RECENT:E_INS_FORWARD;
	Instrument::submitMax = submit_max_;
	Instrument::maxPosition = max_position_;
	Instrument::direction = direction_==0 ? E_DIR_UP : E_DIR_DOWN;
	Instrument::loop = true;

	instruments.insert(make_pair(forward_contract_, forward_ins));
	instruments.insert(make_pair(recent_contract_, recent_ins));
	quote_channel_->subscribe(forward_contract_);
	quote_channel_->subscribe(recent_contract_);
	return true;
}
void Dong0Strategy::on_order(Order *o)
{
	Instrument *ins = instruments[o->instrument];
	switch(o->state)
	{
		case E_ORIGINAL:
			break;
		case E_INSERT:
			ins->on_insert(o);
			break;
		case E_REJECT:
			ins->on_reject(o);
			break;
		case E_CANCEL:
			ins->on_cancel(o);
			break;
		case E_MATCH:
			ins->on_match(o);
			break;
	}
}
void Dong0Strategy::on_quote(Quote *q)
{
	Instrument *ins = instruments[q->InstrumentID];
	ins->on_quote(q);
	static int i = 0;
	if(i== 5){
		Order* o = trader_->NewOrder(q->InstrumentID, q->AskPrice1, 1, E_OPEN, E_SHORT);
		trader_->submit_order(o);	
		cout<<"submit order"<<endl;
	}
}
void Dong0Strategy::on_error(Error  *e)
{
	cout<<"CALLING: "<<__FUNCTION__<<endl;
}
void Dong0Strategy::on_notify(Notify *n)
{
	cout<<"CALLING: "<<__FUNCTION__<<endl;
}
