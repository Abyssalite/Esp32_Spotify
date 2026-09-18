using Android.App;
using Android.Content.PM;
using Avalonia;
using Avalonia.Android;
using Microsoft.Extensions.DependencyInjection;
using Custom_Navigation;

namespace Esp32_Display_Connect.Android;

[Activity(
    Label = "Esp32_Display_Connect.Android",
    Theme = "@style/MyTheme.NoActionBar",
    Icon = "@drawable/icon",
    MainLauncher = true,
    ConfigurationChanges = ConfigChanges.Orientation | ConfigChanges.ScreenSize | ConfigChanges.UiMode)]
public class MainActivity : AvaloniaMainActivity<App>
{
    protected override AppBuilder CustomizeAppBuilder(AppBuilder builder)
    {
        return base.CustomizeAppBuilder(builder)
            .WithInterFont();
    }

    public override void OnBackPressed()
    {
        var navigatorService = App.Services?.GetRequiredService<INavigatorService>();
        
        if (navigatorService == null) return;
        if (navigatorService.IsExit())
        {        
            Finish();
        }
        else  
        {        
            navigatorService.OpenPrevious();
        }
    }
}
