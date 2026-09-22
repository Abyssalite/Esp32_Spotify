using System;
using System.Windows.Input;
using Custom_Popup;

namespace Esp32_Display_Connect.Popup;

public partial class InputPopupViewModel : PopupViewModelBase
{    
    public string? FirstField { get; set; }
    public string? SecondField { get; set; }
    public string? FirstFieldText { get; set; }
    public string? SecondFieldText { get; set; }
    public bool IsSecondField { get; set; } = false;
    public ICommand? AddDeviceCommand { get; }

    private readonly UpdateInput? _input = new();

    public InputPopupViewModel(
        IPopupHost popup,
        string firstField,
        string? secondField = null
    ) : base(popup)
    {
        FirstFieldText = firstField;
        OnPropertyChanged(nameof(FirstFieldText));
        if (secondField != null)
        {
            SecondFieldText = secondField;
            OnPropertyChanged(nameof(SecondFieldText));
            IsSecondField = true;
            OnPropertyChanged(nameof(IsSecondField));
        }
    }


    public override void Save()
    {
        if (_input == null || FirstField == null || (IsSecondField && SecondField == null)) {
            Console.WriteLine("Missing field");
            _popup.Close(null);
        }
        else
        {
            _input.FirstField = FirstField;
            _input.SecondField = SecondField ?? "";
            _popup.Close(_input);
        }
    }
}
