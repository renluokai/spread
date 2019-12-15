#include <algorithm>    // std::for_each
#include <iostream>
#include "position_manager.h"
void PositionManager::UpdatePosition(string instrument, EOpenClose oc, ELongShort ls, int volume, double price, EPositionType  pt)
{
//if openclose is open, add a new entry at positionlist tail
//if openclose is close, sub volume from positionlist head	
	if(insPositionInfo.count(instrument)==0){
		insPositionInfo.insert(make_pair(instrument, PositionInfo()));
	}
	switch(oc){
		case E_OPEN:{
			PositionEntry pe;
			pe.instrument = instrument;
			pe.volume = volume;
			pe.price = price;
			pe.positionType = (pt==P_LONGSHORT) ? (ls==E_LONG ? P_TODAY_LONG : P_TODAY_SHORT) : pt;
			insPositionInfo[instrument].position[pe.positionType].positionList.push_back(pe);
			insPositionInfo[instrument].position[pe.positionType].totalPosition += volume;
		}
		break;
		case E_CLOSE:
		//TODO
		break;
		case E_CLOSE_T:
			if(ls==E_LONG){
				insPositionInfo[instrument].position[P_TODAY_LONG].positionList.sort();
				while(true){
					PositionEntry *pe = &insPositionInfo[instrument].position[P_TODAY_LONG].positionList.front();
					insPositionInfo[instrument].position[P_TODAY_LONG].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					insPositionInfo[instrument].position[P_TODAY_LONG].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}else{
				insPositionInfo[instrument].position[P_TODAY_SHORT].positionList.sort();
				while(true){
					PositionEntry *pe = &insPositionInfo[instrument].position[P_TODAY_SHORT].positionList.front();
					insPositionInfo[instrument].position[P_TODAY_SHORT].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					insPositionInfo[instrument].position[P_TODAY_SHORT].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}
		break;
		case E_CLOSE_Y:
			if(ls==E_LONG){
				insPositionInfo[instrument].position[P_YESTERDAY_LONG].positionList.sort();
				while(true){
					PositionEntry *pe = &insPositionInfo[instrument].position[P_YESTERDAY_LONG].positionList.front();
					insPositionInfo[instrument].position[P_YESTERDAY_LONG].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					insPositionInfo[instrument].position[P_YESTERDAY_LONG].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}else{
				insPositionInfo[instrument].position[P_YESTERDAY_SHORT].positionList.sort();
				while(true){
					PositionEntry *pe = &insPositionInfo[instrument].position[P_YESTERDAY_SHORT].positionList.front();
					insPositionInfo[instrument].position[P_YESTERDAY_SHORT].totalPosition -= volume;
					if(pe->volume > volume){
						pe->volume -= volume;	
						break;
					}else{
						volume -= pe->volume;
					}
					
					insPositionInfo[instrument].position[P_YESTERDAY_SHORT].positionList.pop_front();
					if(volume == 0){
						break;
					}
				}
			}
		break;
	}
	return;
}

int PositionManager::GetPosition(const char* ins, EPositionType posType)
{
	if(insPositionInfo.count(ins) > 0 ){
		return insPositionInfo[ins].position[posType].totalPosition;
	}
	return 0;
}

void PositionManager::ShowPosition(const char* ins)
{
	printf("INSTRUMENT\tY-L\tY-S\tT-L\tT-S\n");
	if(ins == NULL){
		map<string, PositionInfo>::iterator iter = insPositionInfo.begin();
		for(; iter!= insPositionInfo.end(); iter++){
			printf("%s\t\t%d\t%d\t%d\t%d\n", iter->first.c_str(),
					iter->second.position[P_YESTERDAY_LONG].totalPosition,
					iter->second.position[P_YESTERDAY_SHORT].totalPosition,
					iter->second.position[P_TODAY_LONG].totalPosition,
					iter->second.position[P_TODAY_SHORT].totalPosition);
		}
	}else if(insPositionInfo.count(ins) > 0){
		PositionInfo *tmp = &insPositionInfo[ins];
		printf("%s\t\t%d\t%d\t%d\t%d\n", ins,
				tmp->position[P_YESTERDAY_LONG].totalPosition,
				tmp->position[P_YESTERDAY_SHORT].totalPosition,
				tmp->position[P_TODAY_LONG].totalPosition,
				tmp->position[P_TODAY_SHORT].totalPosition);
	}else{
		printf("%s\t\t%d\t%d\t%d\t%d\n", ins, 0, 0, 0, 0);
	}
}

void PositionManager::UpdateQryMatch(string instrument, EOpenClose oc, ELongShort ls, int volume, double price)
{
	const char *o=NULL;
	const char *l=NULL;
	switch(oc){
	case E_OPEN:
		o = "E_OPEN\t";break;
	case E_CLOSE:
		o = "E_CLOSE\t";break;
	case E_CLOSE_T:
		o = "E_CLOSE_T\t";break;
	case E_CLOSE_Y:
		o = "E_CLOSE_Y\t";break;
	}
	switch(ls){
	case E_LONG:
		l = "E_LONG";break;
	case E_SHORT:
		l = "E_SHORT";break;
	}
	if(instrument_qry_match.count(instrument)==0){
		instrument_qry_match.insert(make_pair(instrument, QryMatch()));
	}
	PositionEntry positionEntry;
	positionEntry.instrument = instrument;
	positionEntry.volume = volume;
	positionEntry.price = price;
	positionEntry.positionType = P_LONGSHORT;
	instrument_qry_match[instrument].qryMatchList[oc][ls].push_back(positionEntry);
}

void PositionManager::GeneratePositionFromQryMatch()
{
			
	map<string, YesterdayPosition>::iterator iter;
	iter = instrument_yesterday_position.begin();
	for(;iter!=instrument_yesterday_position.end();iter++){
		UpdatePosition(iter->first, E_OPEN, E_LONG,
						iter->second.long_volume,
						iter->second.long_price,
						P_YESTERDAY_LONG);
		UpdatePosition(iter->first, E_OPEN, E_SHORT,
						iter->second.short_volume,
						iter->second.short_price,
						P_YESTERDAY_SHORT);
	}
	
	map<string, QryMatch>::iterator qmIter=instrument_qry_match.begin();
	for(;qmIter!=instrument_qry_match.end();qmIter++){
		list<PositionEntry>::iterator peIter;
		qmIter->second.qryMatchList[E_OPEN][E_LONG].sort();
		peIter = qmIter->second.qryMatchList[E_OPEN][E_LONG].begin();
		for(;peIter != qmIter->second.qryMatchList[E_OPEN][E_LONG].end();
			peIter++){
				UpdatePosition(qmIter->first, E_OPEN, E_LONG,
						peIter->volume,
						peIter->price,
						P_TODAY_LONG);
		}

		qmIter->second.qryMatchList[E_OPEN][E_SHORT].sort();
		peIter = qmIter->second.qryMatchList[E_OPEN][E_SHORT].begin();
		for(;peIter != qmIter->second.qryMatchList[E_OPEN][E_SHORT].end();
			peIter++){
				UpdatePosition(qmIter->first, E_OPEN, E_SHORT,
						peIter->volume,
						peIter->price,
						P_TODAY_SHORT);
		}

		qmIter->second.qryMatchList[E_CLOSE_Y][E_LONG].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_Y][E_LONG].begin();
		for(;peIter != qmIter->second.qryMatchList[E_CLOSE_Y][E_LONG].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_Y, E_LONG,
						peIter->volume,
						peIter->price);
		}

		qmIter->second.qryMatchList[E_CLOSE_Y][E_SHORT].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_Y][E_SHORT].begin();
		for(;peIter !=qmIter->second.qryMatchList[E_CLOSE_Y][E_SHORT].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_Y, E_SHORT,
						peIter->volume,
						peIter->price);
		}

		qmIter->second.qryMatchList[E_CLOSE_T][E_LONG].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_T][E_LONG].begin();
		for(;peIter != qmIter->second.qryMatchList[E_CLOSE_T][E_LONG].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_T, E_LONG,
						peIter->volume,
						peIter->price);
		}

		qmIter->second.qryMatchList[E_CLOSE_T][E_SHORT].sort();
		peIter = qmIter->second.qryMatchList[E_CLOSE_T][E_SHORT].begin();
		for(;peIter !=qmIter->second.qryMatchList[E_CLOSE_T][E_SHORT].end();
			peIter++){
				UpdatePosition(qmIter->first, E_CLOSE_T, E_SHORT,
						peIter->volume,
						peIter->price);
		}
	}
}

