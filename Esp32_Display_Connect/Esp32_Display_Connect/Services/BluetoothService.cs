using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Custom_EventHub;
using Linux.Bluetooth;
using Linux.Bluetooth.Extensions;
using Esp32_Display_Connect.Events;
using System.Text;

public sealed class BluetoothService : IBluetoothService
{
    private IAdapter1? _adapter;
    private IDevice1? _connectedDevice;

    private IGattCharacteristic1? _rxCharacteristic;
    private IGattCharacteristic1? _txCharacteristic;
    private IGattService1? _service;

    private IDisposable? _txNotificationWatch;

    public async Task<IReadOnlyList<BluetoothDevice>> ScanAsync(
        IEventHub _events,
        int sec = 20,
        CancellationToken cancellationToken = default
    ){
        _adapter ??= await GetAdapterAsync();
        var result = new List<BluetoothDevice>();

        Console.WriteLine($"Using Bluetooth adapter: {_adapter.ObjectPath}");
        
        // Watch for devices appearing during discovery.
        using var watch = await _adapter.WatchDevicesAddedAsync(
            async device => {
                try
                {
                    var bluetoothDevice = await CreateBluetoothDeviceAsync(device);
                    result.Add(bluetoothDevice);

                    _events.Publish(new BluetoothDiscoveredEvent(bluetoothDevice));

                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error reading Bluetooth device: {ex}");
                }
            });

        TimeSpan duration = TimeSpan.FromSeconds(sec);

        Console.WriteLine($"Starting BLE scan for {duration.TotalSeconds} seconds...");

        await _adapter.StartDiscoveryAsync();

        try
        {
            await Task.Delay(duration, cancellationToken);
        }
        catch (OperationCanceledException)
        {
            // Normal cancellation.
        }
        finally
        {
            await _adapter.StopDiscoveryAsync();

            Console.WriteLine("BLE scan stopped.");
        }

        return result;
    }

    public async Task<IReadOnlyList<BluetoothDevice>> GetKnownDeviceAsync()
    {
        _adapter ??= await GetAdapterAsync();

        var devices = await _adapter.GetDevicesAsync(); 
        var result = new List<BluetoothDevice>(); 
        
        Console.WriteLine($"Using Bluetooth adapter: {_adapter.ObjectPath}");

        foreach (var device in devices) 
        { 
            var bluetoothDevice = await CreateBluetoothDeviceAsync(device); 
            result.Add(bluetoothDevice); 
        } 

        Console.WriteLine($"Known devices: {result.Count}");

        return result;
    }

    public void PrintDeviceDescriptionAsync(BluetoothDevice device)
    {
        Console.WriteLine($"Device: {device.Name}");
        Console.WriteLine($"Address: {device.Address}");
        Console.WriteLine($"RSSI: {device.Rssi}");

        if (device.Uuids != null)
        {
            Console.WriteLine("UUIDs:");

            foreach (var uuid in device.Uuids)
                Console.WriteLine($"  {uuid}");
        }
        Console.WriteLine();
    }

    private async Task<IAdapter1> GetAdapterAsync()
    {
        var adapters = await BlueZManager.GetAdaptersAsync();

        if (adapters.Count == 0)
        {
            throw new InvalidOperationException("No Bluetooth adapters were found.");
        }

        return adapters.First();
    }

    private static async Task<BluetoothDevice> CreateBluetoothDeviceAsync(IDevice1 device)
    {
        var properties = await device.GetAllAsync();

        return new BluetoothDevice
        {
            Id = properties.Address,
            Address = properties.Address,
            Name = properties.Alias,
            Rssi = properties.RSSI,
            Uuids = properties.UUIDs,
            ObjectPath = device.ObjectPath.ToString()
        };
    }

    public async Task<BluetoothDevice> ConnectAsync(BluetoothDevice device)
    {
        _adapter ??= await GetAdapterAsync();
        var devices = await _adapter.GetDevicesAsync();
        var linuxDevice = devices.FirstOrDefault(d => d.ObjectPath.ToString() == device.ObjectPath);

        if (linuxDevice is null)
        {
            throw new InvalidOperationException($"Bluetooth device '{device.Address}' " + "could not be found.");
        }

        Console.WriteLine($"Connecting to {device.Name ?? device.Address}...");

        await linuxDevice.ConnectAsync();
        _connectedDevice = linuxDevice;

        await WaitForServicesResolvedAsync(linuxDevice, TimeSpan.FromSeconds(10));

        Console.WriteLine("BLE connected.");
        await DiscoverGattAsync(device);
        return device;
    }

