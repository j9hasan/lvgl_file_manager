#include "lvgl.h"
#include "lvgl_file_manager.h"
#include "sd_card.h"
#include <dirent.h>
#include <string>
#include <algorithm> // for transform
#include <cctype>	 // for tolower
#include <esp_log.h> //for logging

using namespace std;

const char *tag = "file manager";

// function prototypes
void file_manager_init();
static void window_create(string s);
static void create_operation_list();
static void f_manager_init_event(lv_event_t *e);
static void main_list_event_handler(lv_event_t *e);
static void back_event_handler(lv_event_t *e);
static bool process_text_file(string fname);
static bool process_png_file(string fname);
static bool process_jpeg_file(string fname);
static bool list_dir_content(string path);
static bool isFolder(string s);
static string get_file_extension(const string &filename);
static string process_image_path(const string filePath);
static string remove_last_dir(const string &folderPath);

// vars
static string folderPath = "", filePath, clicked_item;
static uint8_t list_click_last_state;

static lv_obj_t *ui_dirList;
static lv_obj_t *currentDirLabel;
static lv_obj_t *txtBuffer;
static lv_obj_t *canvasArea;
static lv_obj_t *operation_list;

string root_path = "/S";

// initial ui with file explorer icon
void ui_init()
{
	lv_obj_t *btn = lv_btn_create(lv_scr_act());
	lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *btnLabel = lv_label_create(btn);
	lv_obj_align(btnLabel, LV_ALIGN_CENTER, 0, 0);
	lv_label_set_text(btnLabel, LV_SYMBOL_DIRECTORY);

	lv_obj_t *appLabel = lv_label_create(lv_scr_act());
	lv_label_set_text(appLabel, "File explorer");

#if LV_FONT_MONTSERRAT_32
	lv_obj_set_size(btn, 45, 40);
	lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_32, 0);
	lv_obj_align(appLabel, LV_ALIGN_CENTER, 0, 35);
#else
	ESP_LOGI(tag, "LV_FONT_MONTSERRAT_36 is not enabled, using default font instead");
	lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_12, 0);
	lv_obj_align(appLabel, LV_ALIGN_CENTER, 0, 25);
#endif

	lv_obj_add_event_cb(btn, f_manager_init_event, LV_EVENT_CLICKED, NULL);
}

static void f_manager_init_event(lv_event_t *e)
{
	ESP_LOGI(tag, "File manager init");
	file_manager_init();
}

void file_manager_init()
{
	// list directory content of root
	list_dir_content(root_path);

	// current dir label
	currentDirLabel = lv_label_create(lv_scr_act());
	lv_label_set_text(currentDirLabel, root_path.c_str());
	lv_obj_align(currentDirLabel, LV_ALIGN_TOP_LEFT, 10, 10);

	// back button create
	lv_obj_t *btn = lv_btn_create(lv_scr_act());
	lv_obj_t *label = lv_label_create(btn);
	lv_label_set_text(label, LV_SYMBOL_PREV);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

	lv_obj_add_event_cb(btn, back_event_handler, LV_EVENT_CLICKED, NULL);
}

static bool list_dir_content(string path)
{
	if (lv_obj_is_valid(ui_dirList))
	{
		lv_obj_del(ui_dirList);
	}

	ESP_LOGI(tag, "creating dir list at %s", path.c_str());

	ui_dirList = lv_list_create(lv_scr_act());
	lv_obj_set_size(ui_dirList, 200, 250);
	lv_obj_center(ui_dirList);

	lv_list_add_text(ui_dirList, "File");

	DIR *dir = opendir(path.c_str());
	struct dirent *entry;
	if (dir == NULL)
	{
		ESP_LOGI(tag, "Dir is NULL");

		return 0;
	}

	int filecount = 0;
	int maxFileLimit = 70;

	// Read filenames and make list
	while ((entry = readdir(dir)) != NULL)
	{
		string dirContent = (entry->d_name);
		string dirType = (entry->d_name);

		// if find any '.' confirm its a file not folder
		lv_obj_t *btn;
		if (isFolder(dirContent))
		{
			btn = lv_list_add_btn(ui_dirList, LV_SYMBOL_DIRECTORY, dirContent.c_str());
		}
		else
		{
			btn = lv_list_add_btn(ui_dirList, LV_SYMBOL_FILE, dirContent.c_str());
		}
		lv_obj_add_event_cb(btn, main_list_event_handler, LV_EVENT_ALL, NULL);

		// cant handle so any files,so limit
		filecount++;
		if (filecount > maxFileLimit)
		{
			// notify that , more file exists
			break;
		}
	}
	closedir(dir);
	return 1;
}

