LDLIBS += -lGL -lGLEW -lX11 -lm
CFLAGS += -g

OBJ += main.o
OBJ += framework.o

main: $(OBJ)

clean:
	$(RM) $(OBJ) main
