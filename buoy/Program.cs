using System.Runtime.InteropServices;

class Buoy
{
    public static int Main()
    {
        Injection.InjectDLL("ColdWaters.exe", "something.dll");

        return 0;
    }
}
