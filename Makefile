
#VPATH = 

CS_OBJECTS = main.o claa_cs.o claa_tcp.o base64.o cJSON.o cmac.o aes.o
CS_INCLUDES = -I.
CS_DEFINES = 
CS_COMPILE_OPTIONS = -g
CS_LINK_OPTIONS = -posix -pthread
CS_LIBRARIES = -lc -ldl -lm -lpthread 

all: csdemo

csdemo: $(CS_OBJECTS)
	gcc -o $@ $(CS_OBJECTS) $(CS_LINK_OPTIONS) $(CS_LIBRARIES)

%.o: %.c
	gcc -c $(CS_COMPILE_OPTIONS) $(CS_DEFINES) $(CS_INCLUDES) $< -o $@

clean:
	rm -rf csdemo $(CS_OBJECTS)
