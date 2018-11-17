OPENCV_LIBS =  -lopencv_highgui -lopencv_core -lopencv_videoio \
			   -lopencv_imgcodecs -lopencv_imgproc

GreenScreen: greenscreen.o
	g++ -o GreenScreen greenscreen.o $(OPENCV_LIBS)

greenscreen.o: greenscreen.cpp
	g++ -c -std=c++11 greenscreen.cpp