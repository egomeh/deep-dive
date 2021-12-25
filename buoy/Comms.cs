using System.Net;
using System.Net.Sockets;

class Comms
{
    int port = 30010;
    Socket? listenerSocket = null;
    Socket? sessionSocket = null;
    Task<Socket>? acceptTask = null;

    public  Comms()
    {
    }

    public bool Initialize()
    {
        IPAddress ipAddress = IPAddress.Any;
        IPEndPoint localEndPoint = new IPEndPoint(ipAddress, port);

        // Create a TCP/IP socket.  
        listenerSocket = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
        listenerSocket.Bind(localEndPoint);
        listenerSocket.Listen(port);

        return true;
    }

    public bool StartListen()
    {
        if (listenerSocket == null)
            return false;

        acceptTask = listenerSocket.AcceptAsync();

        return true;
    }

    public bool WaitForConnection(int millisecondTimeout)
    {
        if (acceptTask == null)
            return false;

        acceptTask.Wait(millisecondTimeout);

        if (!acceptTask.IsCompletedSuccessfully)
            return false;

        sessionSocket = acceptTask.Result;

        return true;
    }

    public bool Send(byte[] data)
    {
        if (sessionSocket == null)
            return false;

        return sessionSocket.Send(data) == data.Length;
    }
}
