/**
   sort .list file entries by route number

   takes .list format contents on stdin, produces list of routes of each
   number somewhere in the .list file

   @author Jim Teresco
 */

import java.util.ArrayList;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;
import java.util.TreeMap;

public class RoutesByNumber {

    public static void main(String args[]) {

	// read from stdin
	Scanner in = new Scanner(System.in);

	// build a balanced BST of the routes traveled by number
	TreeMap<Integer, ArrayList<String>> map = new TreeMap<>();

	// process each .list format line
	while (in.hasNext()) {

	    // region string, which is not relevant
	    String region = in.next();
	    // skip comments
	    if (region.charAt(0) == '#') {
		in.nextLine();
		continue;
	    }

	    // read route and extract any numeric substring
	    String route = in.next();
	    String numberString = "";
	    for (int i = 0; i < route.length(); i++) {
		if (Character.isDigit(route.charAt(i))) {
		    numberString += route.charAt(i);
		}
	    }
	    int number = 0;
	    if (numberString.length() > 0) {
		number = Integer.parseInt(numberString);
	    }

	    // look up this number's list, create new if not in existence,
	    // add entry if exists and route is not already represented
	    ArrayList<String> routeList = map.get(number);
	    if (routeList == null) {
		routeList = new ArrayList<String>();
		routeList.add(route);
		map.put(number, routeList);
	    }
	    else {
		if (!routeList.contains(route)) {
		    routeList.add(route);
		}
	    }
	    in.nextLine();
	}
	
	in.close();

	// report results
	Set<Map.Entry<Integer,ArrayList<String>>> results = map.entrySet();

	for (Map.Entry<Integer,ArrayList<String>> routeSet : results) {
	    System.out.println(routeSet);
	}

	System.out.println("Numbers up to 999 for which no entries are found:");
	for (int i = 1; i <= 999; i++) {
	    if (!map.containsKey(i)) {
		System.out.print(i + " ");
	    }
	}
	System.out.println();
    }    
}
