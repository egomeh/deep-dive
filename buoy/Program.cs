using System.Speech.Recognition;
using System.IO.Pipes;
using System.Windows.Input;

class Buoy
{
    public static int Main()
    {
        Buoy buoy = new Buoy();
        return buoy.Run();
    }

    int Run()
    {
        string sonarDLLPath = Path.GetFullPath(@"../../../../Debug/sonar.dll");

        Comms comms = new Comms();
        comms.Initialize();
        comms.StartListen();

        Injection.InjectDLL("ColdWaters", sonarDLLPath);

        comms.WaitForConnection(30000);

        Voice voice = new Voice();
        voice.StartListeningForCommands();

        bool quit = false;
        while (!quit)
        {
            string? commandText = null;
            voice.commandQueue.TryDequeue(out commandText);

            if (!string.IsNullOrEmpty(commandText))
                Console.WriteLine(String.Format("Voice command: {0}", commandText));
            else
                continue;

            Command command = CommandParser.ParseCommand(commandText);

            if (command.type == CommandType.Invalid)
                continue;

            if (command.type == CommandType.Exit)
                quit = true;
            else comms.Send(command.GetDataToSend());
        }

        return 0;
    }
}

