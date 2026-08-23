SET(SOURCES ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_mono_frame_10_png.c
    ui_img_mono_frame_11_png.c
    ui_img_mono_frame_12_png.c
    ui_img_mono_frame_13_png.c
    ui_img_mono_frame_14_png.c
    ui_img_mono_frame_15_png.c
    ui_img_mono_frame_16_png.c
    ui_img_mono_frame_17_png.c
    ui_img_mono_frame_18_png.c
    ui_img_mono_frame_19_png.c
    ui_font_Font1.c
    ui_font_Font18.c)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES lvgl
)
