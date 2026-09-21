using Esp32_Display_Connect.ViewModels;
using Microsoft.Extensions.DependencyInjection;

public static class ServiceCollectionExtensions
{
    public static void AddCommonServices(this IServiceCollection collection)
    {
        collection.AddLogging();
        collection.AddSingleton<Store>();
        collection.AddSingleton<IDeviceConnectionService, DeviceConnectionService>();
        //collection.AddSingleton<IBluetoothService, BluetoothService>();
        
        collection.AddTransient<MainViewModel>();
        collection.AddTransient<SelectViewModel>();
        collection.AddTransient<AddDeviceViewModel>();
        collection.AddTransient<DeviceViewModel>();
        collection.AddTransient<BluetoothViewModel>();
    }
}