CC=g++
Flag=
LIB=-pthread
obj=led_i2c_command.cpp log.cpp
test:led_i2c_command.cpp log.cpp log.h
	$(CC) -g $(LIB) $(FLAG) -o $@ $^
*.o:*.cpp
	$(CC) -g $(FLAG) $(LIB) -c $^


clean:
	rm -rf *.o
	rm -f test
