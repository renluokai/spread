#ifndef FORECAST_H__
#define FORECAST_H__
#include "../include/data_types.h"

class Forecast{
public:
static bool 
	OrderWillSuccess(double price, Quote *qt, EOpenClose oc, ELongShort ls);
static double volumeRatio;
};
#endif //FORECAST_H__
