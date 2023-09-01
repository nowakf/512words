SOURCES=image.c closest_utf8.c png.c lodepng/lodepng.c

lib512.so: $(SOURCES)
	gcc -fPIC -shared $(SOURCES) -o $@ 

