import java.io.*;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.MalformedURLException;

public class DriverLoader extends URLClassLoader{
    public DriverLoader(){
        super(new URL[]{});
    }

    public  DriverLoader(URL[] path){
        super(path);
    }

    public DriverLoader(String path) throws MalformedURLException, IOException{
        super(new URL[]{DriverLoader.toURL(path)});
    }

    public void addPath(String path) throws MalformedURLException, IOException{
        URL jarfile = DriverLoader.toURL(path);
        try {
            addURL(jarfile);
        }catch(Exception e){
            e.printStackTrace();
            throw e;
        }
    }

    public static URL toURL(String path) throws MalformedURLException, IOException{
        File jarFile = new File(path);
        return jarFile.toURI().toURL();
    }

    public static void main(String[] args) {
        try {
            DriverLoader d = new DriverLoader();
            d.addPath(args[0]);
            d.loadClass(args[1]);
        }catch (Exception e){
            System.out.println("exception");
            e.printStackTrace();
            System.exit(1);
        }
    }
}
