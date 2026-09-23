using System;
using System.Collections.Generic;
using CommunityToolkit.Mvvm.ComponentModel;
using Custom_EventHub;
using Custom_Popup;
using Esp32_Display_Connect.Events;

namespace Esp32_Display_Connect.Popup;

public partial class ConnectPopupViewModel : PopupViewModelBase
{
    [ObservableProperty] 
    private string? _logs = "";
    [ObservableProperty] 
    private string? _imageUrl;

    protected readonly IEventHub _events;
    protected readonly List<IDisposable> _subscriptions = new();

    public ConnectPopupViewModel(
        IPopupHost popup,
        IEventHub events
    ) : base(popup)
    {
        _events = events;

        _subscriptions.Add(_events.Subscribe<DeviceLogsChangedEvent>(async evt =>
        {
            Logs += evt.logs;
            ImageUrl = evt.imgUrl;
        }));
    }
}