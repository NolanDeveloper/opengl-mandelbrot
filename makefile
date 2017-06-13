LDLIBS += -lGL -lGLEW -lX11
CFLAGS += -g

main: main.o

clean:
	$(RM) main.o main
