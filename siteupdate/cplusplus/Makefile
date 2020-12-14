all : siteupdate siteupdateST

siteupdate : \
  siteupdate.cpp */*.cpp \
  */*/*.h */*/*.cpp */*/*/*.cpp \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o
	g++ -std=c++11 -pthread -D threading_enabled -o siteupdate siteupdate.cpp \
	  classes/ClinchedDBValues/ClinchedDBValues.o \
	  classes/ElapsedTime/ElapsedTime.o \
	  classes/ErrorList/ErrorList.o

siteupdateST : \
  siteupdate.cpp \
  */*/*.h */*/*.cpp */*/*/*.cpp \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o \
  classes/*.cpp \
  functions/*.cpp \
  tasks/*.cpp
	g++ -std=c++11 -o siteupdateST siteupdate.cpp \
	  classes/ClinchedDBValues/ClinchedDBValues.o \
	  classes/ElapsedTime/ElapsedTime.o \
	  classes/ErrorList/ErrorList.o

classes/ClinchedDBValues/ClinchedDBValues.o : classes/ClinchedDBValues/ClinchedDBValues.h classes/ClinchedDBValues/ClinchedDBValues.cpp
	g++ -std=c++11 -o classes/ClinchedDBValues/ClinchedDBValues.o -c classes/ClinchedDBValues/ClinchedDBValues.cpp

classes/ElapsedTime/ElapsedTime.o : classes/ElapsedTime/ElapsedTime.h classes/ElapsedTime/ElapsedTime.cpp
	g++ -std=c++11 -o classes/ElapsedTime/ElapsedTime.o -c classes/ElapsedTime/ElapsedTime.cpp

classes/ErrorList/ErrorList.o : classes/ErrorList/ErrorList.h classes/ErrorList/ErrorList.cpp
	g++ -std=c++11 -o classes/ErrorList/ErrorList.o -c classes/ErrorList/ErrorList.cpp
