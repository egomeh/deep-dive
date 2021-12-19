using System.Runtime.InteropServices;

class Buoy
{
    public static int Main()
    {
        string sonarDLLPath = Path.GetFullPath(@"../../../../Debug/sonar.dll");

        Injection.InjectDLL("ColdWaters", sonarDLLPath);

        return 0;
    }
}

