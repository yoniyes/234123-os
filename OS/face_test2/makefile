CC=g++
CFLAGS= -c

all:Tester

Tester : HW2Test.o test_utils.o
	$(CC) HW2Test.o test_utils.o -o Tester

#ExampleTest.o : ExampleTest.cpp cutest.h test_utils.o
#	$(CC) $(CFLAGS) ExampleTest.cpp 

HW2Test.o : HW2Test.cpp cutest.h test_utils.o
	$(CC) $(CFLAGS) HW2Test.cpp 

test_utils.o : test_utils.cpp test_utils.hpp
	$(CC) $(CFLAGS) test_utils.cpp

clean:
	rm Tester *.o
