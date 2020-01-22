CXX:=cl.exe -nologo
CPPFLAGS:=-DUNICODE=1 -D_UNICODE=1
CXXFLAGS:=-EHsc -std:c++14 -W4 -WX -Zi -DEBUG:FASTLINK -MDd

jade_SRCS:=main.cpp
jade_OBJS:=$(jade_SRCS:.cpp=.obj)
jade_LIBS:=

all_TARGET=jade.exe
clean_TARGET=$(all_TARGET) $(all_TARGET:.exe=.pdb) $(all_TARGET:.exe=.ilk) $(jade_OBJS)

.PHONY=all clean

all: $(all_TARGET)
	gtags
clean:
	@for i in $(clean_TARGET) ; do if [ -f "$$i" ]; then rm $$i ; fi ; done
	@find . -type f -name '*~' -delete

jade.exe: $(jade_OBJS)
	$(LINK.cc) -Fe:$@ $(^) $($(basename $@)_LIBS)
%.obj: %.cpp verify.h
	$(COMPILE.cpp) -Fo:$@  $<
