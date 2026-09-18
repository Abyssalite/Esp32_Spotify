using Custom_Navigation;
using System.Windows.Input;
using CommunityToolkit.Mvvm.Input;
using System.Threading.Tasks;
using Custom_EventHub;
using Custom_Popup;

namespace Esp32_Display_Connect.ViewModels;

public partial class AddDeviceViewModel : ViewModelBase, IHandleBackNavigation
{    
    public string? Address { get; set; }
    public string? Name { get; set; }
    public ICommand? AddDeviceCommand { get; }

    public AddDeviceViewModel(
        Store store,
        INavigatorService navigator,
        IEventHub events,
        IPopupHost popup
    ):base(store, navigator, events, popup)    {
        AddDeviceCommand = new AsyncRelayCommand(AddDevice);
    }

    private async Task ClearAsync()
    {
        Name = string.Empty;
        OnPropertyChanged(nameof(Name));
        Address = string.Empty;
        OnPropertyChanged(nameof(Address));
    }

    private async Task AddDevice()
    {
        string name = Helpers.InputOrDefault(Name, "");
        if (name == "")
            return;

        string address = Helpers.InputOrDefault(Address, "");
        if (!Helpers.IsValidIP(address))
            return;

        var device = new Device()
        {
            Name = name,
            Address = address
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