void PositionManager::UpdateYesterdayPosition(string instrument, ELongShort ls, int volume, double price)
{
	if(instrument_yesterday_position.count(instrument)==0){
		instrument_yesterday_position.insert(make_pair(instrument, YesterdayPosition()));
	}
	if(ls == E_LONG){
		instrument_yesterday_position[instrument].long_volume = volume;
		instrument_yesterday_position[instrument].long_price = price;
	}else{
		instrument_yesterday_position[instrument].short_volume = volume;
		instrument_yesterday_position[instrument].short_price = price;
	}
}
void ShowMatchEntry(PositionManager::PositionEntry pe){
}

void PositionManager::ShowQryMatch()
{
	map<string, QryMatch>::iterator iter;
	iter = instrument_qry_match.begin();
	for(;iter!=instrument_qry_match.end(); iter++){
		if(iter->second.qryMatchList[E_OPEN][E_LONG].size()>0){
			//cout<<"----------open long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_OPEN][E_LONG].begin(),
				(iter->second.qryMatchList)[E_OPEN][E_LONG].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE][E_LONG].size()>0){
			//cout<<"----------close long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE][E_LONG].begin(),
				(iter->second.qryMatchList)[E_CLOSE][E_LONG].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_T][E_LONG].size()>0){
			//cout<<"----------close today long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_T][E_LONG].begin(),
				(iter->second.qryMatchList)[E_CLOSE_T][E_LONG].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_Y][E_LONG].size()>0){
			//cout<<"----------close yesterday long----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_Y][E_LONG].begin(),
				(iter->second.qryMatchList)[E_CLOSE_Y][E_LONG].end(),
				ShowMatchEntry);
		}

		if(iter->second.qryMatchList[E_OPEN][E_SHORT].size()>0){
			//cout<<"----------open short----------"<<endl;
			for_each((iter->second.qryMatchList)[E_OPEN][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_OPEN][E_SHORT].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE][E_SHORT].size()>0){
			//cout<<"----------close short----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_CLOSE][E_SHORT].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_T][E_SHORT].size()>0){
			//cout<<"----------close today short----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_T][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_CLOSE_T][E_SHORT].end(),
				ShowMatchEntry);
		}
		if((iter->second.qryMatchList)[E_CLOSE_Y][E_SHORT].size()>0){
			//cout<<"----------close yesterday short----------"<<endl;
			for_each((iter->second.qryMatchList)[E_CLOSE_Y][E_SHORT].begin(),
				(iter->second.qryMatchList)[E_CLOSE_Y][E_SHORT].end(),
				ShowMatchEntry);
		}
	}
}

