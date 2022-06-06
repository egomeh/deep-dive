
enum CommandType
{
    Invalid = 0,
    Exit = 1,
    MakeDepth = 2,
    Speed = 3,
    Rudder = 4,
    DivePlanes = 5,
    DropNoiseMaker = 6,
    SetCourse = 7,
    ShootTube = 8,
    LevelTheShip = 9,
    ReloadTube = 10,
}

struct Command
{
    public CommandType type;
    public byte[] rawData = new byte[0];

    public Command()
    {
        type = CommandType.Invalid;
        rawData= new byte[0];
    }

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

        if (text.Equals("rudder 0"))
        {
            return true;
        }

        if (text.Equals("rudder zero"))
        {
            return true;
        }

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

            if (turnRate > 30)
                turnRate = 30;

            if (left)
                turnRate = -turnRate;

            angle = turnRate;

            return true;
        }

        return false;
    }

    public static bool ParseDivePlaneCommand(string text, out int angle)
    {
        if (text.Equals("dive planes 0"))
        {
            angle = 0;
            return true;
        }

        if (text.Equals("dive planes zero"))
        {
            angle = 0;
            return true;
        }

        if (!Number(text, out angle, out text))
            return false;

        if (angle > 30)
            angle = 30;

        Whitespace(text, out text);

        if (!Match("degrees ", text, out text))
            return false;

        if (Match("up ", text, out text))
        {
            angle = -angle;
        }
        else if (Match("down ", text, out text))
        {
            // Nothing, just to make sure we catch "down "
        }
        else
        {
            return false;
        }

        if (!Match("angle", text, out text))
            return false;

        return true;
    }

    public static bool ParseDropNoiseMaker(string text)
    {
        if (text.StartsWith("drop noise maker"))
            return true;

        if (text.StartsWith("drop noisemaker"))
            return true;

        return false;
    }

    public static bool ParseSetCourseCommand(string text, out int bearing)
    {
        bearing = 0;

        if (!Match("set course bearing", text, out text))
            return false;

        Whitespace(text, out text);

        if (!Number(text, out bearing, out text))
            return false;

        return true;
    }

    public static bool ParseShootCommand(string text, out int bearing, out float distance, out int tube)
    {
        bearing = 0;
        distance = 0.0f;
        tube = 0;

        int n1 = 0;
        int n2 = 0;

        if (!Match("fire solution bearing", text, out text))
            return false;

        Whitespace(text, out text);

        if (!Number(text, out bearing, out text))
            return false;

        Whitespace(text, out text);

        if (!Match("distance", text, out text))
            return false;

        Whitespace(text, out text);

        if (!Number(text, out n1, out text))
            return false;

        Whitespace(text, out text);

        if (!Match("point", text, out text))
            return false;

        Whitespace(text, out text);

        if (!Number(text, out n2, out text))
            return false;

        Whitespace(text, out text);

        if (!Match("thousand yards", text, out text))
            return false;

        Whitespace(text, out text);

        if (!Match("shoot tube", text, out text))
            return false;

        Whitespace(text, out text);

        if (!Number(text, out tube, out text))
            return false;

        string distanceFLoatString = String.Format("{0}.{1}", n1, n2);
        distance = (float.Parse(distanceFLoatString) * 100.0f) / 7.5f;

        return true;
    }
    public static bool ParseLevelTheShipCommand(string text)
    {
        return Match("level the ship", text, out text);
    }

    public static bool ParseReloadTubeCommand(string text, out int tube)
    {
        tube = 0;

        if (Match("reload tube", text, out text))
            return false;

        if (!Number(text, out tube, out text))
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

        int divePlaneAngle;
        if (ParseDivePlaneCommand(text, out divePlaneAngle))
        {
            return new Command()
            {
                type = CommandType.DivePlanes,
                rawData = BitConverter.GetBytes(divePlaneAngle),
            };
        }

        if (ParseDropNoiseMaker(text))
        {
            return new Command()
            {
                type = CommandType.DropNoiseMaker
            };
        }

        int courseBearing;
        if (ParseSetCourseCommand(text, out courseBearing))
        {
            return new Command()
            {
                type = CommandType.SetCourse,
                rawData = BitConverter.GetBytes(courseBearing),
            };
        }

        if (ParseLevelTheShipCommand(text))
        {
            return new Command()
            {
                type = CommandType.LevelTheShip,
            };
        }

        int tubeToReload;
        if (ParseReloadTubeCommand(text, out tubeToReload))
        {
            return new Command()
            {
                type = CommandType.ReloadTube,
                rawData = BitConverter.GetBytes((int)tubeToReload),
            };
        }

        int shootBearing;
        float shootDistance;
        int tube;
        if (ParseShootCommand(text, out shootBearing, out shootDistance, out tube))
        {
            byte[] bearingRawData = BitConverter.GetBytes(shootBearing);
            byte[] distanceRawData = BitConverter.GetBytes(shootDistance);
            byte[] tubeRawData = BitConverter.GetBytes(tube);

            return new Command()
            {
                type = CommandType.ShootTube,
                rawData = bearingRawData.Concat(distanceRawData).Concat(tubeRawData).ToArray(),
            };
        }

        return new Command { type = CommandType.Invalid };
    }
}

