# SM-FbxConverterAutomation

Allows you to prepare the FBX animation for use in Scrap Mechanic with a single click

# Requirements
### You must have the [FBX Converter 2013](https://aps.autodesk.com/developer/overview/fbx-converter-archives) installed on your computer

# How to use
- Install the latest release of the program [here](https://github.com/QuestionableM/SM-FbxConverterAutomation/releases/latest)
- Open `FbxConverterAutomation/config.xml` and specify the path to `FbxConverter.exe` that is located in the `bin` folder of your FBX Converter 2013 installation
Here's an example of the `config.xml` file with the path to `FbxConverter.exe`
```xml
<FbxConverterAutomation
    fbx_converter_path = "path_to_fbx2013_bin_exe">
```
- Launch the program for the first time and make sure there are no errors in the opened console window
- Put your `.fbx` files into the `fbx` folder
- Launch the program again
