CFLAGS := -g -Wall

all: pred

pred: pred.c
	gcc $(CFLAGS) predictors.c -o predictors

run-all: all
	./predictors short_trace1.txt output1.txt
	./predictors short_trace2.txt output2.txt
	./predictors short_trace3.txt output3.txt
	./predictors long_trace1.txt output4.txt
	./predictors long_trace2.txt output5.txt
	./predictors long_trace3.txt output6.txt	

clean:
	rm -f predictors special_output.txt output*.txt
