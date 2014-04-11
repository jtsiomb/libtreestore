PREFIX = /usr/local

csrc = $(wildcard src/*.c)
ccsrc = $(wildcard src/*.cc)
cobj = $(csrc:.c=.o)
ccobj = $(ccsrc:.cc=.o)
obj = $(cobj) $(ccobj)
dep = $(obj:.o=.d) 

cname = treestore
ccname = treestorepp

capi_major = 0
capi_minor = 1
ccapi_major = 0
ccapi_minor = 1

clib_a = lib$(cname).a
cclib_a = lib$(ccname).a

ifeq ($(shell uname -s), Darwin)
	clib_so = lib$(cname).dylib
	cclib_so = lib$(ccname).dylib
	cshared = -dynamiclib
	ccshared = -dynamiclib
else
	clib_so = lib$(cname).so.$(capi_major).$(capi_minor)
	csoname = lib$(cname).so.$(capi_major)
	cdevlink = lib$(cname).so
	cclib_so = lib$(ccname).so.$(ccapi_major).$(ccapi_minor)
	ccsoname = lib$(ccname).so.$(ccapi_major)
	ccdevlink = lib$(ccname).so

	cshared = -shared -Wl,-soname=$(csoname)
	ccshared = -shared -Wl,-soname=$(ccsoname)
	pic = -fPIC
endif

dbg = -g
cxx11 = -std=c++11 -DTS_USE_CPP11

CFLAGS = -pedantic -Wall $(dbg) $(opt) $(pic)
CXXFLAGS = $(cxx11) $(CFLAGS)

.PHONY: all
all: $(clib_so) $(clib_a) $(cclib_so) $(cclib_a)

$(clib_a): $(cobj)
	$(AR) rcs $@ $(cobj)

$(clib_so): $(cobj)
	$(CC) -o $@ $(cshared) $(cobj) $(LDFLAGS)

$(cclib_a): $(ccobj)
	$(AR) rcs $@ $(ccobj)

$(cclib_so): $(ccobj)
	$(CXX) -o $@ $(ccshared) $(ccobj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.d: %.cc
	@$(CPP) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(clib_so) $(cclib_so) $(clib_a) $(cclib_a)

.PHONY: cleandep
cleandep: clean
	rm -f $(dep)


.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	cp src/treestore.h src/treestorepp.h $(DESTDIR)$(PREFIX)/include
	cp $(clib_a) $(clib_so) $(cclib_a) $(cclib_so) $(DESTDIR)$(PREFIX)/lib
	[ -n "$(csoname)" ] && \
		cd $(DESTDIR)$(PREFIX) && \
		rm -f $(csoname) $(cdevlink) $(ccsoname) $(ccdevlink) && \
		ln -s $(clib_so) $(csoname) && \
		ln -s $(cclib_so) $(ccsoname) && \
		ln -s $(csoname) $(cdevlink) && \
		ln -s $(ccsoname) $(ccdevlink) || \
		true
