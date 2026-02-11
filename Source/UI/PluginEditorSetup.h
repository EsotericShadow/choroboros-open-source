#pragma once

#include "../Plugin/PluginEditor.h"

class PluginEditorSetup
{
public:
    static void setupSliders(ChoroborosPluginEditor& editor);
    static void setupValueLabels(ChoroborosPluginEditor& editor);
    static void setupLabels(ChoroborosPluginEditor& editor);
    static void setupHQButton(ChoroborosPluginEditor& editor);
};
