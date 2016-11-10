import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * 2016-6-22
 * CheckCommit.java
 * TODO check commit
 * liunianliang
 */

/**
 * @author liunianliang
 */
public class CheckCommit {

    private static String sCommit = null;
    private static List<String> sCommitFormatter = new ArrayList<String>();
    private static List<String> sHelpInfo = new ArrayList<String>();

    private static final String sDefaultFormatter = "(\\[.+?\\])+\\s*(\\w+)\\s*\\,\\s*(\\w+)\\s*\\,\\s*(.+?)\\s*";

    /**
     * @param args
     */
    public static void main(String[] args) {
        String myDir = System.getProperty("java.class.path");
        myDir = myDir.substring(0, myDir.lastIndexOf("/"));
        sCommit = args[0];

        //System.out.print("myDir = " + myDir + "\n");
        //System.out.print("sCommit = " + sCommit + "\n");

        try {
            Scanner in = new Scanner(new File(myDir + "/commit.cfg"));
            while (in.hasNextLine()) {
                String str = in.nextLine().toString().trim();
                parserConfig(str);
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        startCheck();
    }

    public static void parserConfig(String str) {
        if (str != null && !"".equals(str) && !str.startsWith("#") && !str.startsWith(">>")) {
            //System.out.print("mFormatter = " + str + "\n");
            sCommitFormatter.add(str);
        } else if (str != null && !"".equals(str) && str.startsWith(">>")) {
            sHelpInfo.add(str.replace(">>", "*") + "\n");
        }
    }

    public static void startCheck() {
        boolean sMatch = false;
        for (String formatter : sCommitFormatter) {
            Pattern pattern = Pattern.compile(formatter);
            Matcher matcher = pattern.matcher(sCommit);
            //System.out.print("startCheck -- > formatter = " + formatter + " sCommit = " + sCommit + "\n");
            if (matcher.matches()) {
                sMatch = true;
                break;
            }
        }
        if (!sMatch) {
            Pattern pattern = Pattern.compile(sDefaultFormatter);
            Matcher matcher = pattern.matcher(sCommit);
            sMatch = matcher.matches();
        }
        if (sMatch) {
            System.exit(0);
        } else {
            //System.out.print("Commit check fail !!!" + "\n");
            showHelp();
        }
        sMatch = false;
    }

    public static void showHelp() {
        for (String info : sHelpInfo) {
            if (!info.contains("[color]")) {
                System.out.print(info);
            } else if (info.contains("[color][green]")){
                info = info.replace("[color][green]", "").replace("*", "");
                System.out.printf("*\033[0;32m%s\033[0m",info);
            } else if (info.contains("[color][red]")){
                info = info.replace("[color][red]", "").replace("*", "");
                System.out.printf("*\033[0;31m%s\033[0m",info);
            }
        }
        System.exit(-1);
    }
}
