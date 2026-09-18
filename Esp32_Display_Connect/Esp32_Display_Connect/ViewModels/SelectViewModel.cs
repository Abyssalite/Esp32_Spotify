
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using Custom_Navigation;
using Custom_EventHub;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Extensions.DependencyInjection;
using Custom_Popup;

namespace Esp32_Display_Connect.ViewModels;

public partial class SelectViewModel : ViewModelBase
{    
    public ICommand AddDeviceCommand { get; }
    public ICommand? BluetoothCommand { get; }

    public ObservableCollection<Device>? DevicesList { get; }
    private Device? _selectedDevice;
    public Device? SelectedDevice
    {
        get => _selectedDevice;
        set
        {
            if (value == null || _selectedDevice == value) return;

            _selectedDevice = value;

            _ = OpenDeviceAsync(_selectedDevice);
            _selectedDevice = null;
            OnPropertyChanged(nameof(SelectedDevice));
        }
    }
    
    public SelectViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events,
        IPopupHost popup
    ):base(store, navigator, events, popup)
    {
        DevicesList = _store.DevicesList;
        AddDeviceCommand = new AsyncRelayCommand(addDeviceAsync);
        BluetoothCommand = new AsyncRelayCommand(openBluetooth);

    }

    public async Task openBluetooth()
    {        
        var vm = App.Services?.GetRequiredService<BluetoothViewModel>();
        await _navigator.NavigateMain(vm); 
    }

    async Task addDeviceAsync()
    {
        var vm = App.Services?.GetRequiredService<AddDeviceViewModel>();
        _selectedDevice = null;
        OnPropertyChanged(nameof(SelectedDevice));

        await _navigator.NavigateMain(vm);     
    }

    private async Task OpenDeviceAsync(Device device)
    {
        _store.SelectDevice(device);

        var vm = App.Services?.GetRequiredService<DeviceViewModel>();
        await _navigator.NavigateMain(vm);     
    }
}
