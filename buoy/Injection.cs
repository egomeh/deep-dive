using System.Diagnostics;
using System.Runtime.InteropServices;

class Injection
{
    [DllImport("kernel32.dll")]
    public static extern IntPtr OpenProcess(int dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    public static bool InjectDLL(string ProcessName, string DLLPath)
    {
        Process[] processes = Process.GetProcessesByName("Cold*");

        return true;
    }
}
