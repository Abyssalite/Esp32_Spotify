using System;

namespace Esp32_Display_Connect.Events;

public sealed record SelectedDeviceChangedEvent(Device? device);

public sealed record StatusReceivedEvent(DeviceStatus deviceStatus);
public sealed record ConnectionStatusChangedEvent(string connectionStatus);

/*public sealed record BluetoothDiscoveredEvent(BluetoothDevice device);
public sealed record BluetoothReceiveEvent(string message);
public sealed record BluetoothDeviceAddEvent();

public sealed record ConfirmPopupOpenEvent(string Title, string Message);*/
