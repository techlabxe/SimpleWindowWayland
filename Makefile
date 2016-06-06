TARGET := SimpleWindowWL

.PHONY: $(TARGET) all clean

all: $(TARGET)

SOURCES := \
	WaylandCore.cpp \
	WaylandCore_registry_handlers.cpp \
	WaylandCore_seat_handlers.cpp \
	main.cpp \
	glshader.cpp \
	model.cpp \

LIBRARYS := \
	wayland-client \
	wayland-egl \
	EGL \
	GLESv2 \

OBJS := $(SOURCES:.cpp=.o)
LIBS := $(addprefix -l,$(LIBRARYS) )

DEPENDS = $(OBJS:.o=.d)

CFLAGS := -O0 -g -fpic -MMD -MP
CPPFLAGS := $(CFLAGS) -std=c++11


$(TARGET) : $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS)
	
%.o : %.cpp
	g++ -c $< -o $@ $(CPPFLAGS)

clean:
	rm -f $(TARGET)
	rm -f *.d *.o
 
-include $(DEPENDS)
