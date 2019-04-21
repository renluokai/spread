#ifndef INSTRUMENT_H__
#define INSTRUMENT_H__
class Quote;
class Order;
enum InsType
{
	E_INS_INVALID,
	E_INS_FORWARD,
	E_INS_RECENT,
};
enum Direction
{

	E_DIR_INVALID,
	E_DIR_UP,
	E_DIR_DOWN,
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

	char 		name[64];
	bool 		reached;
	InsType 	insType;
	Instrument	*relativeIns;
	Quote		*lastQuote;
	int			cancelMax;

	static double		spread;
	static double		openThreshold;
	static double		closeThreshold;
	static InsType		openWith;
	static InsType		closeWith; 
	static Direction	direction;
	static int		maxPosition;
	static int		submitMax;
	static bool		loop;
};
#endif /* INSTRUMENT_H__ */ 
