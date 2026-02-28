#pragma once

#include "scene/gui/link_button.h"

class VersionButton : public LinkButton {
	GDCLASS(VersionButton, LinkButton);

public:
	enum VersionFormat {
		// 4.3.2.stable
		FORMAT_BASIC,
		// v4.3.2.stable.mono [HASH]
		FORMAT_WITH_BUILD,
		// Name v4.3.2.stable.mono.official [HASH]
		FORMAT_WITH_NAME_AND_BUILD,
	};

private:
	VersionFormat format = FORMAT_WITH_NAME_AND_BUILD;

protected:
	void _notification(int p_what);

	virtual void pressed() override;

public:
	VersionButton(VersionFormat p_format);
};
