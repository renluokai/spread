#include <iostream>
#include <string.h>

#include "instrument.h"
#include "../include/data_types.h"
using namespace std;
#define STRCPY(a,b) strncpy((a),(b),sizeof(a))

double 		Instrument::spread = 0.0;
double		Instrument::openThreshold = 0.0;
double		Instrument::closeThreshold = 0.0;
InsType		Instrument::openWith = E_FORWARD;
InsType		Instrument::closeWith = E_FORWARD; 
Direction	Instrument::direction = E_UP;
int			Instrument::maxPosition = 0;
int			Instrument::submitMax = 0;

Instrument::Instrument(char *ins_name)
{
	STRCPY(name, ins_name);
	reached = false;
	lastQuote = new Quote;
}

void Instrument::ShowState()
{
}

void Instrument::on_quote(Quote *q)
{
	if(reached == false){
		reached = true;
	}
	if(insType == E_FORWARD){

	}
}

void Instrument::on_match(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

void Instrument::on_reject(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

void Instrument::on_cancel(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}

void Instrument::on_insert(Order*)
{
	cout<<__FUNCTION__<<": "<<name<<endl;
}
