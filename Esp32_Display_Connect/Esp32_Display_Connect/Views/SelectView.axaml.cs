using System.Threading;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Threading;

namespace Esp32_Display_Connect.Views;

public partial class SelectView : UserControl
{
    private CancellationTokenSource? _resizeToken;
    private bool _layoutInitialized = false;

    public SelectView()
    {
        InitializeComponent();

        this.LayoutUpdated += (_, __) =>
        {
            _resizeToken?.Cancel();
            _resizeToken = new CancellationTokenSource();

            var token = _resizeToken.Token;
            Task.Delay(10, token).ContinueWith(t =>
            {
                if (t.IsCanceled) return;
                Dispatcher.UIThread.InvokeAsync(AdjustStackOrientation);
            });
        };
    }

    private void AdjustStackOrientation()
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel == null) return;

        var width = topLevel.Bounds.Width;

        if (width <= 800)
        {
            DeviceSelectStack.Margin = new Thickness(20, 50, 20, 50);
            DeviceSelectBorder.Margin = new Thickness(20, 50, 20, 50);

            DeviceSelectListBox.Margin = new Thickness(0, 0, 0, 8);
            DeviceSelectListBox.Classes.Remove("horizontal");
            DeviceSelectListBox.Classes.Add("vertical");

            DeviceSelectStack.RowDefinitions = new RowDefinitions("*,Auto");
            DeviceSelectStack.ColumnDefinitions = new ColumnDefinitions("*");
            //Grid.SetRow(BluetoothCommandButton, 1);
            //Grid.SetColumn(BluetoothCommandButton, 0);
            Grid.SetRow(AddDeviceCommandButton, 1);
            Grid.SetColumn(AddDeviceCommandButton, 0);
        }

        else if (width > 800)
        {
            DeviceSelectStack.Margin = new Thickness(20, 100, 20, 100);
            DeviceSelectBorder.Margin = new Thickness(100, 0, 100, 0);
            
            DeviceSelectListBox.Margin = new Thickness(0, 0, 10, 0);
            DeviceSelectListBox.Classes.Remove("vertical");
            DeviceSelectListBox.Classes.Add("horizontal");

            DeviceSelectStack.RowDefinitions = new RowDefinitions("*");
            DeviceSelectStack.ColumnDefinitions = new ColumnDefinitions("*,Auto");
            //Grid.SetRow(BluetoothCommandButton, 0);
            //Grid.SetColumn(BluetoothCommandButton, 1);
            Grid.SetRow(AddDeviceCommandButton, 0);
            Grid.SetColumn(AddDeviceCommandButton, 1);
        }

        if (!_layoutInitialized)
        {
            DeviceSelectBorder.IsVisible = true;
            _layoutInitialized = true;
        }
    }
}