class DBFieldLength
{	public:
	static const size_t abbrev = 3;
	static const size_t banner = 6;
	static const size_t city = 100;
	static const size_t color = 16;
	static const size_t continentCode = 3;
	static const size_t continentName = 15;
	static const size_t countryCode = 3;
	static const size_t countryName = 32;
	static const size_t date = 10;
	static const size_t dcErrCode = 22;
	static const size_t graphCategory = 12;
	static const size_t graphDescr = 100;
	static const size_t graphFilename = 32;
	static const size_t graphFormat = 10;
	static const size_t label = 26;
	static const size_t level = 10;
	static const size_t regionCode = 8;
	static const size_t regionName = 48;
	static const size_t regiontype = 32;
	static const size_t root = 32;
	static const size_t route = 16;
	static const size_t routeLongName = 80;
	static const size_t statusChange = 16;
	static const size_t systemFullName = 60;
	static const size_t systemName = 10;
	static const size_t traveler = 48;
	static const size_t updateText = 1024;

	static const size_t countryRegion = countryName + regionName + 3;
	static const size_t dcErrValue = root + label + 1;;
};
