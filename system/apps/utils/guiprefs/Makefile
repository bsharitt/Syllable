COPTS	= -c

OBJS	= main.o fontpanel.o screenpanel.o windowpanel.o keyboardpanel.o coloredit.o
EXE	= guiprefs

LIBS	= -lm -lsyllable

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $@

deps:

install: $(EXE)
	cp -f $(EXE) /system/programs/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(IMAGE)/system/programs/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
