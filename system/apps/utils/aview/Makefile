COPTS = -c

OBJDIR = objs/

OBJS = $(OBJDIR)main.o $(OBJDIR)appwindow.o $(OBJDIR)iview.o $(OBJDIR)statusbar.o
EXE = AView

CATALOGS = bg da de el es fr hu nl pl pt-br ru sv

all: $(OBJDIR) catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png
	rescopy -a $(EXE) resources/$(EXE).catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c $(EXE).cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t $(EXE).cd; \
		cd ..; \
	done;

$(OBJDIR):
	mkdir -p $(OBJDIR)

install: $(EXE)
	-mkdir -p /applications/AView
	-cp -pf $(EXE) /applications/AView/
	-ln -sf /applications/AView/$(EXE) /system/programs/aview

dist: $(EXE)
	-mkdir -p $(IMAGE)/applications/AView
	-cp -pf $(EXE) $(IMAGE)/applications/AView/
	-ln -sf /applications/AView/$(EXE) $(IMAGE)/system/programs/aview

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)main.o: main.cpp
	$(CXX) $(COPTS) main.cpp -o $@

$(OBJDIR)appwindow.o: appwindow.cpp appwindow.h
	$(CXX) $(COPTS) appwindow.cpp -o $@

$(OBJDIR)iview.o: iview.cpp iview.h
	$(CXX) $(COPTS) iview.cpp -o $@

$(OBJDIR)statusbar.o: statusbar.cpp statusbar.h
	$(CXX) $(COPTS) statusbar.cpp -o $@

$(OBJDIR)toolbar.o: toolbar.cpp toolbar.h
	$(CXX) $(COPTS) toolbar.cpp -o $@

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;
