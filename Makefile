SOURCES=closest_2.c image_downsize.c

lib512.so: $(SOURCES)
	gcc -fPIC -shared $(SOURCES) -o $@ 

