using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Custom_EventHub;

public interface IBluetoothService
{
    Task<IReadOnlyList<BluetoothDevice>> ScanAsync(
        IEventHub _events,
        int sec = 20,
        CancellationToken cancellationToken = default);

    Task<IReadOnlyList<BluetoothDevice>> GetKnownDeviceAsync();
    void PrintDeviceDescriptionAsync(BluetoothDevice device);
    Task<BluetoothDevice> ConnectAsync(BluetoothDevice device);
    Task SendAsync(string message);
    Task StartReceiveAsync(IEventHub _events);
    Task StopReceiveAsync();
    Task DisconnectAsync();
}