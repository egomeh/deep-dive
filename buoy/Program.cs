using System.Speech.Recognition;

class Buoy
{
    private bool quit;

    public static int Main()
    {
        Buoy buoy = new Buoy();
        return buoy.Run();
    }

    int Run()
    {
        string sonarDLLPath = Path.GetFullPath(@"../../../../Debug/sonar.dll");

        // Injection.InjectDLL("ColdWaters", sonarDLLPath);

        Voice voice = new Voice();
        voice.StartListeningForCommands();

        quit = false;
        while (!quit)
        {
            string? command = null;
            voice.commandQueue.TryDequeue(out command);

            if (!string.IsNullOrEmpty(command))
                Console.WriteLine(String.Format("Voice command: {0}", command));
        }

        return 0;
    }
}

