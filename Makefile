CFLAGS = -std=c++17 -I. -I/opt/homebrew/Cellar/boost/1.80.0/include/boost \
	-I/opt/homebrew/include \
	`pkg-config --cflags-only-I portaudio-2.0 sndfile fftw3f spdlog`

LDFLAGS = -L/opt/homebrew/lib -l boost_thread-mt -lboost_system -lboost_chrono \
	`pkg-config --libs portaudio-2.0 sndfile fftw3f spdlog`


SOURCES = $(wildcard *.cpp) $(wildcard */*.cpp)

OBJS = $(wildcard *.o) $(wildcard */*.o)

OUTPUTFILE=wayver.a

wayver: main.cpp
	g++ $(CFLAGS) -o $(OUTPUTFILE) -g $(SOURCES) $(LDFLAGS) -Wall

clean:
	rm -f $(OUTPUTFILE) $(OBJS)