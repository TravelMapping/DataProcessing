import java.util.Scanner;

/**
   Convert all OSM URLs in the TM WPT file presented as input
   to have exactly 6 digits of precision.

   @author Jim Teresco
   @version February 2021

*/

public class SixDigits {

    public static void main(String args[]) {

	Scanner s = new Scanner(System.in);
	while (s.hasNextLine()) {
	    String line = s.nextLine();
	    String parts[] = line.split(" ");
	    // first is always a label
	    System.out.print(parts[0]);
	    // alt labels, if any
	    for (int i = 1; i < parts.length - 1; i++) {
		System.out.print(" " + parts[i]);
	    }
	    // URL
	    String urlParts[] = parts[parts.length-1].split("=");
	    urlParts[1] = urlParts[1].substring(0, urlParts[1].length()-4);
	    double lat = Double.parseDouble(urlParts[1]);
	    double lng = Double.parseDouble(urlParts[2]);
	    System.out.printf(" http://www.openstreetmap.org/?lat=%.6f&lon=%.6f\n", lat, lng);
	}
	s.close();
    }
}

