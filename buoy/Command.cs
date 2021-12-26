
enum CommandType
{
    Invalid = 0,
    Exit = 1,
    MakeDepth = 2,
    Speed = 3,
    Rudder = 4,
}

struct Command
{
    public CommandType type;
    public byte[] rawData;

    public byte[] GetDataToSend()
    {
        byte[] data = new byte[rawData.Length + 4];
        BitConverter.GetBytes((int)type).CopyTo(data, 0);
        rawData.CopyTo(data, 4);
        return data;
    }
}

class CommandParser
{
    public static void Whitespace(string text, out string after)
    {
        after = text;

        while (Char.IsWhiteSpace(after[0]))
            after = after.Substring(1);
    }

    public static bool Match(string pattern, string text, out string after)
    {
        after = text;

        if (after.StartsWith(pattern))
        {
            after = after.Substring(pattern.Length);
            return true;
        }

        return false;
    }

    public static bool Number(string text, out int number, out string after)
    {
        after = text;

        number = 0;
        int howManyDigits = 0;

        while (howManyDigits < text.Length && Char.IsDigit(text[howManyDigits]))
            howManyDigits++;

        if (howManyDigits > 0)
        {
            string numberString = text.Substring(0, howManyDigits);
            number = Convert.ToInt32(numberString);
            after = after.Substring(howManyDigits);
            return true;
        }

        return false;
    }

    public static bool ParseMakeDepthCommand(string commandText, out int depth)
    {
        depth = 0;

        if (!Match("make depth", commandText, out commandText))
            return false;

        Whitespace(commandText, out commandText);

        if (!Number(commandText, out depth, out commandText))
            return false;

        if (!Match(" feet", commandText, out commandText))
            return false;

        return true;
    }

    public static bool ParseExitCommand(string commandText)
    {
        if (!Match("exit", commandText, out commandText))
            return false;

        return true;
    }

    public static bool ParseSpeedCommand(string commandText, out int speedSetting)
    {
        speedSetting = 0;
        bool matchedAnySpeed = false;

        if (Match("dead slow ", commandText, out commandText))
        {
            matchedAnySpeed = true;
            speedSetting = 2;
        }

        if (Match("slow ", commandText, out commandText))
        {
            matchedAnySpeed = true;
            speedSetting = 3;
        }

        if (Match("half ", commandText, out commandText))
        {
            matchedAnySpeed = true;
            speedSetting = 4;
        }

        if (Match("full ", commandText, out commandText))
        {
            matchedAnySpeed = true;
            speedSetting = 5;
        }

        if (Match("flank ", commandText, out commandText))
        {
            matchedAnySpeed = true;
            speedSetting = 6;
        }

        if (!matchedAnySpeed)
            return false;


        if (!Match("ahead", commandText, out commandText))
            return false;

        return true;
    }

    public static bool ParseRudderCommand(string text, out int angle)
    {
        angle = 0;

        if (Match("full left rudder", text, out text))
        {
            angle = -30;
            return true;
        }

        if (Match("full right rudder", text, out text))
        {
            angle = 30;
            return true;
        }

        if (Match("turn ", text, out text))
        {
            bool left = false;

            if (Match("left", text, out text))
                left = true;
            else if (Match("right", text, out text))
                left = false;
            else
                return false;

            Whitespace(text, out text);

            int turnRate = 0;
            if (!Number(text, out turnRate, out text))
                return false;

            if (left)
                turnRate = -turnRate;

            angle = turnRate;

            return true;
        }

        return false;
    }

    public static Command ParseCommand(string text)
    {
        if (ParseExitCommand(text))
        {
            return new Command()
            {
                type = CommandType.Exit,
                rawData = new byte[0],
            };
        }

        int depth;
        if (ParseMakeDepthCommand(text, out depth))
        {
            return new Command()
            {
                type = CommandType.MakeDepth,
                rawData = BitConverter.GetBytes(depth),
            };
        }

        int speedSetting;
        if (ParseSpeedCommand(text, out speedSetting))
        {
            return new Command()
            {
                type = CommandType.Speed,
                rawData = BitConverter.GetBytes(speedSetting),
            };
        }

        int rudderAngle;
        if (ParseRudderCommand(text, out rudderAngle))
        {
            return new Command()
            {
                type = CommandType.Rudder,
                rawData = BitConverter.GetBytes(rudderAngle),
            };
        }

        return new Command { type = CommandType.Invalid };
    }
}

