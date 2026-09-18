using System.Threading.Tasks;
using Custom_EventHub;

public interface IDeviceConnectionService
{
    Task ConnectAsync(Device device, IEventHub _events);
    Task DisconnectAsync();

    void Send(string message);
    bool IsConnected { get; }
}