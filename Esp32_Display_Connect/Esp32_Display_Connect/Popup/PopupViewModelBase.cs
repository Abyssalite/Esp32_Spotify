using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Custom_Popup;

namespace Esp32_Display_Connect.Popup;

public partial class PopupViewModelBase : ObservableObject
{
    public ICommand? SaveCommand { get; }
    public ICommand? CloseCommand { get; }
    protected readonly IPopupHost _popup;

    public PopupViewModelBase(IPopupHost popup)
    {
        _popup = popup;
        SaveCommand = new RelayCommand(Save);
        CloseCommand = new RelayCommand(Close);
    }

    public virtual void Close()
    {
        _popup.Close();
    }
    public virtual void Save()
    {
        _popup.Close();
    }
}