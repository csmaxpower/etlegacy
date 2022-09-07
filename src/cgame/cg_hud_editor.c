/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cg_hud_editor.c
 * @brief Sets up and draws in-game HUD editor
 *
 */

#include "cg_local.h"

#define SOUNDEVENT(sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND)

#define SOUND_SELECT    SOUNDEVENT(cgs.media.sndLimboSelect)
#define SOUND_FILTER    SOUNDEVENT(cgs.media.sndLimboFilter)

// grouping hud editing fields
#define INPUT_WIDTH 50
#define INPUT_COLOR_WIDTH 35
#define INPUT_HEIGHT 16
#define CHECKBOX_SIZE 16
#define SLIDERS_WIDTH 135
#define SLIDERS_HEIGHT 16
#define BUTTON_WIDTH 45
#define BUTTON_HEIGHT 16

#define HUDEDITOR_CONTROLS_SPACER_XY 4
#define HUDEDITOR_TITLE_SPACER_Y (BUTTON_HEIGHT + 8)
#define HUDEDITOR_CATEGORY_SPACER_Y 8

#define HUDEDITOR_SELECTHUD_Y 6
#define HUDEDITOR_SIZEPOS_Y (HUDEDITOR_SELECTHUD_Y + BUTTON_HEIGHT + HUDEDITOR_TITLE_SPACER_Y + (BUTTON_HEIGHT * 2) + \
	                         HUDEDITOR_CONTROLS_SPACER_XY + HUDEDITOR_CATEGORY_SPACER_Y)
#define HUDEDITOR_COLORSSTYLE_Y (HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CATEGORY_SPACER_Y + \
	                             (INPUT_HEIGHT * 2) + HUDEDITOR_CONTROLS_SPACER_XY)

float    HUDEditorX;
float    HUDEditorWidth;
float    HUDEditorCenterX;
qboolean wsAdjusted = qfalse;

static panel_button_t *lastFocusComponent;
static qboolean       lastFocusComponentMoved;
static int            elementColorSelection;
static qboolean showAllLayout = qfalse;

static void CG_HudEditorUpdateFields(panel_button_t *button);
static qboolean CG_HudEditor_Dropdown_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditor_HudDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_HudRenderDropdown(panel_button_t *button);
static qboolean CG_HudEditor_StyleTextDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_StyleTextRenderDropdown(panel_button_t *button);
static qboolean CG_HudEditor_AlignTextDropdown_KeyUp(panel_button_t *button, int key);
static void CG_HudEditor_AlignTextRenderDropdown(panel_button_t *button);
static void CG_HudEditor_SetupTitleText(panel_button_t *button);
static void CG_HudEditor_RenderEdit(panel_button_t *button);
static void CG_HudEditorX_Finish(panel_button_t *button);
static void CG_HudEditorY_Finish(panel_button_t *button);
static void CG_HudEditorWidth_Finish(panel_button_t *button);
static void CG_HudEditorHeight_Finish(panel_button_t *button);
static void CG_HudEditorScale_Finish(panel_button_t *button);
static qboolean CG_HudEditoColorSelection_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorPanel_KeyUp(panel_button_t *button, int key);
static void CG_HudEditorRender_Button(panel_button_t *button);
static qboolean CG_HudEditorVisible_CheckboxKeyDown(panel_button_t *button, int key);
static void CG_HudEditor_RenderCheckbox(panel_button_t *button);
static qboolean CG_HudEditorStyle_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorShowBackground_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorShowBorder_CheckboxKeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorButton_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditorButton_KeyUp(panel_button_t *button, int key);
static void CG_DrawHudEditor_ComponentLists(panel_button_t *button);
static qboolean CG_HudEditor_ComponentLists_KeyDown(panel_button_t *button, int key);
static qboolean CG_HudEditor_ComponentLists_KeyUp(panel_button_t *button, int key);

static panel_button_text_t hudEditorHeaderFont =
{
	0.2f,                  0.2f,
	{ 1.f,                 1.f, 1.f,  0.5f },
	ITEM_TEXTSTYLE_NORMAL, 0,
	&cgs.media.limboFont1,
};


