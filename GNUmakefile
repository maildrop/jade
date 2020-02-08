TARGET_OS_VERSION:=_WIN32_WINNT_WIN7 
CXX:=cl.exe -nologo
CPPFLAGS:=-DUNICODE=1 -D_UNICODE=1 -DWINVER=$(TARGET_OS_VERSION) -D_WIN32_WINNT=$(TARGET_OS_VERSION) -Iimgui -Ivcpkg-export-20200207-182453\installed\x64-windows\include
CXXFLAGS:=-EHsc -std:c++14 -O2 -W4 -WX -Zi -DEBUG:FASTLINK -MDd
VPATH:=imgui:imgui/examples
jade_SRCS:=main.cpp imgui.cpp imgui_widgets.cpp imgui_draw.cpp imgui_impl_win32.cpp
jade_OBJS:=$(jade_SRCS:.cpp=.obj)
jade_LIBS:=User32.lib Ole32.lib Gdi32.lib Comctl32.lib \
	sqlite3.lib \
	OpenCL.lib avcodec.lib avdevice.lib avfilter.lib avformat.lib avresample.lib avutil.lib \
	libcrypto.lib libssl.lib libx264.lib postproc.lib swresample.lib swscale.lib vpxmdd.lib zlibd.lib bz2d.lib lzmad.lib 

all_TARGET=jade.exe
clean_TARGET=$(all_TARGET) $(all_TARGET:.exe=.exe.manifest) $(all_TARGET:.exe=.pdb) $(all_TARGET:.exe=.ilk) $(jade_OBJS) GTAGS GPATH GRTAGS vc140.pdb

.PHONY=all clean spawn test

all: $(all_TARGET)

test:
	PATH=.:vcpkg-export-20200207-182453\installed\x64-windows\debug\bin jade.exe

clean:
	@for i in $(clean_TARGET) ; do if [ -f "$$i" ]; then rm $$i ; fi ; done
	@find . -type f -name '*~' -delete

jade.exe: $(jade_OBJS)
	$(LINK.cc) -Fe:$@ $(^) $($(basename $@)_LIBS) -link -DEBUG -LIBPATH:vcpkg-export-20200207-182453\installed\x64-windows\debug\lib
%.obj: %.cpp verify.h
	$(COMPILE.cpp) -Fo:$@  $<
