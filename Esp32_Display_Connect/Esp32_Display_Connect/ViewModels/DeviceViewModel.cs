using System;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using CommunityToolkit.Mvvm.Input;
using Custom_EventHub;
using Custom_Navigation;
using Custom_Popup;
using Esp32_Display_Connect.Events;
using Esp32_Display_Connect.Popup;

namespace Esp32_Display_Connect.ViewModels;

public partial class DeviceViewModel : ViewModelBase, IHandleBackNavigation
{    
    private CancellationTokenSource? _delayToken;

    private readonly IDeviceConnectionService _connection;
    public DeviceStatus? DeviceInfo { set; get; }
    public Device? SelectedDevice { get; }

    private bool? _toggleSpin  = false;
    public bool? ToggleSpin 
    { 
        get => _toggleSpin;
        set
        {
            if (value == null || value == _toggleSpin) return;

            _toggleSpin = value;
            var send = new JsonObject
            {
                ["Data"] = "IsSpin",
                ["IsSpin"] = _toggleSpin,
            };
            _connection.Send(JsonSerializer.Serialize(send));
            OnPropertyChanged(nameof(ToggleSpin));
        }
    }
    private float? _angle = 0.0f;
    public float? Angle 
    { 
        get => _angle;
        set
        {
            if (value == null || value == _angle) return;

            _angle = value;
            var send = new JsonObject
            {
                ["Data"] = "Angle",
                ["Angle"] = _angle
            };
            _connection.Send(JsonSerializer.Serialize(send));
            OnPropertyChanged(nameof(Angle));
        }
    }

    public string? Status { set; get; }
    public ICommand? ChangeWifiCommand { get; }
    public ICommand? ChangeLastfmCommand { get; }
    public ICommand? ChangeIpCommand { get; }
    public ICommand? DebugViewCommand { get; }

    public DeviceViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events,
        IDeviceConnectionService connection,
        IPopupHost popup
    ):base(store, navigator, events, popup)
    {        
        _connection = connection;
        SelectedDevice = _store.SelectedDevice;
        if (SelectedDevice == null) return;

        ChangeIpCommand = new AsyncRelayCommand(updateIp);
        ChangeWifiCommand = new AsyncRelayCommand(updateWifi);
        ChangeLastfmCommand = new AsyncRelayCommand(updateLastfm);
        DebugViewCommand = new AsyncRelayCommand(() => 
            _  = _popup.ShowNotifyPopup(new ConnectPopupViewModel(_popup, _events))
        );

        _subscriptions.Add(_events.Subscribe<StatusReceivedEvent>(async evt =>
        {
            DeviceInfo = evt.deviceStatus;
            OnPropertyChanged(nameof(DeviceInfo));
            _events.Publish(new DeviceLogsChangedEvent(DeviceInfo.ImgUrl, DeviceInfo.Logs));
            ToggleSpin = DeviceInfo.IsSpinMode;

        }));
        _subscriptions.Add(_events.Subscribe<ConnectionStatusChangedEvent>(async evt =>
        {
            Status = evt.connectionStatus;
            OnPropertyChanged(nameof(Status));
        }));

        _ = ConnectAsync();
    }

    private async Task updateWifi()
    {
        var input = new InputPopupViewModel(_popup, "Wifi Name", "Password");
        var tmp = await _popup.ShowInputPopup(input);
        if (tmp is UpdateInput wifi)
        {
            var send = new JsonObject
            {
                ["Data"] = "Wifi",
                ["Ssid"] = wifi.FirstField,
                ["Pass"] = wifi.SecondField
            };
            _connection.Send(JsonSerializer.Serialize(send));
        }
    }
    private async Task updateLastfm()
    {
        var input = new InputPopupViewModel(_popup, "User ID", "API Key");
        var tmp = await _popup.ShowInputPopup(input);
        if (tmp is UpdateInput lastfm)
        {
            var send = new JsonObject
            {
                ["Data"] = "lastfm",
                ["User"] = lastfm.FirstField,
                ["Key"] = lastfm.SecondField
            };
            _connection.Send(JsonSerializer.Serialize(send));
        }
    }

    private async Task updateIp()
    {
        var input = new InputPopupViewModel(_popup, "IP Address");
        var tmp = await _popup.ShowInputPopup(input);
        if (tmp is UpdateInput ip && SelectedDevice != null)
        {
            var result = await _store.StoreUpdateDeviceIp(SelectedDevice, ip.FirstField);
            if (!result) return;

            await _navigator.OpenPrevious();
        }
    }

    private async Task ConnectAsync()
    {
        if (SelectedDevice == null)
            return;
        await _connection.ConnectAsync(SelectedDevice, _events);
    }

    public void SendSetting(string name, float value)
    {
        if (DeviceInfo != null)
        {
            _delayToken?.Cancel();
            _delayToken = new CancellationTokenSource();

            var token = _delayToken.Token;
            Task.Delay(50, token).ContinueWith(async t =>
            {
                if (t.IsCanceled) return;
                
                _connection.Send($"{name}:{value.ToString(System.Globalization.CultureInfo.InvariantCulture)}");
            });
        }
    }

    private async Task ClearAsync()
    {
        await _connection.DisconnectAsync();

        _store.SelectDevice(null);
        Status = null;
        DeviceInfo = null;
    }

    async Task<bool> IHandleBackNavigation.HandleBackAsync()
    {
        await ClearAsync();
        return await Task.FromResult(false);
    }
}