static void main_list_event_handler(lv_event_t *e)
{

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

	// process file, folder strings
	if (code == (LV_EVENT_CLICKED | LV_EVENT_LONG_PRESSED))
	{
		clicked_item = (lv_list_get_btn_text(ui_dirList, obj));
		clicked_item = "/" + clicked_item;

		if (isFolder(clicked_item))
			folderPath += clicked_item;

		else
			filePath = root_path + folderPath + clicked_item;

		ESP_LOGI(tag, "procesed filePath: %s, folderPath: %s", filePath.c_str(), folderPath.c_str());
	}

	// remember last clicked/long pressed
	if (code == LV_EVENT_PRESSED)
	{
		list_click_last_state = code;
	}
	if (code == LV_EVENT_LONG_PRESSED)
	{
		list_click_last_state = code;
	}

	// handle click and long press, long press creates operation menu
	if (code == LV_EVENT_CLICKED)
	{

		if (list_click_last_state == LV_EVENT_PRESSED)
		{
			ESP_LOGI(tag, "LV_EVENT_CLICKED");
			string ext = get_file_extension(clicked_item);
			if (ext == "x")
			{
				list_dir_content((root_path + folderPath));
				lv_label_set_text(currentDirLabel, (root_path + folderPath).c_str());
			}
			if (!isFolder(clicked_item))
			{
				if (ext == "TXT")
				{
					process_text_file(filePath);
				}

				else if (ext == "PNG")
				{
					process_png_file(filePath);
				}

				else if (ext == "JPG")
				{
					process_jpeg_file(filePath);
				}

				else
				{
					ESP_LOGE(tag, "Unsupported file: %s", ext.c_str());
					window_create("error!");
					lv_label_set_text(txtBuffer, "Unsupported file format");
				}
			}
		}

		else if (list_click_last_state == LV_EVENT_LONG_PRESSED)
			create_operation_list();
	}
}

