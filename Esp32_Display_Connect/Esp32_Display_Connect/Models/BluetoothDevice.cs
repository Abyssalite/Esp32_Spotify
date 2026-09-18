public sealed class BluetoothDevice
{
    public string? Id { get; init; }

    public string? Address { get; init; }

    public string? Name { get; init; }

    public string[]? Uuids { get; init; }

    public short? Rssi { get; init; }

    // Linux.Bluetooth object path.
    public string? ObjectPath { get; init; }

    public override string ToString()
        => $"{Name ?? "<unknown>"} ({Address})";
}