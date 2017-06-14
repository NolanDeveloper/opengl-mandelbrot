LDLIBS += -lGL -lGLEW -lX11 -lm
CFLAGS += -g

main: main.o

clean:
	$(RM) main.o main
