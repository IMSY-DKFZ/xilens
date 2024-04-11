CXX=g++
SOURCES=$(wildcard *.cpp)
PROGRAMS=$(SOURCES:%.cpp=%)
OPENCV_CXXFLAGS=`pkg-config --cflags opencv4`
OPENCV_LIBS=`pkg-config --libs opencv4`

all: $(PROGRAMS)

accessImageAfterClosingDevice:
	$(CXX) $(OPENCV_CXXFLAGS) -g -c accessImageAfterClosingDevice.cpp -o accessImageAfterClosingDevice.o
	$(CXX) -g accessImageAfterClosingDevice.o -o accessImageAfterClosingDevice -lm3api -lopencv_core

cameraSpeed:
	$(CXX) -g -c cameraSpeed.cpp -o cameraSpeed.o
	$(CXX) -g cameraSpeed.o -o cameraSpeed -lm3api

clean:
	rm -f *.o $(PROGRAMS)
