test: ../../src/helper.o ../../src/handler.o ../../src/trader.o ../../src/order_manager.o ctp_quote_channel.o ctp_trade_channel.o test.o
	g++ -lpthread  -L./ *.so  -o $@ $^

.PHONY: clean
clean:
	rm -rf *.o
