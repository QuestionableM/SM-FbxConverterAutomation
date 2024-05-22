# SM-FbxConverterAutomation

Allows you to prepare the FBX animation for use in Scrap Mechanic with a single click

# Requirements
### You must have the [FBX Converter 2013](https://aps.autodesk.com/developer/overview/fbx-converter-archives) installed on your computer

# How to use
- Download the latest release of the `FbxConverterAutomation.zip` [here](https://github.com/QuestionableM/SM-FbxConverterAutomation/releases/latest).
- Unzip the archive, open `FbxConverterAutomation/config.xml` and specify the full path to `FbxConverter.exe`, which is located in the `bin` folder of your FBX Converter 2013 installation.<br/>
Here's an example of the `config.xml` file with the path to `FbxConverter.exe`:
```xml
<FbxConverterAutomation
    fbx_converter_path = "FBX Converter/2013.3/bin/FbxConverter.exe">
```
- Launch the program for the first time and make sure there are no errors in the opened console window.
- Put your `.fbx` files into the `fbx` folder.
- Launch the program again.

# Specify a custom FBX and DAE directory path
- Open `FbxConverterAutomation/config.xml` and add `fbx_dir` attribute if you want to change the fbx directory, and `dae_dir` if you want to change the dae directory.<br/>
Here's an example of the `config.xml` file with `fbx_dir` and `dae_dir` attributes defined:
```xml
<FbxConverterAutomation
    fbx_converter_path = "path_to_fbx2013_bin_exe"
    fbx_dir = "optional_fbx_directory_path"
    dae_dir = "optional_dae_directory_path">
```
By default `fbx_dir` is `./fbx` and `dae_dir` is `./dae`.