static panel_button_text_t hudEditorTextFont =
{
	0.24f,                   0.24f,
	{ 1.f,                   1.f,  1.f,0.5f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorTextTitleFont =
{
	0.3f,                    0.3f,
	{ 1.f,                   1.f, 1.f,  1.f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_text_t hudEditorFont_Dropdown =
{
	0.24f,                   0.24f,
	{ 1.f,                   1.f,  1.f,0.5f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

static panel_button_t hudEditorHudDropdown =
{
	NULL,
	"hudeditor_huds",
	{ 0,                           HUDEDITOR_SELECTHUD_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                           0,         0,            0, 0, 0, 0, 1 },
	&hudEditorFont_Dropdown,       // font
	CG_HudEditor_Dropdown_KeyDown, // keyDown
	CG_HudEditor_HudDropdown_KeyUp,// keyUp
	CG_HudEditor_HudRenderDropdown,
	NULL,
	0,
};

static qboolean CG_HudEditor_EditClick(panel_button_t *button, int key)
{
	// don't modify default HUD
	if (!activehud->hudnumber)
	{
		return qfalse;
	}

	return BG_PanelButton_EditClick(button, key);
}

/**
 * @brief CG_HudEditorPanel_EditUp
 * @param button
 * @param key
 * @return
 */
static qboolean CG_HudEditorPanel_EditUp(panel_button_t *button, int key)
{
	return qtrue;
}

static panel_button_t hudEditorPositionSizeTitle =
{
	NULL,
	"Position & Size",
	{ 0,                        HUDEDITOR_SIZEPOS_Y,70, 14 },
	{ 0,                        0,      0,  0, 0, 0, 0, 1},
	&hudEditorTextTitleFont,    // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_HudEditor_SetupTitleText,
	NULL,
	0
};

static panel_button_t hudEditorX =
{
	NULL,
	"hudeditor_X",
	{ 0,                    HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y - INPUT_HEIGHT,INPUT_WIDTH, INPUT_HEIGHT },
	// [0] used by ui_shared EditClick, [1] link to hud, [2] used by ui_shared EditClick [3] additional data like colorRGB, [4] differentiate between hud editor element and hud element
	{ 0,                    0,                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditClick, // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorX_Finish,
	0
};

static panel_button_t hudEditorY =
{
	NULL,
	"hudeditor_Y",
	{ 0,                    HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y - INPUT_HEIGHT,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                    0,                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,     // font
	CG_HudEditor_EditClick, // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorY_Finish,
	0
};

static panel_button_t hudEditorColorsStyleTitle =
{
	NULL,
	"Colors & Style",
	{ 0,                        HUDEDITOR_COLORSSTYLE_Y,70, 14 },
	{ 0,                        0,           0,  0, 0, 0, 0, 1},
	&hudEditorTextTitleFont,    // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_HudEditor_SetupTitleText,
	NULL,
	0
};

static panel_button_t hudEditorW =
{
	NULL,
	"hudeditor_W",
	{ 0,                     HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CONTROLS_SPACER_XY,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditClick,  // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorWidth_Finish,
	0
};

static panel_button_t hudEditorH =
{
	NULL,
	"hudeditor_H",
	{ 0,                      HUDEDITOR_SIZEPOS_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CONTROLS_SPACER_XY,INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                      0,                                                                    0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,       // font
	CG_HudEditor_EditClick,   // keyDown
	CG_HudEditorPanel_EditUp, // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorHeight_Finish,
	0
};

static panel_button_t hudEditorScale =
{
	NULL,
	"hudeditor_S",
	{ 0,                     2 * (INPUT_HEIGHT + 2),INPUT_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,             0,           0, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditClick,  // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorScale_Finish,
	0
};

static panel_button_t hudEditorColorSelectionText =
{
	NULL,
	"Text",
	{ 0,                      HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y - BUTTON_HEIGHT,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                                                0,            5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorColorSelectionBorder =
{
	NULL,
	"Border",
	{ 0,                      HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y - BUTTON_HEIGHT,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                                              0,            5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorColorSelectionBackground =
{
	NULL,
	"BkGnd",
	{ 0,                      HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y - BUTTON_HEIGHT,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                                               0,            5, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditoColorSelection_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,  // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static void CG_HudEditorColor_Finish(panel_button_t *button);

static panel_button_t hudEditorColorR =
{
	NULL,
	"hudeditor_colorR",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CONTROLS_SPACER_XY,INPUT_COLOR_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                   0,                 0, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditClick,  // keyDown
	CG_HudEditorPanel_KeyUp, // keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static panel_button_t hudEditorColorG =
{
	NULL,
	"hudeditor_colorG",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + SLIDERS_HEIGHT + (HUDEDITOR_CONTROLS_SPACER_XY * 2),INPUT_COLOR_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                                          0,                 1, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditClick,  // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};
static panel_button_t hudEditorColorB =
{
	NULL,
	"hudeditor_colorB",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 3),INPUT_COLOR_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                                                0,                 2, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditClick,  // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};
static panel_button_t hudEditorColorA =
{
	NULL,
	"hudeditor_colorA",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),INPUT_COLOR_WIDTH, INPUT_HEIGHT },
	{ 0,                     0,                                                                                                0,                 3, 0, 0, 0, 1},
	&hudEditorTextFont,      // font
	CG_HudEditor_EditClick,  // keyDown
	CG_HudEditorPanel_EditUp,// keyUp
	CG_HudEditor_RenderEdit,
	CG_HudEditorColor_Finish,
	0
};

static qboolean CG_HudEditorColor_KeyDown(panel_button_t *button, int key);
static void CG_HudEditorColor_Render(panel_button_t *button);

static panel_button_t hudEditorColorSliderR =
{
	NULL,
	"hudeditor_colorsliderR",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + HUDEDITOR_CONTROLS_SPACER_XY,SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                     0,                                                             0,             0, 0, 0, 0, 1  },
	&hudEditorTextFont,      // font
	CG_HudEditorColor_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp, // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorColorSliderG =
{
	NULL,
	"hudeditor_colorsliderG",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + SLIDERS_HEIGHT + (HUDEDITOR_CONTROLS_SPACER_XY * 2),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                     0,                                                                                    0,             1, 0, 0, 0, 1  },
	&hudEditorTextFont,      // font
	CG_HudEditorColor_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp, // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorColorSliderB =
{
	NULL,
	"hudeditor_colorsliderB",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 2) + (HUDEDITOR_CONTROLS_SPACER_XY * 3),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                     0,                                                                                          0,             2, 0, 0, 0, 1  },
	&hudEditorTextFont,      // font
	CG_HudEditorColor_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp, // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorColorSliderA =
{
	NULL,
	"hudeditor_colorsliderA",
	{ 0,                     HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),SLIDERS_WIDTH, SLIDERS_HEIGHT },
	{ 0,                     0,                                                                                          0,             3, 0, 0, 0, 1  },
	&hudEditorTextFont,      // font
	CG_HudEditorColor_KeyDown,// keyDown
	CG_HudEditorPanel_KeyUp, // keyUp
	CG_HudEditorColor_Render,
	NULL,
	0
};

static panel_button_t hudEditorVisible =
{
	NULL,
	"Visible",
	{ 0,                        HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 4) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                                         0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorVisible_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorStyle =
{
	NULL,
	"Style",
	{ 0,                        HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 5) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                                           0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorStyle_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorShowBackground =
{
	NULL,
	"BckGrnd",
	{ 0,                        HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 6) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                                         0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorShowBackground_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorShowBorder =
{
	NULL,
	"Border",
	{ 0,                        HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 7) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),CHECKBOX_SIZE, CHECKBOX_SIZE },
	{ 0,                        0,                                                                                                          0,             0, 0, 0, 0, 1 },
	&hudEditorTextFont,         // font
	CG_HudEditorShowBorder_CheckboxKeyDown,// keyDown
	CG_HudEditorPanel_KeyUp,    // keyUp
	CG_HudEditor_RenderCheckbox,
	NULL,
	0
};

static panel_button_t hudEditorStyleText =
{
	NULL,
	"hudeditor_StyleText",
	{ 0,                                 HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 8) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),100, BUTTON_HEIGHT },
	{ 0,                                 0,                                                                                             0,   0, 0, 0, 0, 1 },
	&hudEditorFont_Dropdown,             // font
	CG_HudEditor_Dropdown_KeyDown,       // keyDown
	CG_HudEditor_StyleTextDropdown_KeyUp,// keyUp
	CG_HudEditor_StyleTextRenderDropdown,
	NULL,
	0,
};

static panel_button_t hudEditorAlignText =
{
	NULL,
	"hudeditor_Align",
	{ 0,                                 HUDEDITOR_COLORSSTYLE_Y + HUDEDITOR_TITLE_SPACER_Y + (SLIDERS_HEIGHT * 9) + (HUDEDITOR_CONTROLS_SPACER_XY * 4),60, BUTTON_HEIGHT },
	{ 0,                                 0,                                                                                                 0,  0, 0, 0, 0, 1 },
	&hudEditorFont_Dropdown,             // font
	CG_HudEditor_Dropdown_KeyDown,       // keyDown
	CG_HudEditor_AlignTextDropdown_KeyUp,// keyUp
	CG_HudEditor_AlignTextRenderDropdown,
	NULL,
	0,
};


static panel_button_t hudEditorSave =
{
	NULL,
	"Save",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                              0,            0, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorClone =
{
	NULL,
	"Clone",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                             0,            1, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorDelete =
{
	NULL,
	"Delete",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y,BUTTON_WIDTH, BUTTON_HEIGHT },
	{ 0,                      0,                                            0,            2, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorResetComp =
{
	NULL,
	"Reset Component",
	{ 0,                      HUDEDITOR_SELECTHUD_Y + HUDEDITOR_TITLE_SPACER_Y + BUTTON_HEIGHT + HUDEDITOR_CONTROLS_SPACER_XY,(BUTTON_WIDTH * 3) + (HUDEDITOR_CONTROLS_SPACER_XY * 2), BUTTON_HEIGHT },
	{ 0,                      0,                                                                                  0,                                                       3, 0, 0, 0, 1 },
	&hudEditorTextFont,       // font
	CG_HudEditorButton_KeyDown,// keyDown
	CG_HudEditorButton_KeyUp, // keyUp
	CG_HudEditorRender_Button,
	NULL,
	0
};

static panel_button_t hudEditorComponentsList =
{
	NULL,
	"hudeditor_componentsList",
	{ 3,                            SCREEN_HEIGHT + 3,SCREEN_WIDTH, SCREEN_HEIGHT_SAFE * 0.25 },
	{ 0,                            0,  0,            0, 0, 0, 0, 1             },
	&hudEditorFont_Dropdown,        // font
	CG_HudEditor_ComponentLists_KeyDown,// keyDown
	CG_HudEditor_ComponentLists_KeyUp,// keyUp
	CG_DrawHudEditor_ComponentLists,
	NULL,
	0
};

static panel_button_t *hudEditor[] =
{
	&hudEditorPositionSizeTitle,  &hudEditorX,                    &hudEditorY,
	&hudEditorColorsStyleTitle,   &hudEditorW,                    &hudEditorH,                       &hudEditorScale,
	&hudEditorColorSelectionText, &hudEditorColorSelectionBorder, &hudEditorColorSelectionBackground,
	&hudEditorColorR,             &hudEditorColorG,               &hudEditorColorB,                  &hudEditorColorA,
	&hudEditorColorSliderR,       &hudEditorColorSliderG,         &hudEditorColorSliderB,            &hudEditorColorSliderA,
	&hudEditorVisible,            &hudEditorStyle,                &hudEditorShowBackground,          &hudEditorShowBorder,
	&hudEditorSave,               &hudEditorClone,                &hudEditorDelete,                  &hudEditorResetComp,
	&hudEditorComponentsList,

	// Below here all components that should draw on top
	&hudEditorHudDropdown,        &hudEditorAlignText,            &hudEditorStyleText,
	NULL,
};

void CG_HUDSave_WriteComponent(fileHandle_t fh, int hudNumber, hudStucture_t *hud)
{
	int  j;
	char *s;

	s = va("\thud {\n\t\thudnumber %d\n", hudNumber);
	trap_FS_Write(s, strlen(s), fh);

	for (j = 0; hudComponentFields[j].name; j++)
	{
		if (!hudComponentFields[j].isAlias)
		{
			hudComponent_t *comp = (hudComponent_t *)((char *)hud + hudComponentFields[j].offset);
			s = va("\t\t%-16s\t"
			       "%-6.1f\t%-6.1f\t%-6.1f\t%-6.1f\t"
			       "%i\t%i\t"
			       "%-4.2f\t"
			       "%-4.2f\t%-4.2f\t%-4.2f\t%-4.2f\t"
			       "%i\t"
			       "%-4.2f\t%-4.2f\t%-4.2f\t%-4.2f\t"
			       "%i\t"
			       "%-4.2f\t%-4.2f\t%-4.2f\t%-4.2f\t"
			       "%i\t%i\n",
			       hudComponentFields[j].name,
			       Ccg_Is43Screen() ? comp->location.x : comp->location.x / cgs.adr43, comp->location.y, comp->location.w, comp->location.h,
			       comp->style, comp->visible,
			       comp->scale,
			       comp->colorText[0], comp->colorText[1], comp->colorText[2], comp->colorText[3],
			       comp->showBackGround,
			       comp->colorBackground[0], comp->colorBackground[1], comp->colorBackground[2], comp->colorBackground[3],
			       comp->showBorder,
			       comp->colorBorder[0], comp->colorBorder[1], comp->colorBorder[2], comp->colorBorder[3],
			       comp->styleText, comp->alignText);
			trap_FS_Write(s, strlen(s), fh);
		}
	}

	s = "\t}\n";
	trap_FS_Write(s, strlen(s), fh);
}

/**
 * @brief CG_HudSave
 * @param[in] HUDToDuplicate
 * @param[in] HUDToDelete
 */
qboolean CG_HudSave(int HUDToDuplicate, int HUDToDelete)
{
	int           i;
	fileHandle_t  fh;
	char          *s;
	hudStucture_t *hud;

	if (trap_FS_FOpenFile("hud.dat", &fh, FS_WRITE) < 0)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to save hud to 'hud.dat\n");
		return qfalse;
	}

	if (HUDToDelete == 0)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: can't delete default HUD\n");
		return qfalse;
	}

	if (HUDToDuplicate >= 0)
	{
		int num = 1;

		if (hudCount == MAXHUDS)
		{
			CG_Printf(S_COLOR_RED "ERROR CG_HudSave: no more free HUD slots for clone\n");
			return qfalse;
		}

		// find a free number
		for (i = 1; i < hudCount; i++)
		{
			hud = &hudlist[i];

			if (hud->hudnumber == num)
			{
				num++;
				i = 0;
			}
		}

		activehud         = CG_addHudToList(CG_getHudByNumber(HUDToDuplicate));
		cg_altHud.integer = activehud->hudnumber = num;

		CG_Printf("Clone hud %d on number %d\n", HUDToDuplicate, num);
	}

	s = "hudDef {\n";
	trap_FS_Write(s, strlen(s), fh);

	for (i = 1; i < hudCount; i++)
	{
		hud = &hudlist[i];

		if (hud->hudnumber == HUDToDelete)
		{
			int j;

			// remove last element instead of erasing by moving memory
			if (i == hudCount - 1)
			{
				Com_Memset(&hudlist[i], 0, sizeof(hudStucture_t));
			}
			else
			{
				memmove(&hudlist[i], &hudlist[i + 1], sizeof(hudStucture_t) * (hudCount - i - 1));
			}

			i--;
			hudCount--;

			// FIXME: found a more elegant way for keeping sorting
			for (j = i; j < hudCount; j++)
			{
				CG_HudComponentsFill(&hudlist[j]);
			}

			// Back to default HUD
			cg_altHud.integer = 0;
			activehud         = CG_getHudByNumber(0);

			continue;
		}

		CG_HUDSave_WriteComponent(fh, hud->hudnumber, hud);
	}

	s = "}\n";
	trap_FS_Write(s, strlen(s), fh);

	trap_FS_FCloseFile(fh);

	CG_Printf("Saved huds to 'hud.dat'\n");

	return qtrue;
}

/**
* @brief CG_HudEditor_SetupTitleText
* @param button
*/
static void CG_HudEditor_SetupTitleText(panel_button_t *button)
{
	float textWidth = CG_Text_Width_Ext(button->text, button->font->scalex, 0, button->font->font);
	button->rect.x = HUDEditorCenterX - (textWidth * 0.5f);
	BG_PanelButtonsRender_Text(button);
}

/**
* @brief CG_HudEditor_SetupEditPosition
* @param button
* @param label
* @param totalWidth
*/
static void CG_HudEditor_SetupEditPosition(panel_button_t *button, float totalWidth)
{
	// there's seemingly redundant repetition in this function, but we need explicit
	// calculation for every single editfield because client might be using
	// proportional custom font, so totalWidth doesn't necessarily match between X and W for example

	if (button == &hudEditorX)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorY)
	{
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorW)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorH)
	{
		button->rect.x = HUDEditorCenterX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorColorR)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorColorG)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorColorB)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
	else if (button == &hudEditorColorA)
	{
		button->rect.x = HUDEditorX + (HUDEditorWidth * 0.25f) - (totalWidth * 0.5f);
	}
}

/**
* @brief CG_HudEditor_RenderEdit
* @param button
*/
static void CG_HudEditor_RenderEdit(panel_button_t *button)
{
	char  label[32];
	float textWidth, textHeight, totalWidth;

	// FIXME: get proper names and adjust alignment after
	// !!! NOTE !!!
	// whitespace after : for spacing
	Com_sprintf(label, sizeof(label), "%c: ", button->text[strlen(button->text) - 1]);

	textWidth  = CG_Text_Width_Ext(label, button->font->scalex, 0, button->font->font);
	textHeight = CG_Text_Height_Ext(label, button->font->scaley, 0, button->font->font);
	totalWidth = textWidth + button->rect.w;

	// editfields for these are smaller, but we need the regular editfield width for alignment
	if (button == &hudEditorColorR || button == &hudEditorColorG
	    || button == &hudEditorColorB || button == &hudEditorColorA)
	{
		totalWidth = textWidth + Ccg_WideX(INPUT_WIDTH);
	}

	CG_HudEditor_SetupEditPosition(button, totalWidth);

	CG_Text_Paint_Ext(button->rect.x, button->rect.y + (button->rect.h * 0.5f) + (textHeight / 2),
	                  button->font->scalex, button->font->scaley, colorWhite, label, 0, 0,
	                  button->font->style, button->font->font);

	button->rect.x += textWidth;
	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, colorBlack);

	button->rect.x += 2; // for spacing
	button->rect.y -= button->rect.h * 0.5f - (textHeight * 0.5f);
	BG_PanelButton_RenderEdit(button);
	button->rect.x -= 2;
	button->rect.y += button->rect.h * 0.5f - (textHeight * 0.5f);
}

/**
* @brief CG_HudEditorX_Finish
* @param button
*/
static void CG_HudEditorX_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.x = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorY_Finish
* @param button
*/
static void CG_HudEditorY_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.y = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorWidth_Finish
* @param button
*/
static void CG_HudEditorWidth_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.w = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorHeight_Finish
* @param button
*/
static void CG_HudEditorHeight_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->location.h = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorScale_Finish
* @param button
*/
static void CG_HudEditorScale_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

	comp->scale = Q_atof(buffer);

	BG_PanelButtons_SetFocusButton(NULL);
}

/**
* @brief CG_HudEditorStyle_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorVisible_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!activehud->hudnumber)
	{
		return qfalse;
	}

	comp->visible = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorStyle_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorStyle_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

	if (!activehud->hudnumber)
	{
		return qfalse;
	}

	comp->style = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorShowBackground_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorShowBackground_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!activehud->hudnumber)
	{
		return qfalse;
	}

	comp->showBackGround = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditorShowBorder_CheckboxKeyDown
* @param button
*/
static qboolean CG_HudEditorShowBorder_CheckboxKeyDown(panel_button_t *button, int key)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

	// don't modify default HUD
	if (!activehud->hudnumber)
	{
		return qfalse;
	}

	comp->showBorder = button->data[2] = !button->data[2];

	BG_PanelButtons_SetFocusButton(NULL);

	SOUND_FILTER;

	return qtrue;
}

/**
* @brief CG_HudEditor_RenderCheckbox
* @param button
*/
static void CG_HudEditor_RenderCheckbox(panel_button_t *button)
{
	char  labelText[32];
	float textWidth;
	float textHeight;
	float totalWidth;
	float textOffsetY;

	// FIXME: get proper names and adjust alignment after
	Com_sprintf(labelText, sizeof(labelText), "%s: ", button->text);

	textWidth   = CG_Text_Width_Ext(labelText, button->font->scalex, 0, button->font->font);
	textHeight  = CG_Text_Height_Ext(labelText, button->font->scaley, 0, button->font->font);
	totalWidth  = textWidth + button->rect.w;
	textOffsetY = (BUTTON_HEIGHT - textHeight) * 0.5f;

	button->rect.x = HUDEditorCenterX - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, button->font->scalex, button->font->scaley,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;
	CG_DrawRect_FixedBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 2, colorBlack);

	if (button->data[2])
	{
		CG_DrawPic(button->rect.x + 2, button->rect.y + 2, CHECKBOX_SIZE - 3, CHECKBOX_SIZE - 3, cgs.media.readyShader);
	}
}

/**
* @brief CG_HudEditor_HudRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_HudRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "Select HUD: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.3f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.3f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (BUTTON_HEIGHT - textHeight) * 0.5f;

	button->rect.x = HUDEditorCenterX - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.3f, 0.3f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;
	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, va("%i", activehud->hudnumber),
	                   button == BG_PanelButtons_GetFocusButton(), button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; i < hudCount; i++)
		{
			hudStucture_t *hud = &hudlist[i];

			if (hud->hudnumber == activehud->hudnumber)
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, va("%i", hud->hudnumber), button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

static const char *styleTextString[] =
{
	"NORMAL",
	"BLINK",
	"PULSE",
	"SHADOWED",
	"OUTLINED",
	"OUTLINESHADOWED",
	"SHADOWEDMORE",
	NULL
};

/**
* @brief CG_HudEditor_StyleTextRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_StyleTextRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "Style: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.3f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.3f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (CHECKBOX_SIZE - textHeight) * 0.5f;

	button->rect.x = HUDEditorCenterX - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.3f, 0.3f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;

	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, styleTextString[button->data[2]], button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; styleTextString[i] != NULL; i++)
		{
			if (!Q_stricmp(styleTextString[button->data[2]], styleTextString[i]))
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, styleTextString[i], button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

static const char *alignTextString[] =
{
	"LEFT",
	"CENTER",
	"RIGHT",
	"CENTER2",
	NULL
};

/**
* @brief CG_HudEditor_AlignTextRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_AlignTextRenderDropdown(panel_button_t *button)
{
	const char *labelText  = "Align: ";
	float      textWidth   = CG_Text_Width_Ext(labelText, 0.3f, 0, button->font->font);
	float      textHeight  = CG_Text_Height_Ext(labelText, 0.3f, 0, button->font->font);
	float      totalWidth  = textWidth + button->rect.w;
	float      textOffsetY = (BUTTON_HEIGHT - textHeight) * 0.5f;

	button->rect.x = HUDEditorCenterX - (totalWidth * 0.5f);
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + textHeight + textOffsetY, 0.3f, 0.3f,
	                  colorWhite, labelText, 0, 0, button->font->style, button->font->font);

	button->rect.x += textWidth;

	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, alignTextString[button->data[2]], button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float  y = button->rect.y;
		vec4_t colour;
		int    i;

		for (i = 0; alignTextString[i] != NULL; i++)
		{
			if (!Q_stricmp(alignTextString[button->data[2]], alignTextString[i]))
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, alignTextString[i], button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

static panel_button_t hudEditorHudDropdown;
static panel_button_t hudEditorCompDropdown;

/**
* @brief CG_HudEditor_Dropdown_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_Dropdown_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		// don't modify default HUD but allow selecting comp and hud
		if (activehud->hudnumber || button == &hudEditorHudDropdown || button == &hudEditorCompDropdown)
		{
			BG_PanelButtons_SetFocusButton(button);
			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_HudDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_HudDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; i < hudCount; i++)
			{
				hudStucture_t *hud = &hudlist[i];

				if (hud->hudnumber == activehud->hudnumber)
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					cg_altHud.integer = hud->hudnumber;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_StyleTextDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_StyleTextDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; styleTextString[i] != NULL; i++)
			{
				if (!Q_stricmp(styleTextString[button->data[2]], styleTextString[i]))
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

					comp->styleText = button->data[2] = i;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_AlignTextDropdown_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_AlignTextDropdown_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t rect;
			int       i;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			for (i = 0; alignTextString[i] != NULL; i++)
			{
				if (!Q_stricmp(alignTextString[button->data[2]], alignTextString[i]))
				{
					continue;
				}

				rect.y += button->rect.h;

				if (BG_CursorInRect(&rect))
				{
					hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);

					comp->alignText = button->data[2] = i;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/*
static char *colorSelectionElement[] =
{
	"Text",
	"BckGrnd",
	"Border",
};
 */

static qboolean CG_HudEditoColorSelection_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		if (button == &hudEditorColorSelectionText)
		{
			//button->data[3] = 0;
			elementColorSelection = 0;
		}
		else if (button == &hudEditorColorSelectionBorder)
		{
			//button->data[3] = 1;
			elementColorSelection = 1;
		}
		else if (button == &hudEditorColorSelectionBackground)
		{
			//button->data[3] = 2;
			elementColorSelection = 2;
		}
//		button->data[3] = (button->data[3] >= ARRAY_LEN(colorSelectionElement) - 1) ? 0 : ++(button->data[3]);
//
//		button->text = colorSelectionElement[button->data[3]];

		if (lastFocusComponent)
		{
			CG_HudEditorUpdateFields(lastFocusComponent);
		}
		return qtrue;
	}

	return qfalse;
}

static qboolean CG_HudEditorButton_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		button->data[4] = cg.time;

		return qtrue;
	}

	return qfalse;
}

static qboolean CG_HudEditorButton_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		SOUND_SELECT;

		button->data[4] = 0;

		return qtrue;
	}

	return qfalse;
}

static void CG_ResetComponent()
{
	if (lastFocusComponent)
	{
		hudComponent_t *comp;
		hudComponent_t *defaultComp;

		comp        = (hudComponent_t *)((char *)activehud + hudComponentFields[lastFocusComponent->data[0]].offset);
		defaultComp = (hudComponent_t *)((char *)CG_getHudByNumber(0) + hudComponentFields[lastFocusComponent->data[0]].offset);

		Com_Memcpy(comp, defaultComp, sizeof(hudComponent_t));

		CG_HudEditorUpdateFields(lastFocusComponent);
	}
}

/**
 * @brief CG_HudEditorRender_Button_Ext
 * @param[in] r
 * @param[in] text
 * @param[in] font
 */
void CG_HudEditorRender_Button_Ext(rectDef_t *r, const char *text, panel_button_text_t *font)
{
	vec4_t clrBdr = { 0.1f, 0.1f, 0.1f, 0.5f };
	vec4_t clrBck = { 0.3f, 0.3f, 0.3f, 0.4f };

	vec4_t clrBck_hi = { 0.5f, 0.5f, 0.5f, 0.4f };
	vec4_t clrTxt_hi = { 0.9f, 0.9f, 0.9f, 1.f };

	qboolean hilight = BG_CursorInRect(r);

	CG_FillRect(r->x, r->y, r->w, r->h, hilight ? clrBck_hi : clrBck);
	CG_DrawRect_FixedBorder(r->x, r->y, r->w, r->h, 1, clrBdr);

	if (text)
	{
		float w;

		w = CG_Text_Width_Ext(text, font->scalex, 0, font->font);

		CG_Text_Paint_Ext(r->x + ((r->w + 2) - w) * 0.5f, r->y + r->h / 1.5, font->scalex, font->scaley, hilight ? clrTxt_hi : font->colour, text, font->align, 0, font->style, font->font);
	}
}

/**
 * @brief CG_HudEditor_SetupButtonPosition
 * @param[in] button
 * @param[in] buttonW
 */
static float CG_HudEditor_SetupButtonPosition(panel_button_t *button, float buttonW)
{
	// left aligned
	if (button == &hudEditorSave || button == &hudEditorResetComp || button == &hudEditorColorSelectionText)
	{
		button->rect.x = HUDEditorCenterX - (buttonW * 0.5f) - buttonW - HUDEDITOR_CONTROLS_SPACER_XY;
	}
	// centered
	else if (button == &hudEditorClone || button == &hudEditorColorSelectionBorder)
	{
		button->rect.x = HUDEditorCenterX - (buttonW * 0.5f);
	}
	// right aligned
	else if (button == &hudEditorDelete || button == &hudEditorColorSelectionBackground)
	{
		button->rect.x = HUDEditorCenterX + (buttonW * 0.5f) + HUDEDITOR_CONTROLS_SPACER_XY;
	}

	return 0;
}

#define TIMER_KEYDOWN 750.f

/**
 * @brief CG_PanelButtonsRender_Button
 * @param[in] CG_HudEditorRender_Button
 */
static void CG_HudEditorRender_Button(panel_button_t *button)
{
	float buttonW = Ccg_WideX(BUTTON_WIDTH);

	CG_HudEditor_SetupButtonPosition(button, buttonW);

	if (button->data[4])
	{
		vec4_t backG    = { 1, 1, 1, 0.3f };
		float  curValue = (cg.time - button->data[4]) / TIMER_KEYDOWN;

		CG_FilledBar(button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorRed, colorGreen, backG, curValue, BAR_LERP_COLOR);

		if (curValue > 1.f)
		{
			switch (button->data[3])
			{
			case 0:
				CG_HudSave(-1, -1);
				break;
			case 1:
				CG_HudSave(activehud->hudnumber, -1);
				break;
			case 2:
				CG_HudSave(-1, activehud->hudnumber);
				break;
			case 3:
				CG_ResetComponent();
				break;
			default:
				break;
			}

			button->data[4] = 0;
		}
	}

	CG_HudEditorRender_Button_Ext(&button->rect, button->text, button->font);
}

/**
 * @brief CG_HudEditorPanel_KeyUp
 * @param button
 * @param key
 * @return
 */
static qboolean CG_HudEditorPanel_KeyUp(panel_button_t *button, int key)
{
	BG_PanelButtons_SetFocusButton(NULL);
	return qtrue;
}

/**
* @brief CG_HudEditorUpdateFields
* @param[in] button
*/
static void CG_HudEditorUpdateFields(panel_button_t *button)
{
	hudComponent_t *comp;
	char           buffer[256];
	vec4_t(*compColor) = NULL;

	comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.x);
	trap_Cvar_Set("hudeditor_x", buffer);
	hudEditorX.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.y);
	trap_Cvar_Set("hudeditor_y", buffer);
	hudEditorY.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.w);
	trap_Cvar_Set("hudeditor_w", buffer);
	hudEditorW.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->location.h);
	trap_Cvar_Set("hudeditor_h", buffer);
	hudEditorH.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->scale);
	trap_Cvar_Set("hudeditor_s", buffer);
	hudEditorScale.data[1] = button->data[0];

	switch (elementColorSelection /*hudEditorColorSelection.data[3]*/)
	{
	case 0: compColor = &comp->colorText; break;
	case 1: compColor = &comp->colorBackground; break;
	case 2: compColor = &comp->colorBorder; break;
	default: break;
	}

	if (compColor)
	{
		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[0] * 255.0f);
		trap_Cvar_Set("hudeditor_colorR", buffer);
		hudEditorColorR.data[1] = button->data[0];

		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[1] * 255.0f);
		trap_Cvar_Set("hudeditor_colorG", buffer);
		hudEditorColorG.data[1] = button->data[0];

		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[2] * 255.0f);
		trap_Cvar_Set("hudeditor_colorB", buffer);
		hudEditorColorB.data[1] = button->data[0];

		Com_sprintf(buffer, sizeof(buffer), "%0.1f", (*compColor)[3] * 255.0f);
		trap_Cvar_Set("hudeditor_colorA", buffer);
		hudEditorColorA.data[1] = button->data[0];

		hudEditorColorSliderR.data[1] = button->data[0];
		hudEditorColorSliderG.data[1] = button->data[0];
		hudEditorColorSliderB.data[1] = button->data[0];
		hudEditorColorSliderA.data[1] = button->data[0];
	}
