
TARGET = output

all:
	gcc -o $(TARGET) -Wall setup.c snapshot.c
clean:
	rm -rf $(TARGET)