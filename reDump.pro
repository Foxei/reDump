CONFIG += c++11

TARGET = reDump

QMAKE_CXXFLAGS += -mfloat-abi=hard
QMAKE_CXXFLAGS += -O3

SOURCES += main.cpp

# Default rules for deployment.
target.path = /home/root/
!isEmpty(target.path): INSTALLS += target
