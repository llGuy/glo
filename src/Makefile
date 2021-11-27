all: client server
	

client:
	gcc -o gloc -g net.c bitv.c math.c glo.c render.c io.c -lglfw -lGL -lGLEW -lm -DBUILD_CLIENT
server:
	gcc -o glos -g net.c bitv.c math.c glo.c render.c io.c -lglfw -lGL -lGLEW -lm

run:
	./gloc