/*
	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBackground[0] * 255.0f);
	trap_Cvar_Set("hudeditor_colorbackgroundR", buffer);
	hudEditorColorBackgroundR.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBackground[1] * 255.0f);
	trap_Cvar_Set("hudeditor_colorbackgroundG", buffer);
	hudEditorColorBackgroundG.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBackground[2] * 255.0f);
	trap_Cvar_Set("hudeditor_colorbackgroundB", buffer);
	hudEditorColorBackgroundB.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBackground[3] * 255.0f);
	trap_Cvar_Set("hudeditor_colorbackgroundA", buffer);
	hudEditorColorBackgroundA.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBorder[0] * 255.0f);
	trap_Cvar_Set("hudeditor_colorborderR", buffer);
	hudEditorColorBorderR.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBorder[1] * 255.0f);
	trap_Cvar_Set("hudeditor_colorborderG", buffer);
	hudEditorColorBorderG.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBorder[2] * 255.0f);
	trap_Cvar_Set("hudeditor_colorborderB", buffer);
	hudEditorColorBorderB.data[1] = button->data[0];

	Com_sprintf(buffer, sizeof(buffer), "%0.1f", comp->colorBorder[3] * 255.0f);
	trap_Cvar_Set("hudeditor_colorborderA", buffer);
	hudEditorColorBorderA.data[1] = button->data[0];
*/
	hudEditorVisible.data[1] = button->data[0];
	hudEditorVisible.data[2] = comp->visible;

	hudEditorStyle.data[1] = button->data[0];
	hudEditorStyle.data[2] = comp->style;

	hudEditorShowBackground.data[1] = button->data[0];
	hudEditorShowBackground.data[2] = comp->showBackGround;

	hudEditorShowBorder.data[1] = button->data[0];
	hudEditorShowBorder.data[2] = comp->showBorder;

	hudEditorStyleText.data[1] = button->data[0];
	hudEditorStyleText.data[2] = comp->styleText;

	hudEditorAlignText.data[1] = button->data[0];
	hudEditorAlignText.data[2] = comp->alignText;
}

