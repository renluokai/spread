#ifndef INSTRUMENT_H__
#define INSTRUMENT_H__
class Quote;
class Order;
enum InsType
{
	E_FORWARD,
	E_RECCENT,
};
enum Direction
{
	E_UP,
	E_DOWN,
};
class Instrument
{
public:
	Instrument(char*);
	void on_quote(Quote*);	
	void on_match(Order*);	
	void on_reject(Order*);	
	void on_cancel(Order*);	
	void on_insert(Order*);	
	void ShowState();
private:
	char 		name[64];
	bool 		reached;
	InsType 	insType;
	Quote		*lastQuote;

	static double		spread;
	static double		openThreshold;
	static double		closeThreshold;
	static InsType		openWith;
	static InsType		closeWith; 
	static Direction	direction;
	static int			maxPosition;
	static int			submitMax;
};
#endif /* INSTRUMENT_H__ */ 
