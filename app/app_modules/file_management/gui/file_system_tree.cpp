#include "file_system_tree.h"

#include "app/gui/app_control.h"

static const int default_column_count = 5;
static FileSystemTree::ColumnSetting default_column_settings[default_column_count] = {
	{ 0, true, TTRC("Name"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 3 },
	{ 1, true, TTRC("Modified"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 2 },
	{ 2, false, TTRC("Created"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 2 },
	{ 3, true, TTRC("Type"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
	{ 4, true, TTRC("Size"), HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT, true, 1 },
};

void FileSystemTree::_notification(int p_what) {
	// switch (p_what) {
	// 	case NOTIFICATION_DRAW: {
	// 	} break;
	// }
}

void FileSystemTree::_update_display_mode() {
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	set_hide_root(true);
	set_allow_rmb_select(true);
	set_allow_reselect(true);

	uint32_t column_count = 1;
	if (display_mode == DISPLAY_MODE_TREE) {
		column_count = 1;

		set_select_mode(Tree::SELECT_MULTI);

		set_theme_type_variation("Tree");
		set_hide_folding(false);
		set_columns(column_count);
		set_column_titles_visible(false);
	} else if (display_mode == DISPLAY_MODE_LIST) {
		column_count = 4;

		set_select_mode(Tree::SELECT_ROW);

		set_theme_type_variation("TreeTable");
		set_hide_folding(true);
		set_columns(column_count);
		set_column_titles_visible(true);
	}

	ERR_FAIL_COND(column_count >= column_settings.size());

	uint32_t column = 0;
	for (uint32_t i = 0; i < column_settings.size() && column < column_count; i++) {
		const ColumnSetting &setting = column_settings[i];
		if (!setting.visible) {
			continue;
		}

		set_column_title(column, setting.title);
		set_column_title_alignment(column, setting.alignment);
		set_column_clip_content(column, setting.clip);
		if (setting.expand_ratio < 0) {
			set_column_expand(column, false);
		} else {
			set_column_expand(column, true);
			set_column_expand_ratio(column, setting.expand_ratio);
		}
		column++;
	}
}

TreeItem *FileSystemTree::_add_tree_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = p_fi.type == FOLDER_TYPE;
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder")); // TODO
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File")); // TODO

		icon = is_dir ? folder : file;
	}

	TreeItem *item = create_item(p_parent, p_index);
	String path = p_fi.path;
	item->set_text(0, p_fi.name);
	item->set_icon(0, icon);
	item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);

	// TODO
	// if (da->is_link(path)) {
	// 	item->set_icon_overlay(0, get_app_theme_icon(SNAME("LinkOverlay")));
	// 	// TRANSLATORS: This is a tooltip for a file that is a symbolic link to another file.
	// 	item->set_tooltip_text(0, vformat(RTR("Link to: %s"), da->read_link(path)));
	// }

	const int icon_size = get_theme_constant(SNAME("class_icon_size"));
	item->set_icon_max_width(0, icon_size);

	// TODO
	// Color parent_bg_color = p_parent->get_custom_bg_color(0);
	// if (has_custom_color) {
	// 	item->set_custom_bg_color(0, parent_bg_color.darkened(ITEM_BG_DARK_SCALE));
	// } else if (parent_bg_color != Color()) {
	// 	item->set_custom_bg_color(0, parent_bg_color);
	// }

	item->set_selectable(0, true);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = path;
	d["is_dir"] = is_dir;
	item->set_metadata(0, d);

	return item;
}

TreeItem *FileSystemTree::_add_list_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	bool is_dir = p_fi.type == FOLDER_TYPE;
	Ref<Texture2D> icon = p_fi.icon;
	if (!icon.is_valid()) {
		Ref<Texture2D> folder = get_app_theme_icon(SNAME("Folder")); // TODO
		Ref<Texture2D> file = get_app_theme_icon(SNAME("File")); // TODO

		icon = is_dir ? folder : file;
	}

	TreeItem *item = create_item(p_parent, p_index);
	item->set_text(0, p_fi.name);
	item->set_icon(0, icon);
	// item->set_editable(0, true);

	String modified_time = itos(p_fi.modified_time);
	item->set_text(1, modified_time);
	// item->set_selectable(1, true);

	// item->set_cell_mode(2, TreeItem::CELL_MODE_CHECK);
	// item->set_editable(2, true);
	item->set_text(2, p_fi.type);
	// item->set_checked(2, info.is_singleton);

	String size = itos(p_fi.size);
	item->set_text(3, size);
	// item->add_button(3, get_editor_theme_icon(SNAME("Load")), BUTTON_OPEN);
	// item->add_button(3, get_editor_theme_icon(SNAME("MoveUp")), BUTTON_MOVE_UP);
	// item->add_button(3, get_editor_theme_icon(SNAME("MoveDown")), BUTTON_MOVE_DOWN);
	// item->add_button(3, get_editor_theme_icon(SNAME("Remove")), BUTTON_DELETE);
	// item->set_selectable(3, false);

	Dictionary d;
	d["name"] = p_fi.name;
	d["path"] = p_fi.path;
	d["is_dir"] = is_dir;
	item->set_metadata(0, d);

	return item;
}

void FileSystemTree::set_display_mode(DisplayMode p_display_mode) {
	if (display_mode == p_display_mode) {
		return;
	}

	display_mode = p_display_mode;
	_update_display_mode();
}

void FileSystemTree::set_column_settings(const Array &p_column_settings) {
}

Array FileSystemTree::get_column_settings() const {
	return Array();
}

TreeItem *FileSystemTree::add_item(const FileInfo &p_fi, TreeItem *p_parent, int p_index) {
	if (display_mode == DISPLAY_MODE_TREE) {
		return _add_tree_item(p_fi, p_parent, p_index);
	} else if (display_mode == DISPLAY_MODE_LIST) {
		return _add_list_item(p_fi, p_parent, p_index);
	}

	return nullptr;
}

void FileSystemTree::_bind_methods() {
}

FileSystemTree::FileSystemTree() {
	for (int i = 0; i < default_column_count; i++) {
		column_settings[i] = default_column_settings[i];
	}

	callable_mp(this, &FileSystemTree::_update_display_mode).call_deferred();
}

FileSystemTree::~FileSystemTree() {
}
