#ifndef NAG_EDITOR_UI_H
#define NAG_EDITOR_UI_H

#include "imgui.h"

static constexpr ImVec4 kDangerBtnColor{.6f, .2f, .2f, 1.f};
static constexpr ImVec4 kSuccessBtnColor{.2f, .6f, .2f, 1.f};

static constexpr ImVec4 kDangerTextColor{.9f, .3f, .3f, 1.f};
static constexpr ImVec4 kWarningTextColor{.9f, .9f, .3f, 1.f};

/// Font size for the UI. Adjust as needed for different screen DPIs.
static constexpr float kUIFontSize = 14.f;

/// Duration to show status messages before fading out (in seconds).
static constexpr float kStatusMessageDuration{3.f};

#endif // NAG_EDITOR_UI_H
