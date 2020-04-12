#ifndef FORECAST_H__
#define FORECAST_H__
#include "../include/data_types.h"

#include <memory>
using std::shared_ptr;

class Forecast{
public:
static bool 
	OrderWillSuccess(double price, shared_ptr<Quote> qt, EOpenClose oc, ELongShort ls);
static void
	QuoteDirection(Quote &qt);
static double volumeRatio;
};
#endif //FORECAST_H__
