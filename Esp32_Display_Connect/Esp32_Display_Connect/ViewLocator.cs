using System;
using Avalonia.Controls;
using Avalonia.Controls.Templates;
using Esp32_Display_Connect.ViewModels;
using Esp32_Display_Connect.Popup;

namespace Esp32_Display_Connect;

public class ViewLocator : IDataTemplate
{
    public Control? Build(object? param)
    {
        if (param is null)
            return null;

        var name = param.GetType().FullName!.Replace("ViewModel", "View", StringComparison.Ordinal);
        var type = Type.GetType(name);

        if (type != null)
        {
            return (Control)Activator.CreateInstance(type)!;
        }

        return new TextBlock { Text = "Not Found: " + name };
    }

    public bool Match(object? data)
    {
        return data is ViewModelBase || data is PopupViewModelBase;
    }
}