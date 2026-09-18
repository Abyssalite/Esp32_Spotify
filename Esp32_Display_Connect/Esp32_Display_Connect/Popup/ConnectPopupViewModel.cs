using System;
using CommunityToolkit.Mvvm.ComponentModel;
using Custom_Popup;

namespace Esp32_Display_Connect.Popup;

public partial class ConnectPopupViewModel : PopupViewModelBase
{
    [ObservableProperty] private string _text;

    public ConnectPopupViewModel(
        string message,
        IPopupHost popup
    ) : base(popup)
    {
        _text = message;
    }

}