#pragma once

#include "macro/singleton_macro.h"
#include "enum/font_icon_enums.h"

// Font icons
#include "icon/FontAwesome6.h"
#include "icon/FontAwesome6Brands.h"
#include "icon/FontAwesome5.h"
#include "icon/FontAwesome5Brands.h"
#include "icon/FontAwesome4.h"
#include "icon/CodIcons.h"
#include "icon/FontAudio.h"
#include "icon/ForkAwesome.h"
#include "icon/Kenney.h"
#include "icon/Lucide.h"
#include "icon/MaterialDesign.h"
#include "icon/MaterialDesignIcons.h"
#include "icon/MaterialSymbols.h"

// Standard library
#include <unordered_map>

// Dependencies
#include <imgui.h>

// Forward declarations
class ImFont;


class FontManager {
    DECLARE_SINGLETON(FontManager)

public:
    void SetFontIcon(FontIcon fontIcon);
    void SetDefaultFontIcon();

#ifdef SHOW_FONT_ICONS
    void Render(bool* openWindow = nullptr);
#endif

private:
    std::unordered_map<FontIcon, ImFont*> m_FontIcons;
    ImFontConfig m_DefaultFontConfig;

    void init();

    void loadFontAwesome6S();
    void loadFontAwesome6R();
    void loadFontAwesome6B();
    void loadFontAwesome5S();
    void loadFontAwesome5R();
    void loadFontAwesome5B();
    void loadFontAwesome4();
    void loadCodIcons();
    void loadFontAudio();
    void loadForkAwesome();
    void loadKenney();
    void loadLucide();
    void loadMaterialDesign();
    // void loadMaterialDesignIcons();
    void loadMaterialSymbolsO();
    // void loadMaterialSymbolsR();
    // void loadMaterialSymbolsS();

#ifdef SHOW_FONT_ICONS
    void drawFontAwesome6S(bool* openWindow);
    void drawFontAwesome6R(bool* openWindow);
    void drawFontAwesome6B(bool* openWindow);
    void drawFontAwesome5S(bool* openWindow);
    void drawFontAwesome5R(bool* openWindow);
    void drawFontAwesome5B(bool* openWindow);
    void drawFontAwesome4(bool* openWindow);
    void drawCodIcons(bool* openWindow);
    void drawFontAudio(bool* openWindow);
    void drawForkAwesome(bool* openWindow);
    void drawKenney(bool* openWindow);
    void drawLucide(bool* openWindow);
    void drawMaterialDesign(bool* openWindow);
    // void drawMaterialDesignIcons(bool* openWindow);
    void drawMaterialSymbolsO(bool* openWindow);
    // void drawMaterialSymbolsR(bool* openWindow);
    // void drawMaterialSymbolsS(bool* openWindow);
#endif
};