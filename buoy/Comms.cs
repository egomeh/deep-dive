using System.Net;
using System.Net.Sockets;

class Comms
{
    int port = 1234;
    Socket? listenerSocket = null;
    Socket? sessionSocket = null;

    public  Comms()
    {
    }

    public bool Initialize()
    {
        IPHostEntry ipHostInfo = Dns.GetHostEntry(Dns.GetHostName());
        IPAddress ipAddress = ipHostInfo.AddressList[0];
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

        sessionSocket = listenerSocket.Accept();

        return true;
    }

    public bool Send(byte[] data)
    {
        if (sessionSocket == null)
            return false;

        return sessionSocket.Send(data) == data.Length;
    }
}
