all : siteupdate siteupdateST

siteupdate : \
  siteupdate.cpp */*.cpp \
  */*/*.h */*/*.cpp */*/*/*.cpp \
  classes/Arguments/Arguments.o \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o \
  functions/crawl_hwy_data.o \
  functions/double_quotes.o \
  functions/format_clinched_mi.o \
  functions/lower.o \
  functions/split.o \
  functions/upper.o \
  functions/valid_num_str.o
	g++ -std=c++11 -pthread -D threading_enabled -o siteupdate siteupdate.cpp \
	  classes/Arguments/Arguments.o \
	  classes/ClinchedDBValues/ClinchedDBValues.o \
	  classes/ElapsedTime/ElapsedTime.o \
	  classes/ErrorList/ErrorList.o \
	  functions/crawl_hwy_data.o \
	  functions/double_quotes.o \
	  functions/format_clinched_mi.o \
	  functions/lower.o \
	  functions/split.o \
	  functions/upper.o \
	  functions/valid_num_str.o

siteupdateST : \
  siteupdate.cpp \
  */*/*.h */*/*.cpp */*/*/*.cpp \
  classes/*.cpp \
  classes/Arguments/Arguments.o \
  classes/ClinchedDBValues/ClinchedDBValues.o \
  classes/ElapsedTime/ElapsedTime.o \
  classes/ErrorList/ErrorList.o \
  functions/*.cpp \
  functions/crawl_hwy_data.o \
  functions/double_quotes.o \
  functions/format_clinched_mi.o \
  functions/lower.o \
  functions/split.o \
  functions/upper.o \
  functions/valid_num_str.o \
  tasks/*.cpp
	g++ -std=c++11 -o siteupdateST siteupdate.cpp \
	  classes/Arguments/Arguments.o \
	  classes/ClinchedDBValues/ClinchedDBValues.o \
	  classes/ElapsedTime/ElapsedTime.o \
	  classes/ErrorList/ErrorList.o \
	  functions/crawl_hwy_data.o \
	  functions/double_quotes.o \
	  functions/format_clinched_mi.o \
	  functions/lower.o \
	  functions/split.o \
	  functions/upper.o \
	  functions/valid_num_str.o

classes/Arguments/Arguments.o : classes/Arguments/Arguments.h classes/Arguments/Arguments.cpp
	g++ -std=c++11 -o classes/Arguments/Arguments.o -c classes/Arguments/Arguments.cpp

classes/ClinchedDBValues/ClinchedDBValues.o : classes/ClinchedDBValues/ClinchedDBValues.h classes/ClinchedDBValues/ClinchedDBValues.cpp
	g++ -std=c++11 -o classes/ClinchedDBValues/ClinchedDBValues.o -c classes/ClinchedDBValues/ClinchedDBValues.cpp

classes/ElapsedTime/ElapsedTime.o : classes/ElapsedTime/ElapsedTime.h classes/ElapsedTime/ElapsedTime.cpp
	g++ -std=c++11 -o classes/ElapsedTime/ElapsedTime.o -c classes/ElapsedTime/ElapsedTime.cpp

classes/ErrorList/ErrorList.o : classes/ErrorList/ErrorList.h classes/ErrorList/ErrorList.cpp
	g++ -std=c++11 -o classes/ErrorList/ErrorList.o -c classes/ErrorList/ErrorList.cpp

functions/crawl_hwy_data.o : functions/crawl_hwy_data.h functions/crawl_hwy_data.cpp
	g++ -std=c++11 -o functions/crawl_hwy_data.o -c functions/crawl_hwy_data.cpp

functions/double_quotes.o : functions/double_quotes.h functions/double_quotes.cpp
	g++ -std=c++11 -o functions/double_quotes.o -c functions/double_quotes.cpp

functions/format_clinched_mi.o : functions/format_clinched_mi.h functions/format_clinched_mi.cpp
	g++ -std=c++11 -o functions/format_clinched_mi.o -c functions/format_clinched_mi.cpp

functions/lower.o : functions/lower.h functions/lower.cpp
	g++ -std=c++11 -o functions/lower.o -c functions/lower.cpp

functions/split.o : functions/split.h functions/split.cpp
	g++ -std=c++11 -o functions/split.o -c functions/split.cpp

functions/upper.o : functions/upper.h functions/upper.cpp
	g++ -std=c++11 -o functions/upper.o -c functions/upper.cpp

functions/valid_num_str.o : functions/valid_num_str.h functions/valid_num_str.cpp
	g++ -std=c++11 -o functions/valid_num_str.o -c functions/valid_num_str.cpp