/**
* @brief CG_HudEditorRender draw borders for hud elements
* @param[in] button
*/
static void CG_HudEditor_Render(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);
	vec4_t         *color;

	button->rect = comp->location;

	if (button == lastFocusComponent)
	{
		color = &colorYellow;
	}
	else if (showAllLayout || BG_CursorInRect(&button->rect))
	{
		color = comp->visible ? &colorMdGreen : &colorMdRed;
	}
	else
	{
		return;
	}

	CG_DrawRect_FixedBorder(button->rect.x - 1, button->rect.y - 1, button->rect.w + 2, button->rect.h + 2, 2, *color);
}

/**
* @brief CG_HudEditor_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

		if (lastFocusComponent && BG_CursorInRect(&lastFocusComponent->rect))
		{
			CG_HudEditorUpdateFields(lastFocusComponent);
			lastFocusComponent->data[7] = 0;

			return qtrue;
		}
		else if (comp->visible)
		{
			CG_HudEditorUpdateFields(button);
			BG_PanelButtons_SetFocusButton(button);
			button->data[7] = 0;

			return qtrue;
		}
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

		if (lastFocusComponent && lastFocusComponentMoved)
		{
			lastFocusComponentMoved     = qfalse;
			lastFocusComponent->data[7] = 1;

			return qtrue;
		}
		else if (comp->visible)
		{
			lastFocusComponent = button;
			BG_PanelButtons_SetFocusButton(NULL);
			button->data[7] = 1;

			return qtrue;
		}
	}

	return qfalse;
}

panel_button_t *hudComponentsPanel[HUD_COMPONENTS_NUM + 1];
panel_button_t hudComponents[HUD_COMPONENTS_NUM];

/**
* @brief CG_HudEditor_CompRenderDropdown
* @param[in] button
*/
static void CG_HudEditor_CompRenderDropdown(panel_button_t *button)
{
	CG_DropdownMainBox(button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	                   button->font->scalex, button->font->scaley, colorBlack, lastFocusComponent ? lastFocusComponent->text : "Select Comp", button == BG_PanelButtons_GetFocusButton(),
	                   button->font->colour, button->font->style, button->font->font);

	if (button == BG_PanelButtons_GetFocusButton())
	{
		float          y = button->rect.y;
		vec4_t         colour;
		panel_button_t **buttons = hudComponentsPanel;
		panel_button_t *parsedButton;

		for ( ; *buttons; buttons++)
		{
			parsedButton = (*buttons);

			if (parsedButton == lastFocusComponent)
			{
				continue;
			}

			y = CG_DropdownBox(button->rect.x, y, button->rect.w, button->rect.h,
			                   button->font->scalex, button->font->scaley, colorBlack, parsedButton->text, button == BG_PanelButtons_GetFocusButton(),
			                   button->font->colour, button->font->style, button->font->font);
		}

		VectorCopy(colorBlack, colour);
		colour[3] = 0.3f;
		CG_DrawRect(button->rect.x, button->rect.y + button->rect.h, button->rect.w, y - button->rect.y, 1.0f, colour);
	}
}

