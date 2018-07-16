# tmg2html
From yakra/tmtools/tmg2html.<br>
The "official" version can be found at [this link](https://github.com/yakra/tmtools/tree/master/tmg2html).
<hr>
**Purpose:**<br>
Converts a .tmg [graph](http://travelmapping.net/graphs/) file into a web page with a Javascript "HDX-Lite" implementation using the HTML5 \<canvas\> tag.<br>
This can be used to check concurrencies and NMPs, without having to deal with the slowdown Leaflet experiences with larger datasets in the full [HDX](http://courses.teresco.org/metal/hdx/) implementation.

**Compiling:**<br>
C++11 support is required.<br>
With GCC, I use the commandline `g++ tmg2html.cpp -o tmg2html -std=c++11`<br>
tmg2html.cpp is all you need; no other files from the `tmtools` repository are required.

**Commandline:**<br>
`tmg2html <InputFile> <OutputFile>`

**Web page interface:**
* Pan by using the arrow keys, or double-clicking or dragging the map.
* Zoom by using the mouse wheel, or the `+` or `-` keys or buttons.
* Clicking a vertex or edge on the map will highlight it and display its info.
* Clicking a vertex or edge in the table will highlight & pan to it, and display its info.

**Compatibility:**<br>
* **Firefox** is recommended.
* **Chrome & Chromium:** Rendering & redrawing the map, or getting results after clicking a vertex or edge are considerably slower, especially with larger data sets or more data on screen.
* **Other browsers** have not been tested.
