#include <filesystem>
#include <string>
#include <chrono>
#include <vector>

#include <pugixml/pugixml.hpp>
#include <Windows.h>

#define CON_LOG_PREFIX "[SMDAE] "
#include "Console.hpp"

constexpr const char* g_daeDirKey = "dae_dir";
constexpr const char* g_fbxDirKey = "fbx_dir";

static std::string g_fbxConverterPath;
static std::string g_fbxDirectory;
static std::string g_daeDirectory;

enum ConvertProgress : std::uint8_t
{
	CPROG_Converting,
	CPROG_RemovingArtifacts,
	CPROG_Failed,
	CPROG_Finished
};

inline static void ClearConsoleLine()
{
	DWORD v_prev_mode = 0;
	if (!GetConsoleMode(Console::Handle, &v_prev_mode))
		return;

	if (!SetConsoleMode(Console::Handle, v_prev_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
		return;

	DebugOutNP("\33[1A\33[2K");
	SetConsoleMode(Console::Handle, v_prev_mode);
}

inline static void WriteProgressLine(const std::string& dae_path, ConvertProgress prog)
{
	if (prog != CPROG_Converting)
		ClearConsoleLine();

	DebugOut(0b1101_fg, dae_path, 0b1110_fg, " -> ");

	switch (prog)
	{
	case CPROG_Converting:
		DebugOutLNP(0b0111_fg, "Converting");
		break;
	case CPROG_RemovingArtifacts:
		DebugOutLNP(0b1011_fg, "Removing DAE Artifacts");
		break;
	case CPROG_Failed:
		DebugOutLNP(0b1001_fg, "FAILED");
		break;
	case CPROG_Finished:
		DebugOutLNP(0b0101_fg, "Finished");
		break;
	}
}

inline static bool createDirectorySafe(const std::string& path)
{
	std::error_code v_ec;
	const bool v_success = std::filesystem::create_directory(path, v_ec);
	
	return !v_ec && v_success;
}

inline static bool isDirectorySafe(const std::string& path)
{
	std::error_code v_ec;
	const bool v_is_directory = std::filesystem::is_directory(path);

	return !v_ec && v_is_directory;
}

inline static bool isRegularFileSafe(const std::string& path)
{
	std::error_code v_ec;
	const bool v_reg_file = std::filesystem::is_regular_file(path, v_ec);

	return !v_ec && v_reg_file;
}

inline static bool fileExistsSafe(const std::string& path)
{
	std::error_code v_ec;
	const bool v_exists = std::filesystem::exists(path, v_ec);

	return !v_ec && v_exists;
}

inline static bool regularFileExistsSafe(const std::string& path)
{
	return fileExistsSafe(path) && isRegularFileSafe(path);
}

inline static std::string str_to_lower(const std::string& path)
{
	std::string v_output = path;
	for (char& v_out_char : v_output)
		v_out_char = std::tolower(v_out_char);

	return v_output;
}

inline static bool isAnExecutable(const std::string& path)
{
	const std::filesystem::path v_fs_path(path);
	return v_fs_path.has_extension() && str_to_lower(v_fs_path.extension().string()) == ".exe";
}

inline static void focusOnConsole()
{
	HWND v_con_wnd = GetConsoleWindow();

	SetForegroundWindow(v_con_wnd);
	SetActiveWindow(v_con_wnd);
}

inline static void holdUntilInput()
{
	int _ = getchar();
}

inline static void focusAndHoldUntilInput()
{
	DebugOutLNP("Press any key to close the program");
	focusOnConsole();
	holdUntilInput();
}

inline static void removeDAEArtifacts(const std::string& source_file)
{
	pugi::xml_document v_doc;
	const pugi::xml_parse_result v_result = v_doc.load_file(source_file.c_str());
	if (v_result.status != pugi::status_ok)
	{
		WriteProgressLine(source_file, CPROG_Failed);
		DebugOut("XML Error: ", v_result.description());
		return;
	}

	pugi::xml_node v_root_node = v_doc.root();
	pugi::xml_node v_collada_node = v_root_node.child("COLLADA");
	pugi::xml_node v_library_visual_scene = v_collada_node.child("library_visual_scenes");
	pugi::xml_node v_visual_scene = v_library_visual_scene.child("visual_scene");
	pugi::xml_node v_main_node = v_visual_scene.child("node");

	std::vector<pugi::xml_node> v_node_swap = {};
	for (pugi::xml_node v_cur_node : v_main_node.children())
		if (std::strcmp(v_cur_node.name(), "node") == 0)
			v_node_swap.push_back(v_cur_node);

	std::reverse(v_node_swap.begin(), v_node_swap.end());
	for (pugi::xml_node v_cur_node : v_node_swap)
		v_visual_scene.prepend_copy(v_cur_node);

	v_visual_scene.remove_child(v_main_node);

	v_doc.save_file(source_file.c_str(), " ", pugi::format_no_empty_element_tags | pugi::format_indent, pugi::encoding_utf8);

	WriteProgressLine(source_file, CPROG_Finished);
}

inline static bool convertFBXToDAE(const std::string& fbx_path, std::string& out_dae_path)
{
	namespace fs = std::filesystem;

	const std::string v_fbx_name = fs::path(fbx_path).stem().string();
	out_dae_path = g_daeDirectory + "/" + v_fbx_name + ".dae";

	WriteProgressLine(out_dae_path, CPROG_Converting);
	std::string v_command = std::format("FbxConverter \"{}\" \"{}\" /sffFBX /dffDAE", fbx_path, out_dae_path);

	STARTUPINFOA v_startup_info;
	ZeroMemory(&v_startup_info, sizeof(v_startup_info));
	v_startup_info.cb = sizeof(v_startup_info);

	PROCESS_INFORMATION v_proc_info;

	if (::CreateProcessA(
		g_fbxConverterPath.c_str(),
		v_command.data(),
		NULL,
		NULL,
		FALSE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&v_startup_info,
		&v_proc_info
		))
	{
		WaitForSingleObject(v_proc_info.hProcess, INFINITE);

		DWORD v_exit_code;
		GetExitCodeProcess(v_proc_info.hProcess, &v_exit_code);

		CloseHandle(v_proc_info.hThread);
		CloseHandle(v_proc_info.hProcess);

		if (v_exit_code == 0)
		{
			WriteProgressLine(out_dae_path, CPROG_RemovingArtifacts);
			return true;
		}
	}

	WriteProgressLine(out_dae_path, CPROG_Failed);
	return false;
}

inline static std::vector<std::string> getFbxFiles()
{
	namespace fs = std::filesystem;

	std::vector<std::string> v_output;

	fs::directory_iterator v_dir_iter(g_fbxDirectory, fs::directory_options::skip_permission_denied);

	for (const auto& v_item : v_dir_iter)
	{
		if (!v_item.is_regular_file() || !v_item.path().has_extension())
			continue;

		if (v_item.path().extension() != ".fbx")
			continue;

		v_output.push_back(v_item.path().string());
	}

	return v_output;
}

inline static bool loadPathStr(pugi::xml_node config, const char* attr, const char* default_val, std::string* str_ref)
{
	*str_ref = config.attribute(attr).as_string(default_val);
	return true;
}

inline static bool readConfig()
{
	pugi::xml_document v_doc;
	pugi::xml_parse_result v_res = v_doc.load_file("./config.xml");
	if (v_res != pugi::status_ok)
	{
		DebugErrorL("Couldn't read the config. Error: ", v_res.description());
		return false;
	}

	pugi::xml_node v_config_node = v_doc.root().child("FbxConverterAutomation");
	g_fbxConverterPath = v_config_node.attribute("fbx_converter_path").as_string();

	if (!loadPathStr(v_config_node, g_fbxDirKey, "./fbx", &g_fbxDirectory))
		return false;

	if (!loadPathStr(v_config_node, g_daeDirKey, "./dae", &g_daeDirectory))
		return false;

	if (g_fbxConverterPath.empty())
	{
		DebugErrorL("Failed to read fbx_converter_path");
		return false;
	}

	if (!regularFileExistsSafe(g_fbxConverterPath))
	{
		DebugErrorL("The specified fbx_converter_path doesn't exist or is not pointing to a regular file (Path: ", g_fbxConverterPath, ")");
		return false;
	}

	if (!isAnExecutable(g_fbxConverterPath))
	{
		DebugOutL("The specified fbx_converter_path does not lead to an executable (Path: ", g_fbxConverterPath, ")");
		return false;
	}

	DebugOutL("Valid fbx converter path: ", g_fbxConverterPath);
	return true;
}

inline static bool checkDirSafe(const char* key, const std::string& str)
{
	if (!isDirectorySafe(str))
	{
		DebugErrorL("The specified ", key, " does not lead to a directory! (Path: ", str, ")");
		return false;
	}

	return true;
}

inline static bool checkMainDirectories()
{
	createDirectorySafe(g_fbxDirectory);
	createDirectorySafe(g_daeDirectory);

	return checkDirSafe(g_fbxDirKey, g_fbxDirectory)
		&& checkDirSafe(g_daeDirKey, g_daeDirectory);
}

int main()
{
	namespace chr = std::chrono;
	auto v_before = chr::high_resolution_clock::now();
	
	AttachToDebugConsole();

	if (!readConfig() || !checkMainDirectories())
	{
		focusAndHoldUntilInput();
		return -1;
	}

	const std::vector<std::string> v_fbx_paths = getFbxFiles();
	std::string v_tmp_dae_path;
	for (const std::string& v_cur_fbx_file : v_fbx_paths)
	{
		if (!convertFBXToDAE(v_cur_fbx_file, v_tmp_dae_path))
			continue;

		removeDAEArtifacts(v_tmp_dae_path);
	}

	const auto v_duration = chr::duration_cast<chr::milliseconds>(chr::high_resolution_clock::now() - v_before).count();
	DebugOutL("Finished in ", v_duration, "ms");

	focusAndHoldUntilInput();
	return 0;
}