double PositionManager::GetHeadSpread(const char *forward,
									const char *recent,
									ELongShort ls, bool forwardIsMain)
{
	double spread=0.0;
	EPositionType forwardPe, recentPe;
	if(forwardIsMain == true){
		if(ls == E_LONG){
			forwardPe = P_TODAY_LONG;
			recentPe = P_TODAY_SHORT;
		}else{
			forwardPe = P_TODAY_SHORT;
			recentPe = P_TODAY_LONG;
		}
	}else{
		if(ls == E_LONG){
			forwardPe = P_TODAY_SHORT;
			recentPe = P_TODAY_LONG;
		}else{
			forwardPe = P_TODAY_LONG;
			recentPe = P_TODAY_SHORT;
		}
	}
	double f= insPositionInfo[forward].position[forwardPe].positionList.front().price;
	double r= insPositionInfo[recent].position[recentPe].positionList.front().price;
	spread = f - r;
	return spread;
}
double PositionManager::GetAverageSpread(const char *forward,
										const char *recent,
										int volumeYesterday,
										int volumeToday,
										ELongShort ls, bool forwardIsMain)
{
	double spread=0.0;

	EPositionType fPeT, rPeT, fPeY, rPeY;
	if(forwardIsMain == true){
		if(ls == E_LONG){
			fPeT = P_TODAY_LONG;
			rPeT = P_TODAY_SHORT;
			fPeY = P_YESTERDAY_LONG;
			rPeY = P_YESTERDAY_SHORT;
		}else{
			fPeT = P_TODAY_SHORT;
			rPeT = P_TODAY_LONG;
			fPeY = P_YESTERDAY_SHORT;
			rPeY = P_YESTERDAY_LONG;
		}
	}else{
		if(ls == E_LONG){
			fPeT = P_TODAY_SHORT;
			rPeT = P_TODAY_LONG;
			fPeY = P_YESTERDAY_SHORT;
			rPeY = P_YESTERDAY_LONG;
		}else{
			fPeT = P_TODAY_LONG;
			rPeT = P_TODAY_SHORT;
			fPeY = P_YESTERDAY_LONG;
			rPeY = P_YESTERDAY_SHORT;
		}
	}
	int totalVolume = volumeYesterday + volumeToday;
	double forwardTurnover=0.0;
	double recentTurnover=0.0;

	list<PositionEntry>::iterator iter;
	int fAddedT = 0, rAddedT = 0;
	int fAddedY = 0, rAddedY = 0;
	int remind;
	//calc forward turnover 
	iter = insPositionInfo[forward].position[fPeT].positionList.begin();
	for(;
		iter != insPositionInfo[forward].position[fPeT].positionList.end();
		iter++)
	{
		remind = volumeToday - fAddedT;
		if(iter->volume >= remind){
			forwardTurnover += iter->price*remind;
			fAddedT += remind;
			break;
		}else{
			forwardTurnover += iter->price*iter->volume;
			fAddedT += iter->volume;
		}
	}
	iter = insPositionInfo[forward].position[fPeY].positionList.begin();
	for(;
		iter != insPositionInfo[forward].position[fPeY].positionList.end();
		iter++)
	{
		remind = volumeYesterday - fAddedY;
		if(iter->volume >= remind){
			forwardTurnover += iter->price*remind;
			fAddedY += remind;
			break;
		}else{
			forwardTurnover += iter->price*iter->volume;
			fAddedY += iter->volume;
		}
	}


	//calc recent turnover 
	iter = insPositionInfo[recent].position[rPeT].positionList.begin();
	for(;
		iter != insPositionInfo[recent].position[rPeT].positionList.end();
		iter++)
	{
		remind = volumeToday - rAddedT;
		if(iter->volume >= remind){
			recentTurnover += iter->price*remind;
			rAddedT += remind;
			break;
		}else{
			recentTurnover += iter->price*iter->volume;
			rAddedT += iter->volume;
		}
	}
	iter = insPositionInfo[recent].position[rPeY].positionList.begin();
	for(;
		iter != insPositionInfo[recent].position[rPeY].positionList.end();
		iter++)
	{
		remind = volumeYesterday - rAddedY;
		if(iter->volume >= remind){
			recentTurnover += iter->price*remind;
			rAddedY += remind;
			break;
		}else{
			recentTurnover += iter->price*iter->volume;
			rAddedY += iter->volume;
		}
	}
	spread = forwardTurnover/totalVolume - recentTurnover/totalVolume;
	return spread;
}
