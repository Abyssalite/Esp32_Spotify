using System.Threading;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Threading;

namespace Esp32_Display_Connect.Views;

public partial class AddDeviceView : UserControl
{
    private CancellationTokenSource? _resizeToken;
    private bool _layoutInitialized = false;

    public AddDeviceView()
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
            DeviceAddBorder.Margin = new Thickness(20, 50, 20, 50);
            DeviceAddTextStack.Margin = new Thickness(0, 0, 0, 20);
            DeviceAddStack.Margin = new Thickness(-30, 103, 30, 102);

            DeviceAddStack.RowDefinitions = new RowDefinitions("*,Auto");
            DeviceAddStack.ColumnDefinitions = new ColumnDefinitions("*");
            Grid.SetRow(DeviceAddButton, 1);
            Grid.SetColumn(DeviceAddButton, 0);
        }

        else if (width > 800)
        {
            DeviceAddBorder.Margin = new Thickness(100, 0, 100, 0);
            DeviceAddTextStack.Margin = new Thickness(0, 0, 50, 0);
            DeviceAddStack.Margin = new Thickness(40, 103, 90, 102);

            DeviceAddStack.RowDefinitions = new RowDefinitions("*");
            DeviceAddStack.ColumnDefinitions = new ColumnDefinitions("*,Auto");
            Grid.SetRow(DeviceAddButton, 0);
            Grid.SetColumn(DeviceAddButton, 1);
        }

        if (!_layoutInitialized)
        {
            DeviceAddBorder.IsVisible = true;
            _layoutInitialized = true;
        }
    }
}