/**
* @brief CG_HudEditorColor_Finish colors
* @param button
*/
static void CG_HudEditorColor_Finish(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	char           buffer[MAX_EDITFIELD];

	trap_Cvar_VariableStringBuffer(button->text, buffer, MAX_EDITFIELD);

//	switch (hudEditorColorSelection.data[3])
//	{
//	case 0: comp->colorText[button->data[3]]       = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
//	case 1: comp->colorBackground[button->data[3]] = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
//	case 2: comp->colorBorder[button->data[3]]     = Com_Clamp(0, 1.0f, Q_atof(buffer) / 255.0f); break;
//	default: break;
//	}

	if (lastFocusComponent)
	{
		CG_HudEditorUpdateFields(lastFocusComponent);
	}

	BG_PanelButtons_SetFocusButton(NULL);
}

static qboolean CG_HudEditorColor_KeyDown(panel_button_t *button, int key)
{
	// don't modify default HUD
	if (activehud->hudnumber && key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);

		return qtrue;
	}

	return qfalse;
}

static void CG_HudEditorColor_Render(panel_button_t *button)
{
	hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[1]].offset);
	vec4_t         backG = { 1, 1, 1, 0.3f };
	vec4_t         *color;
	float          offset;

	// update color continuously
	if (lastFocusComponent && BG_PanelButtons_GetFocusButton() == button)
	{
		offset = Com_Clamp(0, 1.0f, (cgs.cursorX - button->rect.x) / button->rect.w);

//		switch (hudEditorColorSelection.data[3])
//		{
//		case 0: comp->colorText[button->data[3]]       = offset; break;
//		case 1: comp->colorBackground[button->data[3]] = offset; break;
//		case 2: comp->colorBorder[button->data[3]]     = offset; break;
//		default: break;
//		}

		CG_HudEditorUpdateFields(lastFocusComponent);
	}
	else
	{
//		switch (hudEditorColorSelection.data[3])
//		{
//		case 0: offset = comp->colorText[button->data[3]]      ; break;
//		case 1: offset = comp->colorBackground[button->data[3]]; break;
//		case 2: offset = comp->colorBorder[button->data[3]]    ; break;
//		default: break;
//		}
	}

	switch (button->data[3])
	{
	case 0: color = &colorRed; break;
	case 1: color = &colorGreen; break;
	case 2: color = &colorBlue; break;
	case 3: color = &colorWhite; break;
	default: return;
	}

	//CG_FilledBar(button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorBlack, *color, backG, offset, BAR_BORDER | BAR_LERP_COLOR);
}

