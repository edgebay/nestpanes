#pragma once

#include "core/string/ustring.h"
#include "core/templates/vector.h"

Vector<String> get_locales();
void load_translations(const String &p_locale);
