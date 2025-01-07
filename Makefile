CXXFLAGS = -O3 -Wuninitialized -Wmaybe-uninitialized -D CXXCOMPILED
CXX = g++

cppbuild: main.o HTMLFile.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o cppbuild

winclean:
	del *.o cppbuild.exe