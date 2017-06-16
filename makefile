LDLIBS += -lGL -lGLEW -lX11 -lm
CFLAGS += -g -Iupng

OBJ += main.o
OBJ += framework.o
OBJ += png.o
OBJ += upng.o

main: $(OBJ)

upng.o: upng/upng.c upng/upng.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	$(RM) $(OBJ) main
