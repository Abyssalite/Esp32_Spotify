using System.Threading.Tasks;
using Custom_EventHub;
using Custom_Navigation;
using Custom_Popup;
using Microsoft.Extensions.DependencyInjection;

namespace Esp32_Display_Connect.ViewModels;

public partial class MainViewModel : ViewModelBase
{    
    private readonly IViewHost _viewhost;
    public IViewHost ViewHost => _viewhost;
    private readonly IPopupHost _popuphost;
    public IPopupHost Popuphost => _popuphost;
    public MainViewModel(
        Store store,
        IViewHost viewHost,
        INavigatorService navigator,
        IEventHub events,
        IPopupHost popup
    ):base(store, navigator, events, popup)
    {
        _viewhost = viewHost;
        _popuphost = popup;
        _ = InitializeAsync();
    }

    public async Task InitializeAsync()
    {        
        _store.DevicesList = await Helpers.LoadAsync() ?? [];

        var vm = App.Services?.GetRequiredService<SelectViewModel>();
        await _navigator.NavigateMain(vm); 
    }
}
