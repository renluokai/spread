#ifndef DONG_0_STRATEGY_H__
#define DONG_0_STRATEGY_H__
#include "../include/istrategy.h"
#include "../channel/ctp/ctp_trade_channel.h"
#include "../channel/ctp/ctp_quote_channel.h"
#include "instrument.h"

class Dong0Strategy : public Strategy
{
public:
	Dong0Strategy(int argc, char** argv);
public:
	virtual bool on_init();
	virtual void on_order(Order *o);
	virtual void on_quote(Quote *q);
	virtual void on_error(Error *e);
	virtual void on_notify(Notify *n);
private:
	bool load_config();
private:
	map<string, Instrument*> instruments;
private:
	int argc_;
	char**	argv_;
	Trader* trader_;
	Handler* handler_;
	Config* quote_cfg_;
	Config* trade_cfg_;
	QuoteChannel* quote_channel_;
	TradeChannel* trade_channel_;
	char *forward_contract_;
	char *recent_contract_;
	int main_contract_;
	int open_with_;
	int close_with_;
	int open_threshold_;
	int close_threshold_;
	int forward_cancel_max_;
	int recent_cancel_max_;
	int direction_;
	int submit_max_;
	int trigger_spread_;
	int max_position_;
	int stop_loss_;
	int stop_loss_type_;
	
private:
	sem_t quit_sem_;
	pthread_t quote_thread_;
	pthread_t trade_thread_;
};
#endif /* DONG_0_STRATEGY_H__ */