/**
* @brief CG_HudEditorSetup
*/
void CG_HudEditorSetup(void)
{
	int i, j;

	// setup some useful coordinates for the side panel
	HUDEditorX       = SCREEN_WIDTH_SAFE;
	HUDEditorWidth   = (HUDEditorX * 1.25f) - HUDEditorX;
	HUDEditorCenterX = HUDEditorX + (HUDEditorWidth * 0.5f);

	for (i = 0, j = 0; hudComponentFields[i].name; i++, j++)
	{
		hudComponent_t *comp;

		if (hudComponentFields[i].isAlias)
		{
			j--;
			continue;
		}

		comp = (hudComponent_t *)((char *)activehud + hudComponentFields[i].offset);

		hudComponents[j].text      = hudComponentFields[i].name;
		hudComponents[j].rect      = comp->location;
		hudComponents[j].onKeyDown = CG_HudEditor_KeyDown;
		hudComponents[j].onKeyUp   = CG_HudEditor_KeyUp;
		hudComponents[j].onDraw    = CG_HudEditor_Render;
		hudComponents[j].data[0]   = i; // link button to hud component

		hudComponentsPanel[j] = &hudComponents[j];
	}

	if (!wsAdjusted)
	{
		// set up the drawing of HUD editor controls to the right side panel
		for (i = 0; hudEditor[i]; i++)
		{
			// FIXME: temporary, remove if statement once all elements are repositioned
			if (!hudEditor[i]->rect.x)
			{
				hudEditor[i]->rect.x = HUDEditorX;
			}
			hudEditor[i]->rect.w = Ccg_WideX(hudEditor[i]->rect.w);
		}

		wsAdjusted = qtrue;
	}

	// last element needs to be NULL
	hudComponentsPanel[HUD_COMPONENTS_NUM] = NULL;

	// clear last selected button
	lastFocusComponent = NULL;

	elementColorSelection = 0;
}

