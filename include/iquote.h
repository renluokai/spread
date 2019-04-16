#ifndef I_QUOTE_H__
#define I_QUOTE_H__
class Config;
class Handler;
class QuoteChannel{
public:
	virtual bool open(Config*, Handler*)=0;
	virtual bool close()=0;
	virtual bool subscribe(const char*)=0;
	virtual bool unsubscribe(const char*)=0;
};
#endif /* I_QUOTE_H__ */	