    private async Task DiscoverGattAsync(BluetoothDevice device)
    {
        if (_connectedDevice is null)
            throw new InvalidOperationException("No Bluetooth device is connected.");

        var services = await _connectedDevice.GetServicesAsync();

        foreach (var service in services)
        {
            var serviceProperties = await service.GetAllAsync();

            if (!string.Equals(
                    serviceProperties.UUID,
                    Env.ServiceUuid,
                    StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            _service = service;

            var characteristics = await service.GetCharacteristicsAsync();

            foreach (var characteristic in characteristics)
            {
                var properties = await characteristic.GetAllAsync();

                if (properties.Flags.Contains("write"))
                {
                    device.RxCharacteristicUuid = properties.UUID;
                    _rxCharacteristic = characteristic;
                }

                if (properties.Flags.Contains("notify"))
                {
                    device.TxCharacteristicUuid = properties.UUID;
                    _txCharacteristic = characteristic;
                }
            }

            break;
        }

        if (_service is null)
            throw new InvalidOperationException("Service was not found.");

        if (_rxCharacteristic is null)
            throw new InvalidOperationException("RX characteristic was not found.");

        if (_txCharacteristic is null)
            throw new InvalidOperationException("TX characteristic was not found.");
    }

    private async Task WaitForServicesResolvedAsync(IDevice1 device, TimeSpan timeout)
    {
        var properties = await device.GetAllAsync();

        if (properties.ServicesResolved)
            return;

        var tcs = new TaskCompletionSource<bool>(
            TaskCreationOptions.RunContinuationsAsynchronously);

        using var timeoutCts = new CancellationTokenSource(timeout);

        timeoutCts.Token.Register(() =>
        {
            tcs.TrySetException(
                new TimeoutException("Bluetooth GATT service discovery timed out."));
        });

        using var watch = await device.WatchPropertiesAsync(
            changes =>
            {
                foreach (var change in changes.Changed)
                {
                    if (change.Key != "ServicesResolved")
                        continue;

                    if (change.Value is bool resolved && resolved)
                    {
                        tcs.TrySetResult(true);
                    }
                }
            });

        await tcs.Task;
    }

    public async Task DisconnectAsync()
    {
        if (_connectedDevice is null)
            return;
        if (_txCharacteristic is not null)
            await _txCharacteristic.StopNotifyAsync();

        await _connectedDevice.DisconnectAsync();
        
        _connectedDevice = null;
        _txCharacteristic = null;
        _rxCharacteristic = null;
        _service = null;

        Console.WriteLine("BLE disconnected.");
    }

    public async Task SendAsync(string message)
    {
        if (_connectedDevice is null)
            throw new InvalidOperationException("No Bluetooth device is connected.");

        if (_rxCharacteristic is null)
            throw new InvalidOperationException($"RX characteristic was not found.");

        var data = System.Text.Encoding.UTF8.GetBytes(message);

        await _rxCharacteristic.WriteValueAsync(data, new Dictionary<string, object>());
    }

    public async Task StartReceiveAsync(IEventHub _events)
    {
        Console.WriteLine("Start BLE RX.");

        if (_connectedDevice is null)
            throw new InvalidOperationException("No Bluetooth device is connected.");

        if (_txCharacteristic is null)
            throw new InvalidOperationException("TX characteristic was not found.");

        _txNotificationWatch = await _txCharacteristic.WatchPropertiesAsync(
            changes =>
            {
                foreach (var change in changes.Changed)
                {
                    if (change.Key != "Value")
                        continue;
                    if (change.Value is not byte[] bytes)
                        continue;

                    var message = Encoding.UTF8.GetString(bytes);
                    _events.Publish(new BluetoothReceiveEvent(message));
                }
            });

        await _txCharacteristic.StartNotifyAsync();

        Console.WriteLine("BLE RX started.");
    }

    public async Task StopReceiveAsync()
    {
        if (_txCharacteristic is not null)
            await _txCharacteristic.StopNotifyAsync();

        _txNotificationWatch?.Dispose();
        _txNotificationWatch = null;

        Console.WriteLine("BLE RX stopped.");
    }
}