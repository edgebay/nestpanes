#include "app_about.h"

#include "app/app_string_names.h"
#include "app/gui/version_button.h"
#include "app/themes/app_scale.h"
// #include "core/authors.gen.h"
// #include "core/donors.gen.h"
#include "core/license.gen.h"
#include "core/version.h"
#include "scene/gui/item_list.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "scene/resources/style_box.h"

void AppAbout::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			_about_text_label->set_text(String(U"© 2025-2026 Bay.\n"));
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			const Ref<Font> font = get_theme_font(SNAME("source"), AppStringName(AppFonts));
			const int font_size = get_theme_font_size(SNAME("source_size"), AppStringName(AppFonts));

			_tpl_text->begin_bulk_theme_override();
			_tpl_text->add_theme_font_override("normal_font", font);
			_tpl_text->add_theme_font_size_override("normal_font_size", font_size);
			_tpl_text->add_theme_constant_override(SceneStringName(line_separation), 4 * APP_SCALE);
			_tpl_text->end_bulk_theme_override();

			license_text_label->begin_bulk_theme_override();
			license_text_label->add_theme_font_override("normal_font", font);
			license_text_label->add_theme_font_size_override("normal_font_size", font_size);
			license_text_label->add_theme_constant_override(SceneStringName(line_separation), 4 * APP_SCALE);
			license_text_label->end_bulk_theme_override();

			_logo->set_texture(get_theme_icon(SNAME("Logo"), SNAME("AppIcons"))); // TODO: Logo
		} break;
	}
}

void AppAbout::_license_tree_selected() {
	TreeItem *selected = _tpl_tree->get_selected();
	_tpl_text->scroll_to_line(0);
	_tpl_text->set_text(selected->get_metadata(0));
}

AppAbout::AppAbout() {
	set_title(TTRC("About"));
	set_hide_on_ok(true);

	VBoxContainer *vbc = memnew(VBoxContainer);
	add_child(vbc);

	HBoxContainer *hbc = memnew(HBoxContainer);
	hbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hbc->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	hbc->add_theme_constant_override("separation", 30 * APP_SCALE);
	vbc->add_child(hbc);

	_logo = memnew(TextureRect);
	_logo->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	hbc->add_child(_logo);

	VBoxContainer *version_info_vbc = memnew(VBoxContainer);

	// Add a dummy control node for spacing.
	Control *v_spacer = memnew(Control);
	version_info_vbc->add_child(v_spacer);

	version_info_vbc->add_child(memnew(VersionButton(VersionButton::FORMAT_WITH_NAME_AND_BUILD)));

	_about_text_label = memnew(Label);
	_about_text_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	_about_text_label->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	_about_text_label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	version_info_vbc->add_child(_about_text_label);

	hbc->add_child(version_info_vbc);

	TabContainer *tc = memnew(TabContainer);
	// tc->set_tab_alignment(TabBar::ALIGNMENT_CENTER);
	tc->set_custom_minimum_size(Size2(400, 200) * APP_SCALE);
	tc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tc->set_theme_type_variation("TabContainerOdd");
	vbc->add_child(tc);

	// License.

	license_text_label = memnew(RichTextLabel);
	license_text_label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	license_text_label->set_threaded(true);
	license_text_label->set_name(TTRC("License"));
	license_text_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	license_text_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	license_text_label->set_text(String::utf8(LICENSE_TEXT));
	tc->add_child(license_text_label);

	// Thirdparty License.

	VBoxContainer *license_thirdparty = memnew(VBoxContainer);
	license_thirdparty->set_name(TTRC("Third-party Licenses"));
	license_thirdparty->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tc->add_child(license_thirdparty);

	// TODO: modify tpl_string?
	String tpl_string = String(APP_VERSION_NAME) + TTRC(" relies on a number of third-party free and open source libraries, all compatible with the terms of its MIT license. The following is an exhaustive list of all such third-party components with their respective copyright statements and license terms.");
	Label *tpl_label = memnew(Label(tpl_string));
	tpl_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	tpl_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tpl_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	tpl_label->set_size(Size2(630, 1) * APP_SCALE);
	license_thirdparty->add_child(tpl_label);

	HSplitContainer *tpl_hbc = memnew(HSplitContainer);
	tpl_hbc->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tpl_hbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tpl_hbc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tpl_hbc->set_split_offset(240 * APP_SCALE);
	license_thirdparty->add_child(tpl_hbc);

	_tpl_tree = memnew(Tree);
	_tpl_tree->set_hide_root(true);
	TreeItem *root = _tpl_tree->create_item();
	TreeItem *tpl_ti_all = _tpl_tree->create_item(root);
	tpl_ti_all->set_text(0, TTRC("All Components"));
	tpl_ti_all->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_ALWAYS);
	TreeItem *tpl_ti_tp = _tpl_tree->create_item(root);
	tpl_ti_tp->set_text(0, TTRC("Components"));
	tpl_ti_tp->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_ALWAYS);
	tpl_ti_tp->set_selectable(0, false);
	TreeItem *tpl_ti_lc = _tpl_tree->create_item(root);
	tpl_ti_lc->set_text(0, TTRC("Licenses"));
	tpl_ti_lc->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_ALWAYS);
	tpl_ti_lc->set_selectable(0, false);
	String long_text = "";
	for (int component_index = 0; component_index < COPYRIGHT_INFO_COUNT; component_index++) {
		const ComponentCopyright &component = COPYRIGHT_INFO[component_index];
		TreeItem *ti = _tpl_tree->create_item(tpl_ti_tp);
		String component_name = String::utf8(component.name);
		ti->set_text(0, component_name);
		String text = component_name + "\n";
		long_text += "- " + component_name + "\n";
		for (int part_index = 0; part_index < component.part_count; part_index++) {
			const ComponentCopyrightPart &part = component.parts[part_index];
			text += "\n    Files:";
			for (int file_num = 0; file_num < part.file_count; file_num++) {
				text += "\n        " + String::utf8(part.files[file_num]);
			}
			String copyright;
			for (int copyright_index = 0; copyright_index < part.copyright_count; copyright_index++) {
				copyright += String::utf8("\n    \xc2\xa9 ") + String::utf8(part.copyright_statements[copyright_index]);
			}
			text += copyright;
			long_text += copyright;
			String license = "\n    License: " + String::utf8(part.license) + "\n";
			text += license;
			long_text += license + "\n";
		}
		ti->set_metadata(0, text);
	}
	for (int i = 0; i < LICENSE_COUNT; i++) {
		TreeItem *ti = _tpl_tree->create_item(tpl_ti_lc);
		String licensename = String::utf8(LICENSE_NAMES[i]);
		ti->set_text(0, licensename);
		long_text += "- " + licensename + "\n\n";
		String licensebody = String::utf8(LICENSE_BODIES[i]);
		ti->set_metadata(0, licensebody);
		long_text += "    " + licensebody.replace("\n", "\n    ") + "\n\n";
	}
	tpl_ti_all->set_metadata(0, long_text);
	tpl_hbc->add_child(_tpl_tree);

	_tpl_text = memnew(RichTextLabel);
	_tpl_text->set_threaded(true);
	_tpl_text->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_tpl_text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tpl_hbc->add_child(_tpl_text);

	_tpl_tree->connect(SceneStringName(item_selected), callable_mp(this, &AppAbout::_license_tree_selected));
	tpl_ti_all->select(0);
	_tpl_text->set_text(tpl_ti_all->get_metadata(0));
}
