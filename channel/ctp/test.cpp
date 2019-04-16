#include <iostream>
#include <string.h>
#include "ctp_quote_channel.h"
#include "ctp_trade_channel.h"
#include "../../include/handler.h"
#include "../../include/config.h"
using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))
int main()
{
	Config cfg;
	STRCPY(cfg.front, "tcp://180.168.146.187:10031");
	STRCPY(cfg.user, "115385");
	STRCPY(cfg.password, "Rlk170310");
	STRCPY(cfg.broker_id, "9999");

	Handler hdlr;
	CtpQuoteChannel ctp_channel;	
	int bret = ctp_channel.open(&cfg, &hdlr);
	cout<<"open status:"<<(bret==true?"true":"false")<<endl;
	//ctp_channel.subscribe("ni1905");
	//ctp_channel.subscribe("cu1905");
	CtpTradeChannel ctp_t_channel;	
	Config cfg_t;
	STRCPY(cfg_t.front, "tcp://180.168.146.187:10030");
	STRCPY(cfg_t.user, "115385");
	STRCPY(cfg_t.password, "Rlk170310");
	STRCPY(cfg_t.broker_id, "9999");

	bret = ctp_t_channel.open(&cfg_t, &hdlr);
	if(bret == true)
		cout<<"************************\nTrade channel started\n************************\n"<<endl;
	char s;
	while(1){
		cin>>s;

				Order o;
				strcpy(o.instrument, "cu1905");
				o.open_close = E_OPEN;
				o.long_short = E_LONG;
				o.submit_volume = 100;
				o.submit_price = 48500;
		switch(s){
			case 'i':
				ctp_t_channel.submit(&o);
				break;
			case 'c':
				ctp_t_channel.cancel(&o);
				break;
		}
	}
	return 0;
}
