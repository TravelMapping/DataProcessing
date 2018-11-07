# nmpbycountry

**Purpose:**<br>
Splits *tm-master.nmp* into smaller .nmp files filtered by country.

**Compiling:**<br>
C++11 support is required.<br>
With GCC, I use the commandline `g++ nmpbycountry.cpp -std=c++11 -o nmpbycountry`

**Usage:**<br>
`nmpbycountry RgCsvFile MasterNMP OutputDir`, where
* `RgCsvFile` is the path of *regions.csv*
* `MasterNMP` is the path of *tm-master.nmp*
* `OutputDir` is the directory in which to write the resulting .nmp files. Trailing slash required.<br>

The output directory should be empty before running *nmpbycountry*. Files for countries without NMPs will not be written. Thus any existing file for a country that subsequently has all NMPs removed will not be overwritten.

---

# nmpbyregion

**Purpose:**<br>
Splits *tm-master.nmp* into smaller .nmp files filtered by region.

**Usage:**<br>
`nmpbyregion HwyDataDir MasterNMP OutputDir`, where
* `HwyDataDir` is the path of the *hwy_data* directory in the HighwayData repository, or equivalent
* `MasterNMP` is the path of *tm-master.nmp*
* `OutputDir` is the directory in which to write the resulting .nmp files. Trailing slash required.

---

# nmpfpfilter

**Purpose:**<br>
Removes marked false-positive pairs from a specified .nmp file.

**Usage:**<br>
`nmpfpfilter InputFile OutputFile`

---

# nmplifilter

**Purpose:**<br>
Removes all NMP pairs from a specified .nmp file except those that look intentional.

**Usage:**<br>
`nmplifilter InputFile OutputFile`
