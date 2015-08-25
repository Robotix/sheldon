##Compile command
g++ -I/usr/local/include \`pkg-config --cflags --libs tesseract\` bot-detect.cpp -lopencv\_core -lopencv\_imgproc -lopencv\_highgui