#define COMPONENT_BUTTON_WIDTH 120
#define COMPONENT_BUTTON_HEIGHT 12
#define COMPONENT_BUTTON_SPACE_X 2
#define COMPONENT_BUTTON_SPACE_Y 2

/**
 * @brief CG_DrawHudEditor_ComponentLists
 */
static void CG_DrawHudEditor_ComponentLists(panel_button_t *button)
{
	float          x = button->rect.x;
	float          y = button->rect.y;
	int            offsetX, offsetY;
	panel_button_t **buttons = hudComponentsPanel;
	panel_button_t *parsedButton;
	hudComponent_t *comp;

	for ( ; *buttons; buttons++)
	{
		parsedButton = (*buttons);

		comp = (hudComponent_t *)((char *)activehud + hudComponentFields[parsedButton->data[0]].offset);

		CG_FillRect(x, y, COMPONENT_BUTTON_WIDTH, COMPONENT_BUTTON_HEIGHT, lastFocusComponent == parsedButton ? (vec4_t) { 1, 1, 0, 0.4f } : (vec4_t) { 0.3f, 0.3f, 0.3f, 0.4f });

		if (BG_CursorInRect(&(rectDef_t) { x, y, COMPONENT_BUTTON_WIDTH, COMPONENT_BUTTON_HEIGHT }))
		{
			CG_FillRect(x, y, COMPONENT_BUTTON_WIDTH, COMPONENT_BUTTON_HEIGHT, (vec4_t) { 1, 1, 1, 0.4f });

			if (parsedButton != lastFocusComponent)
			{
				CG_DrawRect_FixedBorder(parsedButton->rect.x - 1, parsedButton->rect.y - 1, parsedButton->rect.w + 2, parsedButton->rect.h + 2, 2, comp->visible ? colorMdGreen : colorMdRed);
			}
		}

		offsetX = CG_Text_Width_Ext(parsedButton->text, button->font->scalex, 0, button->font->font);
		offsetY = CG_Text_Height_Ext(parsedButton->text, button->font->scaley, 0, button->font->font);

		CG_Text_Paint_Ext(x + (COMPONENT_BUTTON_WIDTH - offsetX) * 0.5f, y + ((COMPONENT_BUTTON_HEIGHT + offsetY) * 0.5f), button->font->scalex, button->font->scaley,
		                  comp->visible ? colorMdGreen : colorMdRed, parsedButton->text, 0, 0, button->font->style, button->font->font);

		y += COMPONENT_BUTTON_HEIGHT + COMPONENT_BUTTON_SPACE_Y;

		if (y + COMPONENT_BUTTON_HEIGHT >= SCREEN_HEIGHT_SAFE + (SCREEN_HEIGHT_SAFE * 0.25f))
		{
			y  = button->rect.y;
			x += COMPONENT_BUTTON_WIDTH + COMPONENT_BUTTON_SPACE_X;
		}
	}
}

