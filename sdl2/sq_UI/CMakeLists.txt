SET(SOURCES screens/ui_Screen0.c
    screens/ui_ScreenDial.c
    screens/ui_ScreenApp.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_screenshot1_png.c
    images/ui_img_dial_transparent_png.c
    images/ui_img_hour_hand_png.c
    images/ui_img_minute_hand_png.c
    images/ui_img_second_hand_png.c
    fonts/ui_font_ScurceHanfont.c)

add_library(ui ${SOURCES})
