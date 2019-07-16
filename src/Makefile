.PHONY: t
.PHONY: clean
.PHONY: clean
.PHONY: log
vpath %.h ../include
CPPFLAGS += -g -std=gnu++11
utility := ../utility/tinyxml2.o
shangdong0 := ../strategy/instrument.o ../strategy/dong_0_strategy.o ../strategy/forecast.o
channels := ../channel/ctp/ctp_quote_channel.o ../channel/ctp/ctp_trade_channel.o
objects := $(utility) helper.o handler.o $(channels) $(shangdong0) position_manager.o order_manager.o trader.o trade_main.o 
t: $(objects)  
	rm -rf *.log
	g++  -lpthread -L../channel/ctp/ -lthosttraderapi_se -lthostmduserapi_se -o $@ $^
	rm -rf $(objects)
*.o:*.cpp
*.o:*.cpp
	g++ -g -c  $@ $^

log:
	@mv *.log history_log	
	@mv *.con history_log	
clean:
	rm -rf $(objects)	
#%.d: %.cpp
#	g++ -M $(CPPFLAGS) $< > $@.$$$$;					\
#	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;	\
#	#rm -f $@.$$$$
