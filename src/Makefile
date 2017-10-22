CC=g++
Flag=
LIB=-lpthread
obj=led_i2c_command.cpp
test:led_i2c_command.cpp
	$(CC) -g $(LIB) $(FLAG) -o $@ $^
*.o:*.cpp
	$(CC) -g $(FLAG) $(LIB) -c $^


clean:
	rm -rf *.o
