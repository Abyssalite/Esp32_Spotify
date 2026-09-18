using Custom_Navigation;
using System.Windows.Input;
using CommunityToolkit.Mvvm.Input;
using System.Threading.Tasks;
using Custom_EventHub;
using System;
using Esp32_Display_Connect.Events;
using System.Collections.ObjectModel;
using System.Collections.Generic;
using Custom_Popup;
using Esp32_Display_Connect.Popup;
using System.Text.Json;

namespace Esp32_Display_Connect.ViewModels;

public partial class BluetoothViewModel : ViewModelBase, IHandleBackNavigation
{    
    private BluetoothDevice? _selectedBtDevice;
    public BluetoothDevice? SelectedBtDevice
    {
        get => _selectedBtDevice;
        set
        {
            if (value == null || _selectedBtDevice == value) return;

            _selectedBtDevice = value;

            _ = SelectDeviceAsync(_selectedBtDevice);
            _selectedBtDevice = null;
            OnPropertyChanged(nameof(SelectedBtDevice));
        }
    }
    public ObservableCollection<BluetoothDevice>? BtDevicesList { get; } = [];

    public ICommand? RescanCommand { get; }

    private readonly IBluetoothService _bluetooth;

    public BluetoothViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events,
        IBluetoothService bluetooth,
        IPopupHost popup
    ):base(store, navigator, events, popup)
    {             
        _bluetooth = bluetooth;
  
        _subscriptions.Add(_events.Subscribe<BluetoothDiscoveredEvent>(async evt =>
        {
            BtDevicesList.Add(evt.device);
        }));
        _subscriptions.Add(_events.Subscribe<BluetoothReceiveEvent>(async evt =>
        {
            Console.WriteLine($"Receive: {evt.message}");
        }));

        RescanCommand = new AsyncRelayCommand(ScanAsync);

        _ = LoadKnownAsync();
    }

    async Task LoadKnownAsync()
    {
        if (BtDevicesList == null) return;

        IReadOnlyList<BluetoothDevice> known = await _bluetooth.GetKnownDeviceAsync();
        foreach (var device in known)
            BtDevicesList.Add(device);
    }

    async Task ScanAsync()
    {
        _ = await _bluetooth.ScanAsync(_events, 30);
    }

    private async Task ClearAsync()
    {
        _selectedBtDevice = null;
        OnPropertyChanged(nameof(SelectedBtDevice));

        _popup.Close();
    }

    private async Task SelectDeviceAsync(BluetoothDevice device)
    {
        BluetoothDevice connectedDevice = new();
        string lanAddress = "";
        try
        {            
            var notify = new ConnectPopupViewModel("Connecting...", _popup);
            _ = _popup.ShowNotifyPopup(notify);
            connectedDevice = await _bluetooth.ConnectAsync(device);
        } 
        catch
        {
            Console.WriteLine("Can't connect device.");
            await _navigator.OpenPrevious();
        }

        await _bluetooth.StartReceiveAsync(_events);  
        _popup.Close();
        var input = new InputPopupViewModel(_popup);
        var tmp = await _popup.ShowInputPopup(input);
        if (tmp is WifiInput wifi)
        {
            var json = JsonSerializer.Serialize(wifi);

            await _bluetooth.SendAsync(json);
        }
        await _bluetooth.SendAsync("IP");

        //await AddDevice(lanAddress, connectedDevice);
    }

    private async Task AddDevice(object? input, BluetoothDevice btDevice)
    {
        string name = Helpers.InputOrDefault(btDevice.Name, "");
        if (name == "")
            return;

        string address = Helpers.InputOrDefault(btDevice.Address, "");
        if (!Helpers.IsValidIP(address))
            return;

        var device = new Device()
        {
            Name = name,
            Address = address,
            bluetooth = btDevice
        };

        var result = await _store.StoreAddDevice(device);
        if (result) return;
        
        await _navigator.OpenPrevious();
    }

    async Task<bool> IHandleBackNavigation.HandleBackAsync()
    {
        await ClearAsync();
        return await Task.FromResult(false);
    }
}
