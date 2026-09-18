using System.Windows.Input;
using Custom_Popup;

namespace Esp32_Display_Connect.Popup;

public partial class InputPopupViewModel : PopupViewModelBase
{    
    public string? SSID { get; set; }
    public string? Pass { get; set; }
    public ICommand? AddDeviceCommand { get; }

    private readonly WifiInput? _wifi = new();

    public InputPopupViewModel(
        IPopupHost popup
    ) : base(popup)
    {
    }


    public override void Save()
    {
        if (_wifi == null || SSID == null || Pass == null)
                _popup.Close(null);

        else
        {
            _wifi.Ssid = SSID;
            _wifi.Password = Pass;
            _popup.Close(_wifi);
        }
    }
}
