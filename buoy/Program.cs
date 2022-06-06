using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Net;
using System.Runtime.Serialization;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text.RegularExpressions;

class Buoy
{
    public static int Main()
    {
        Buoy buoy = new Buoy();
        return buoy.Run();
    }

    int Run()
    {
        var assembly = Assembly.GetExecutingAssembly();
        string temporaryPath = Path.GetTempPath();
        string targetDllPath = Path.Combine(temporaryPath, "sonar.dll");

        using (Stream? inStream = assembly.GetManifestResourceStream("buoy.sonar.dll"))
        {
            if (inStream == null)
                return -1; // Probably handle this better but it should really not fail.

            using (FileStream outStream = File.OpenWrite(targetDllPath))
            {
                BinaryReader reader = new BinaryReader(inStream);
                BinaryWriter writer = new BinaryWriter(outStream);

                byte[] buffer = new Byte[1024];
                int bytesRead;

                while ((bytesRead = inStream.Read(buffer, 0, 1024)) > 0)
                {
                    outStream.Write(buffer, 0, bytesRead);
                }
            }
        }

        Injection.InjectDLL("ColdWaters", Path.GetFullPath(targetDllPath));

        Comms comms = new Comms();
        comms.Initialize();
        comms.StartListen();

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

