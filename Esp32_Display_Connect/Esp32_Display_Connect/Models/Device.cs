
using System;

public class Device
{
    public required string Address { get; set; }
    public string? Status { get; set; } = "";
    public required string Name { get; set; }
    public Guid ID = Guid.NewGuid();
    public BluetoothDevice? bluetooth { get; set; } = new();

    public Device()
    {
    } 
}
