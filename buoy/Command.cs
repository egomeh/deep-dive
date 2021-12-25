
enum CommandType
{
    Invalid = 0,
    Exit = 1,
    MakeDepth = 2,
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

        while (Char.IsDigit(text[howManyDigits]))
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

        return new Command { type = CommandType.Invalid };
    }
}

