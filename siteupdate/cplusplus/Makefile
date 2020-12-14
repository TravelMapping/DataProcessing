all : siteupdate siteupdateST

siteupdate : siteupdate.cpp */*.cpp */*/*.h */*/*.cpp */*/*/*.cpp
	g++ -std=c++11 -pthread -D threading_enabled -o siteupdate siteupdate.cpp

siteupdateST : siteupdate.cpp */*/*.h */*/*.cpp */*/*/*.cpp classes/*.cpp functions/*.cpp tasks/*.cpp
	g++ -std=c++11 -o siteupdateST siteupdate.cpp
