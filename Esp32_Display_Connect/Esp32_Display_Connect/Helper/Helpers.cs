using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;

public static class Helpers
{
    public static object? GetPropertyValue(object obj, string propertyName)
    {
        return obj.GetType().GetProperty(propertyName)?.GetValue(obj);
    } 
    
    public static bool IsValidIP(string? ip)
    {
        if (string.IsNullOrWhiteSpace(ip))
            return false;

        return IPAddress.TryParse(ip, out IPAddress? address) 
            && address.AddressFamily == AddressFamily.InterNetwork;
    }
    
    public static string InputOrDefault(string? input, string defaultValue) =>
        string.IsNullOrWhiteSpace(input) ? defaultValue : input;

    public static void Print(object? data)
    {
        Console.WriteLine(JsonSerializer.Serialize(
            data,
            AppJsonContext.Default.ObservableCollectionDevice));
    }

    private static string GetAppDataPath()
    {
        var folder = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        var appFolder = Path.Combine(folder, "Esp32_Display");
        Directory.CreateDirectory(appFolder);
        return appFolder;
    }

    public static async Task SaveAsync(Store store)
    {

        string json = JsonSerializer.Serialize(store.DevicesList, AppJsonContext.Default.ObservableCollectionDevice);

        string path = Path.Combine(GetAppDataPath(), "Devices.json");

        await File.WriteAllTextAsync(path, json);
        Console.WriteLine("Saved to: " + path);
    }

    public static async Task<ObservableCollection<Device>?> LoadAsync()
    {
        string path = Path.Combine(GetAppDataPath(), "Devices.json");

        try
        {
            Console.WriteLine("Loading...");

            if (!File.Exists(path))
            {
                Console.WriteLine("File not found. Creating new list.");
                return null;
            }

            string json = await File.ReadAllTextAsync(path);
            Console.WriteLine("Loaded from: " + path);

            return JsonSerializer.Deserialize(
                       json,
                       AppJsonContext.Default.ObservableCollectionDevice)
                   ?? null;
        }
        catch (Exception ex)
        {
            Console.WriteLine("Error loading file: " + ex.Message);
            return null;
        }
    }
}