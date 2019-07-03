#ifndef FORECAST_H__
#define FORECAST_H__
#include "../include/data_types.h"

class Forecast{
public:
static bool OrderWillSuccess(double price, Quote *forward, Quote *recent);
};
#endif //FORECAST_H__