/**
* @brief CG_HudEditor_ComponentLists_KeyDown
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_ComponentLists_KeyDown(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

/**
* @brief CG_HudEditor_ComponentLists_KeyUp
* @param[in] button
* @param[in] key
* @return
*/
static qboolean CG_HudEditor_ComponentLists_KeyUp(panel_button_t *button, int key)
{
	if (key == K_MOUSE1)
	{
		if (button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t      rect;
			panel_button_t **buttons = hudComponentsPanel;
			panel_button_t *parsedButton;

			Com_Memcpy(&rect, &button->rect, sizeof(rect));

			rect.w = COMPONENT_BUTTON_WIDTH;
			rect.h = COMPONENT_BUTTON_HEIGHT;

			for ( ; *buttons; buttons++)
			{
				parsedButton = (*buttons);

				if (BG_CursorInRect(&rect))
				{
					SOUND_SELECT;
					lastFocusComponent = parsedButton;
					CG_HudEditorUpdateFields(parsedButton);
					break;
				}

				rect.y += rect.h + COMPONENT_BUTTON_SPACE_Y;

				if (rect.y + COMPONENT_BUTTON_HEIGHT >= SCREEN_HEIGHT_SAFE + (SCREEN_HEIGHT_SAFE * 0.25f))
				{
					rect.y  = button->rect.y;
					rect.x += COMPONENT_BUTTON_WIDTH + COMPONENT_BUTTON_SPACE_X;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_DrawHudEditor_ToolTip
 * @param[in] name
 */
static void CG_DrawHudEditor_ToolTip(panel_button_t *button)
{
	int offsetX = CG_Text_Width_Ext(button->text, 0.20f, 0, &cgs.media.limboFont1);

	if (cgDC.cursorx + 10 + offsetX >= 640)
	{
		CG_Text_Paint_Ext(cgDC.cursorx - 10 - offsetX, cgDC.cursory, 0.20f, 0.22f, colorGreen, button->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	else
	{
		CG_Text_Paint_Ext(cgDC.cursorx + 10, cgDC.cursory, 0.20f, 0.22f, colorGreen, button->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

static int helpStatus = SHOW_ON;

static void CG_HudEditor_ToogleShowLayout(void)
{
    showAllLayout = !showAllLayout;
}

static void CG_HudEditor_ToogleHelp(void)
{
	if (helpStatus != SHOW_ON)
	{
		CG_ShowHelp_On(&helpStatus);
	}
	else if (helpStatus == SHOW_ON)
	{
		CG_ShowHelp_Off(&helpStatus);
	}
}

/**
 * @brief CG_HudEditor_HelpDraw
 */
static void CG_HudEditor_HelpDraw(void)
{
	if (helpStatus != SHOW_OFF)
	{
		static const helpType_t help[] =
		{
			{ "K_DOWN",              "move down by 1px"                  },
			{ "K_LEFT",              "move left by 1px"                  },
			{ "K_UP",                "move down by 1px"                  },
			{ "K_RIGHT",             "move right by 1px"                 },
			{ NULL,                  NULL                                },
			{ "K_RCTRL / K_LCTRL",   "hold to move by 0.1px"             },
			{ "K_RSHIFT / K_LSHIFT", "hold to move by 5px"               },
			{ NULL,                  NULL                                },
			{ "K_RALT / K_LALT",     "hold to resize"                    },
			{ NULL,                  NULL                                },
			{ "K_PGUP",              "move from bottom -> middle -> top" },
			{ "K_PGDN",              "move from top -> middle -> bottom" },
			{ "K_HOME",              "move from left -> middle -> right" },
			{ "K_END",               "move from right -> middle -> left" },
			{ NULL,                  NULL                                },
            { "l",                   "show all layout on/off"            },
			{ "h",                   "help on/off"                       },
		};

		vec4_t bgColor;

		VectorCopy(colorLtGrey, bgColor);
		bgColor[3] = .5f;

		CG_DrawHelpWindow(Ccg_WideX(SCREEN_WIDTH) * 0.1, SCREEN_HEIGHT * 0.6, &helpStatus, "HUD EDITOR CONTROLS", help, sizeof(help) / sizeof(helpType_t),
		                  bgColor, colorBlack, colorMdGrey, colorBlack,
		                  &hudEditorHeaderFont, &hudEditorTextFont);
	}
}

/**
* @brief CG_DrawHudEditor
*/
void CG_DrawHudEditor(void)
{
	panel_button_t **buttons = hudComponentsPanel;
	panel_button_t *button;
	hudComponent_t *comp;

	BG_PanelButtonsRender(hudComponentsPanel);
	BG_PanelButtonsRender(hudEditor);
	CG_HudEditor_HelpDraw();

	trap_R_SetColor(NULL);
	CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);

	// start parsing hud components from the last focused button
	qboolean skip = qtrue;
	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

		if (skip)
		{
			if (button != lastFocusComponent)
			{
				continue;
			}

			skip = qfalse;
		}

		if (comp->visible && BG_CursorInRect(&button->rect))
		{
			CG_DrawHudEditor_ToolTip(button);
			return;
		}
	}

	// start for beginning
	buttons = hudComponentsPanel;

	for ( ; *buttons; buttons++)
	{
		button = (*buttons);

		comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

		// early return
		if (lastFocusComponent && lastFocusComponent == button)
		{
			break;
		}

		if (comp->visible && BG_CursorInRect(&button->rect))
		{
			CG_DrawHudEditor_ToolTip(button);
			break;
		}
	}
}

/**
* @brief CG_HudEditor_KeyHandling
* @param[in] key
* @param[in] down
*/
void CG_HudEditor_KeyHandling(int key, qboolean down)
{
	if (BG_PanelButtonsKeyEvent(key, down, hudEditor))
	{
		return;
	}

	if (key == K_MOUSE2)
	{
		lastFocusComponent = NULL;
		return;
	}
    
    if (key == 'l' && down)
    {
        CG_HudEditor_ToogleShowLayout();
        return;
    }

	if (key == 'h' && down)
	{
		CG_HudEditor_ToogleHelp();
		return;
	}

	// start parsing hud components from the last focused button
	if (lastFocusComponent)
	{
		panel_button_t **buttons = hudComponentsPanel;
		panel_button_t *button;

		for ( ; *buttons; buttons++)
		{
			button = (*buttons);

			if (button == lastFocusComponent)
			{
				if (BG_PanelButtonsKeyEvent(key, down, ++buttons))
				{
					return;
				}
				break;
			}
		}
	}

	if (BG_PanelButtonsKeyEvent(key, down, hudComponentsPanel))
	{
		return;
	}

	// don't modify default HUD
	if (activehud->hudnumber && lastFocusComponent && down)
	{
		hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[lastFocusComponent->data[0]].offset);
		qboolean       changeSize;
		float          offset;
		float          *pValue;

		changeSize = (trap_Key_IsDown(K_RALT) || trap_Key_IsDown(K_LALT));

		if (trap_Key_IsDown(K_RCTRL) || trap_Key_IsDown(K_LCTRL))
		{
			offset = 0.1f;
		}
		else if (trap_Key_IsDown(K_RSHIFT) || trap_Key_IsDown(K_LSHIFT))
		{
			offset = 5;
		}
		else
		{
			offset = 1;
		}

		switch (key)
		{
		case K_LEFTARROW:  pValue           = (changeSize ? &comp->location.w : &comp->location.x); *pValue -= offset ; break;
		case K_RIGHTARROW: pValue           = (changeSize ? &comp->location.w : &comp->location.x); *pValue += offset ; break;
		case K_UPARROW:    pValue           = (changeSize ? &comp->location.h : &comp->location.y); *pValue -= offset ; break;
		case K_DOWNARROW:  pValue           = (changeSize ? &comp->location.h : &comp->location.y); *pValue += offset ; break;
		case K_PGUP:       comp->location.y = ((comp->location.y <= (SCREEN_HEIGHT - comp->location.h) / 2.f) ?
			                                   0 : (SCREEN_HEIGHT - comp->location.h) / 2.f); break;
		case K_PGDN:       comp->location.y = ((comp->location.y < (SCREEN_HEIGHT - comp->location.h) / 2.f) ?
			                                   (SCREEN_HEIGHT - comp->location.h) / 2.f : SCREEN_HEIGHT - comp->location.h); break;
		case K_HOME:       comp->location.x = (((int)comp->location.x <= (int)((Ccg_WideX(SCREEN_WIDTH) - comp->location.w) / 2.f)) ?
			                                   0 : (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) / 2.f); break;
		case K_END:        comp->location.x = ((comp->location.x < (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) / 2.f) ?
			                                   (Ccg_WideX(SCREEN_WIDTH) - comp->location.w) / 2.f: Ccg_WideX(SCREEN_WIDTH) - comp->location.w); break;
		default: return;
		}

		CG_HudEditorUpdateFields(lastFocusComponent);

		return;
	}
}

/**
* @brief CG_HudEditorMouseMove_Handling
* @param[in] x
* @param[in] y
*/
void CG_HudEditorMouseMove_Handling(int x, int y)
{
	if (!cg.editingHud)
	{
		return;
	}

	panel_button_t *button = lastFocusComponent;
	static float   offsetX = 0;
	static float   offsetY = 0;

	// don't modify default HUD
	if (activehud->hudnumber && button && !button->data[7] && BG_CursorInRect(&button->rect))
	{
		hudComponent_t *comp = (hudComponent_t *)((char *)activehud + hudComponentFields[button->data[0]].offset);

		lastFocusComponentMoved = qtrue;

		if (!offsetX && !offsetY)
		{
			offsetX = (x - comp->location.x);
			offsetY = (y - comp->location.y);
		}

		comp->location.x = x - offsetX;
		comp->location.y = y - offsetY;
		CG_HudEditorUpdateFields(button);
	}
	else
	{
		offsetX = 0;
		offsetY = 0;
	}
}