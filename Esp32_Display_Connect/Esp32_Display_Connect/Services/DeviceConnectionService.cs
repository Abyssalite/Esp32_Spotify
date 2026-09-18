using System;
using System.Net.WebSockets;
using System.Text.Json;
using System.Threading.Tasks;
using Custom_EventHub;
using Esp32_Display_Connect.Events;
using Websocket.Client;

public class DeviceConnectionService : IDeviceConnectionService
{
    private WebsocketClient? _wsClient;
    public bool IsConnected => _wsClient?.IsStarted ?? false;

    public async Task ConnectAsync(Device device, IEventHub _events)
    {
        await DisconnectAsync();

        var url = new Uri($"ws://{device.Address}/ws");

        _wsClient = new WebsocketClient(url)
        {
            ReconnectTimeout = TimeSpan.FromSeconds(10),
            ErrorReconnectTimeout = TimeSpan.FromSeconds(10)
        };

        _wsClient.MessageReceived.Subscribe(msg =>
        {
            try
            {
                var status = JsonSerializer.Deserialize<DeviceStatus>(msg.Text ?? "");

                if (status != null)
                {
                    _events.Publish(new StatusReceivedEvent(status));
                }
            }
            catch(Exception ex)
            {
                Console.WriteLine($"JSON error: {ex.Message}");
            }
        });

        _wsClient.ReconnectionHappened.Subscribe(info =>
        {
            _events.Publish(new ConnectionStatusChangedEvent($"Status: {info.Type}"));
        });

        await _wsClient.Start();
    }

    public void Send(string message)
    {
        if (_wsClient?.IsStarted == true)
        {
            try
            {
                _wsClient.Send(message);
            }
            catch(Exception ex)
            {
                Console.WriteLine($"WS send error: {ex.Message}");
            }
        }
    }

    public async Task DisconnectAsync()
    {
        if (_wsClient == null)
            return;

        try
        {
            if (_wsClient.IsStarted)
            {
                await _wsClient.Stop(WebSocketCloseStatus.NormalClosure, "Disconnect");
            }
        }
        catch(Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
        finally
        {
            _wsClient.Dispose();
            _wsClient = null;
        }
    }
}