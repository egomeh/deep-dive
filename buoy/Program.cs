using System.Speech.Recognition;
using System.IO.Pipes;

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

        var server = new NamedPipeServerStream("ColdPipe");

        Injection.InjectDLL("ColdWaters", sonarDLLPath);

        server.WaitForConnection();
        StreamReader reader = new StreamReader(server);
        StreamWriter writer = new StreamWriter(server);

        Voice voice = new Voice();
        voice.StartListeningForCommands();

        quit = false;
        while (!quit)
        {
            string? command = null;
            voice.commandQueue.TryDequeue(out command);

            if (!string.IsNullOrEmpty(command))
                Console.WriteLine(String.Format("Voice command: {0}", command));
            else
                continue;

            if (command.StartsWith("make depth "))
            {   
                command = command.Remove(0, 11);
                command = command.Remove(command.Length - 5, 5);
                writer.WriteLine(command);
                writer.Flush();
            }
        }

        return 0;
    }
}