static void window_create(string s)
{
	lv_obj_t *msgbox = lv_msgbox_create(lv_scr_act());
	lv_obj_set_size(msgbox, 220, 200);
	lv_obj_clear_flag(msgbox, LV_OBJ_FLAG_SCROLLABLE);
	lv_msgbox_add_title(msgbox, s.c_str());
	lv_msgbox_add_close_button(msgbox);

	canvasArea = lv_obj_create(msgbox);
	lv_obj_set_size(canvasArea, 220, 175);
	lv_obj_add_flag(canvasArea, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(canvasArea, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(canvasArea, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_align(canvasArea, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_add_flag(canvasArea, LV_OBJ_FLAG_SCROLLABLE);
	// lv_label_set_long_mode(canvasArea, LV_LABEL_LONG_WRAP);

	txtBuffer = lv_label_create(canvasArea);
	lv_label_set_text(txtBuffer, "");

	static lv_style_t shadowBoxStyle;
	lv_style_init(&shadowBoxStyle);

	/*Set a background color and a radius*/
	lv_style_set_bg_opa(&shadowBoxStyle, LV_OPA_COVER);
	// lv_style_set_bg_color(&shadowBoxStyle, lv_palette_lighten(LV_PALETTE_GREY, 1));
	lv_style_set_radius(&shadowBoxStyle, 0);
	lv_style_set_border_width(&shadowBoxStyle, 0);
	lv_style_set_shadow_color(&shadowBoxStyle, lv_color_hex(0x655151));
	lv_style_set_shadow_opa(&shadowBoxStyle, 150);
	lv_style_set_shadow_width(&shadowBoxStyle, 30);
	lv_style_set_shadow_spread(&shadowBoxStyle, 500);
	lv_obj_add_style(msgbox, &shadowBoxStyle, 0);
	return;
}

static bool process_text_file(string fname)
{
	window_create("text viewer");

	FILE *file;
	string fullText;
	char buffer[100]; // Buffer to store each line
	file = fopen(fname.c_str(), "r");

	if (file == NULL)
	{
		perror("Error opening file");
		return 1;
	}

	// Read and print each line
	while (fgets(buffer, sizeof(buffer), file) != NULL)
	{
		ESP_LOGI(tag, "%s", buffer);
		fullText += buffer;
	}

	lv_label_set_text(txtBuffer, fullText.c_str());

	fclose(file); // Close the file
	return 0;
}

static bool process_png_file(string fname)
{
	// set LV_USE_FS_STDIO 'S' in lv_conf.h

	ESP_LOGI(tag, "%s", fname.c_str());

	window_create("png decoder");

	lv_obj_t *img = lv_image_create(canvasArea);

	string path = process_image_path(fname);

	lv_image_set_src(img, path.c_str());

	lv_obj_align(img, LV_ALIGN_CENTER, 0, -15);

	return 1;
}

static bool process_jpeg_file(string fname)
{
	if (process_text_file(fname))
		return 1;
	else
		return 0;
}

static void back_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED)
	{
		folderPath = remove_last_dir(folderPath);
		list_dir_content(root_path + folderPath);
		lv_label_set_text(currentDirLabel, (root_path + folderPath).c_str());
	}
}

static bool isFolder(string s)
{
	for (int i = s.length(); i >= 0; i--)
	{
		if (s[i] == '.')
		{
			return false;
		}
	}
	return true;
}

static string get_file_extension(const string &filename)
{
	size_t dotPos = filename.find_last_of('.');
	if (dotPos != string::npos && dotPos != filename.size() - 1)
	{
		return filename.substr(dotPos + 1);
	}
	return "x";
}

// Function to remove the last directory from the folder path
static string remove_last_dir(const string &folderPath)
{
	// Find the position of the last '/' character
	size_t pos = folderPath.find_last_of('/');

	// If '/' is not found or it's at the beginning, return an empty string
	if (pos == string::npos)
	{
		ESP_LOGE(tag, "remLastDir: no / foound ");
		return "";
	}

	return folderPath.substr(0, pos);
}

static string process_image_path(string filePath)
{
	// Remove the first '/'
	filePath.erase(0, 1);

	// Find the position of the second '/'
	size_t secondSlashPos = filePath.find('/', filePath.find('/'));

	// If the second '/' is not found, return the original string
	if (secondSlashPos == std::string::npos)
	{
		return filePath;
	}

	// Replace the second '/' with ':'
	filePath.replace(secondSlashPos, 1, ":");

	// Find the position of the dot (.) character representing the file extension
	size_t dotPos = filePath.find_last_of('.');

	// If the dot is found and it's not the last character
	if (dotPos != std::string::npos && dotPos != filePath.length() - 1)
	{
		// Convert the extension to lowercase
		std::transform(filePath.begin() + dotPos, filePath.end(), filePath.begin() + dotPos, ::tolower);
	}
	ESP_LOGI(tag, "%s", filePath.c_str());
	return filePath;
}

static void operation_list_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
	if (code == LV_EVENT_CLICKED)
	{
		LV_UNUSED(obj);

		string clicked_item(lv_list_get_btn_text(ui_dirList, obj));

		if (clicked_item == "Close" && lv_obj_is_valid(operation_list))
		{
			lv_obj_del(operation_list);
		}

		else if (clicked_item == "Apply" && lv_obj_is_valid(operation_list))
		{
			lv_obj_del(operation_list);
		}

		else if (clicked_item == "Delete" && lv_obj_is_valid(operation_list))
		{
			if (remove(filePath.c_str()) == 0)
				list_dir_content(root_path + folderPath);
			else
				ESP_LOGW(tag, "failed to delete");
			lv_obj_del(operation_list);
		}

		else if (clicked_item == "Open" && lv_obj_is_valid(operation_list))
		{
			if (isFolder(clicked_item))
			{
				lv_obj_del(operation_list);
				list_dir_content(root_path + folderPath);
			}

			// else
			// 	filePath = root_path + folderPath + clicked_item;
			// lv_event_send(list)
		}
	}
}
static void create_operation_list()
{
	/*Create a list*/
	operation_list = lv_list_create(lv_screen_active());
	lv_obj_set_size(operation_list, lv_pct(50), lv_pct(60));
	lv_obj_align(operation_list, LV_ALIGN_TOP_RIGHT, -10, 10);

	/*Add buttons to the list*/
	lv_obj_t *btn;
	// lv_list_add_text(operation_list, "File");
	// btn = lv_list_add_button(operation_list, LV_SYMBOL_FILE, "New");
	// lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_button(operation_list, LV_SYMBOL_DIRECTORY, "Open");
	lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_button(operation_list, LV_SYMBOL_SAVE, "Save");
	lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_button(operation_list, LV_SYMBOL_CLOSE, "Delete");
	lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_button(operation_list, LV_SYMBOL_EDIT, "Edit");
	lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);

	// lv_list_add_text(operation_list, "Connectivity");
	// btn = lv_list_add_button(operation_list, LV_SYMBOL_BLUETOOTH, "Bluetooth");
	// lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	// btn = lv_list_add_button(operation_list, LV_SYMBOL_GPS, "Navigation");
	// lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	// btn = lv_list_add_button(operation_list, LV_SYMBOL_USB, "USB");
	// lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	// btn = lv_list_add_button(operation_list, LV_SYMBOL_BATTERY_FULL, "Battery");
	// lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);

	lv_list_add_text(operation_list, "Exit");
	btn = lv_list_add_button(operation_list, LV_SYMBOL_OK, "Apply");
	lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
	btn = lv_list_add_button(operation_list, LV_SYMBOL_CLOSE, "Close");
	lv_obj_add_event_cb(btn, operation_list_event_handler, LV_EVENT_CLICKED, NULL);
}

// static void close_event_handler(lv_event_t *e)
// {
// 	lv_event_code_t code = lv_event_get_code(e);
// 	lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

// 	if (code == LV_EVENT_CLICKED)
// 	{
// 		lv_obj_del(obj);
// 	}
// }