TARGET := SimpleWindowWL

.PHONY: $(TARGET) all clean

all: $(TARGET)

SOURCES := \
	WaylandCore.cpp \
	WaylandCore_registry_handlers.cpp \
	WaylandCore_seat_handlers.cpp \
	main.cpp \

LIBRARYS := \
	wayland-client \


OBJS := $(SOURCES:.cpp=.o)
LIBS := $(addprefix -l,$(LIBRARYS) )

DEPENDS = $(OBJS:.o=.d)

CFLAGS := -O0 -g -fpic -MMD -MP
CPPFLAGS := $(CFLAGS)


$(TARGET) : $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS)
	
%.o : %.cpp
	g++ -c $< -o $@ $(CPPFLAGS)

clean:
	rm -f $(TARGET)
	rm -f *.d *.o
 
-include $(DEPENDS)
