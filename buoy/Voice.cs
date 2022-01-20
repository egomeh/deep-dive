using System.Speech.Recognition;
using System.Collections.Concurrent;

class Voice
{
    public ConcurrentQueue<string> commandQueue;
    SpeechRecognitionEngine sr;

    public Voice()
    {
        commandQueue = new ConcurrentQueue<String>();
        sr = new SpeechRecognitionEngine();
    }

    public void StartListeningForCommands()
    {
        List<GrammarBuilder> commands = new List<GrammarBuilder>();

        List<string> numbers = new List<string>();

        for (int i = 0; i <= 1000; ++i)
        {
            numbers.Add(String.Format("{0}", i));
        }

        Choices zeroToThousand = new Choices(numbers.ToArray());

        // dive at/make depth
        Choices divePrefix = new Choices(new String[] { "make depth", "dive at" });
        GrammarBuilder depthCommandGrammarBuilder = new GrammarBuilder();
        depthCommandGrammarBuilder.Append(divePrefix);
        depthCommandGrammarBuilder.Append(zeroToThousand);
        depthCommandGrammarBuilder.Append("feet");
        commands.Add(depthCommandGrammarBuilder);

        // set speed
        Choices speedOptions = new Choices(new String[] { "dead slow", "slow", "half", "full", "flank" });
        GrammarBuilder speedCommandBuilder = new GrammarBuilder();
        speedCommandBuilder.Append(speedOptions);
        speedCommandBuilder.Append("ahead");
        commands.Add(speedCommandBuilder);

        // full rudder
        Choices directionOption = new Choices(new String[] { "left", "right" });
        GrammarBuilder fullRudderBuilder = new GrammarBuilder();
        fullRudderBuilder.Append("full");
        fullRudderBuilder.Append(directionOption);
        fullRudderBuilder.Append("rudder");
        commands.Add(fullRudderBuilder);

        // turn rudder
        GrammarBuilder rudderTurnBuilder = new GrammarBuilder();
        rudderTurnBuilder.Append("turn");
        rudderTurnBuilder.Append(directionOption);
        rudderTurnBuilder.Append(zeroToThousand);
        commands.Add(rudderTurnBuilder);

        // angle dive planes
        Choices upDownOption = new Choices(new String[] { "up", "down" });
        GrammarBuilder divePlaneBuilder = new GrammarBuilder();
        divePlaneBuilder.Append(zeroToThousand);
        divePlaneBuilder.Append("degrees");
        divePlaneBuilder.Append(upDownOption);
        divePlaneBuilder.Append("angle");
        commands.Add(divePlaneBuilder);

        // set course
        GrammarBuilder setCourseBuilder = new GrammarBuilder();
        setCourseBuilder.Append("set course bearing");
        setCourseBuilder.Append(zeroToThousand);
        commands.Add(setCourseBuilder);

        commands.Add(new GrammarBuilder("steady"));
        commands.Add(new GrammarBuilder("drop noise maker"));
        commands.Add(new GrammarBuilder("exit"));
        commands.Add(new GrammarBuilder("dive planes zero"));
        commands.Add(new GrammarBuilder("rudder zero"));
        commands.Add(new GrammarBuilder("shoot"));
        

        Choices allChoices = new Choices(commands.ToArray());
        GrammarBuilder finalCommand = new GrammarBuilder(allChoices);
        Grammar grammar = new Grammar(finalCommand);
        sr.LoadGrammar(grammar);

        // sr.SpeechRecognized += new EventHandler(sr_SpeechRecognized);
        sr.SpeechRecognized += sr_SpeechRecognized;

        sr.SetInputToDefaultAudioDevice();
        sr.RecognizeAsync(RecognizeMode.Multiple);
    }

    void sr_SpeechRecognized(object? sender, SpeechRecognizedEventArgs e)
    {
        commandQueue.Enqueue(e.Result.Text);
    }
}
