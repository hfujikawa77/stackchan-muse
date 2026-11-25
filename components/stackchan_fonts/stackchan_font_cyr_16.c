/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font unifont.otf --size 16 --bpp 1 --format lvgl --lv-font-name stackchan_font_cyr_16 --no-compress -r 0x400-0x52F -o font.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef STACKCHAN_FONT_CYR_16
#define STACKCHAN_FONT_CYR_16 1
#endif

#if STACKCHAN_FONT_CYR_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0400 "Ѐ" */
    0x60, 0x60, 0x3f, 0x82, 0x8, 0x3e, 0x82, 0x8,
    0x20, 0xfc,

    /* U+0401 "Ё" */
    0x48, 0x0, 0x0, 0xfe, 0x8, 0x20, 0xfa, 0x8,
    0x20, 0x83, 0xf0,

    /* U+0402 "Ђ" */
    0xfc, 0x40, 0x81, 0x3, 0xc4, 0x48, 0x91, 0x22,
    0x58,

    /* U+0403 "Ѓ" */
    0x19, 0x80, 0x0, 0xfe, 0x8, 0x20, 0x82, 0x8,
    0x20, 0x82, 0x0,

    /* U+0404 "Є" */
    0x39, 0x18, 0x20, 0xfa, 0x8, 0x20, 0x44, 0xe0,

    /* U+0405 "Ѕ" */
    0x7a, 0x18, 0x20, 0x60, 0x60, 0x61, 0x89, 0xe0,

    /* U+0406 "І" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+0407 "Ї" */
    0x90, 0x0, 0xf, 0x90, 0x84, 0x21, 0x8, 0x42,
    0x7c,

    /* U+0408 "Ј" */
    0x1c, 0x20, 0x82, 0x8, 0x20, 0xa2, 0x91, 0xc0,

    /* U+0409 "Љ" */
    0x78, 0x48, 0x48, 0x48, 0x4e, 0x49, 0x49, 0x49,
    0x49, 0x8e,

    /* U+040A "Њ" */
    0x91, 0x22, 0x44, 0x8f, 0xd2, 0x64, 0xc9, 0x93,
    0x38,

    /* U+040B "Ћ" */
    0xfc, 0x40, 0x81, 0x3, 0xc4, 0x48, 0x91, 0x22,
    0x44,

    /* U+040C "Ќ" */
    0x19, 0x80, 0x0, 0x8e, 0x4a, 0x28, 0xc3, 0xa,
    0x24, 0x8a, 0x10,

    /* U+040D "Ѝ" */
    0x60, 0x60, 0x21, 0x8e, 0x59, 0x69, 0xa7, 0x1c,
    0x61, 0x84,

    /* U+040E "Ў" */
    0x85, 0x11, 0xe0, 0x8, 0x30, 0x91, 0x24, 0x28,
    0x60, 0x41, 0x2, 0xc, 0x0,

    /* U+040F "Џ" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0xfc, 0x40, 0x80,

    /* U+0410 "А" */
    0x31, 0x28, 0xa1, 0x87, 0xf8, 0x61, 0x86, 0x10,

    /* U+0411 "Б" */
    0xfa, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+0412 "В" */
    0xfa, 0x18, 0x61, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+0413 "Г" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x0,

    /* U+0414 "Д" */
    0xe, 0x12, 0x12, 0x12, 0x22, 0x22, 0x22, 0x42,
    0x42, 0xff, 0x81, 0x81,

    /* U+0415 "Е" */
    0xfe, 0x8, 0x20, 0xfa, 0x8, 0x20, 0x83, 0xf0,

    /* U+0416 "Ж" */
    0x93, 0x29, 0x52, 0xc3, 0x8b, 0x15, 0x4a, 0x95,
    0x24,

    /* U+0417 "З" */
    0x7a, 0x10, 0x41, 0x78, 0x20, 0x41, 0x85, 0xe0,

    /* U+0418 "И" */
    0x86, 0x39, 0x65, 0xa6, 0x9c, 0x71, 0x86, 0x10,

    /* U+0419 "Й" */
    0x48, 0xc0, 0x21, 0x8e, 0x59, 0x69, 0xa7, 0x1c,
    0x61, 0x84,

    /* U+041A "К" */
    0x8e, 0x4a, 0x28, 0xc3, 0xa, 0x24, 0x8a, 0x10,

    /* U+041B "Л" */
    0x3c, 0x92, 0x49, 0x24, 0x92, 0x51, 0x86, 0x10,

    /* U+041C "М" */
    0x86, 0x1c, 0xf5, 0xb6, 0x18, 0x61, 0x86, 0x10,

    /* U+041D "Н" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x86, 0x10,

    /* U+041E "О" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x61, 0x85, 0xe0,

    /* U+041F "П" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x10,

    /* U+0420 "Р" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x20, 0x82, 0x0,

    /* U+0421 "С" */
    0x7a, 0x18, 0x20, 0x82, 0x8, 0x21, 0x89, 0xe0,

    /* U+0422 "Т" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20,

    /* U+0423 "У" */
    0x83, 0x9, 0x12, 0x42, 0x86, 0x4, 0x10, 0x20,
    0xc0,

    /* U+0424 "Ф" */
    0x10, 0xfa, 0x4c, 0x99, 0x32, 0x64, 0xbe, 0x10,
    0x20, 0x40,

    /* U+0425 "Х" */
    0x86, 0x24, 0x94, 0x31, 0x44, 0xa2, 0x8a, 0x10,

    /* U+0426 "Ц" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x42, 0x85,
    0xfc, 0x8, 0x10,

    /* U+0427 "Ч" */
    0x86, 0x18, 0x61, 0x86, 0x37, 0x41, 0x4, 0x10,

    /* U+0428 "Ш" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0x93,
    0xfc,

    /* U+0429 "Щ" */
    0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92,
    0x92, 0xff, 0x1, 0x1,

    /* U+042A "Ъ" */
    0xe0, 0x40, 0x81, 0x3, 0xc4, 0x48, 0x91, 0x22,
    0x78,

    /* U+042B "Ы" */
    0x86, 0x18, 0x61, 0xe6, 0x59, 0x65, 0x97, 0x90,

    /* U+042C "Ь" */
    0x82, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x87, 0xe0,

    /* U+042D "Э" */
    0x72, 0x20, 0x41, 0x7c, 0x10, 0x41, 0x89, 0xc0,

    /* U+042E "Ю" */
    0x9a, 0x9a, 0x69, 0xe6, 0x9a, 0x69, 0xa6, 0x60,

    /* U+042F "Я" */
    0x7e, 0x18, 0x61, 0x7c, 0x94, 0x61, 0x86, 0x10,

    /* U+0430 "а" */
    0x7a, 0x10, 0x5f, 0x86, 0x18, 0xdd,

    /* U+0431 "б" */
    0x4, 0xe4, 0x20, 0xfa, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+0432 "в" */
    0xfa, 0x18, 0xbe, 0x86, 0x18, 0x7e,

    /* U+0433 "г" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x20,

    /* U+0434 "д" */
    0x3c, 0x49, 0x14, 0x28, 0x50, 0xa1, 0x7f, 0x80,

    /* U+0435 "е" */
    0x7a, 0x18, 0x7f, 0x82, 0x8, 0x5e,

    /* U+0436 "ж" */
    0x93, 0x29, 0x51, 0xc5, 0x8a, 0xa5, 0x49,

    /* U+0437 "з" */
    0x7a, 0x10, 0x9e, 0x8, 0x18, 0x9e,

    /* U+0438 "и" */
    0x8e, 0x59, 0x69, 0xa7, 0x1c, 0x71,

    /* U+0439 "й" */
    0x48, 0xc0, 0x0, 0x8e, 0x59, 0x69, 0xa7, 0x1c,
    0x71,

    /* U+043A "к" */
    0x8e, 0x4a, 0x30, 0xa2, 0x48, 0xa1,

    /* U+043B "л" */
    0x3c, 0x92, 0x49, 0x25, 0x18, 0x61,

    /* U+043C "м" */
    0x87, 0x3d, 0x6d, 0x86, 0x18, 0x61,

    /* U+043D "н" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x61,

    /* U+043E "о" */
    0x7a, 0x18, 0x61, 0x86, 0x18, 0x5e,

    /* U+043F "п" */
    0xfe, 0x18, 0x61, 0x86, 0x18, 0x61,

    /* U+0440 "р" */
    0xbb, 0x18, 0x61, 0x86, 0x1c, 0x6e, 0x82, 0x0,

    /* U+0441 "с" */
    0x7a, 0x18, 0x20, 0x82, 0x8, 0x5e,

    /* U+0442 "т" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8,

    /* U+0443 "у" */
    0x86, 0x24, 0x94, 0x30, 0x82, 0x8, 0x43, 0x0,

    /* U+0444 "ф" */
    0x10, 0x20, 0x43, 0xe9, 0x32, 0x64, 0xc9, 0x93,
    0x25, 0xf0, 0x81, 0x0,

    /* U+0445 "х" */
    0x86, 0x24, 0x8c, 0x51, 0x28, 0xa1,

    /* U+0446 "ц" */
    0x85, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x7f, 0x2,
    0x4,

    /* U+0447 "ч" */
    0x86, 0x18, 0x61, 0x8d, 0xd0, 0x41,

    /* U+0448 "ш" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xff,

    /* U+0449 "щ" */
    0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xff,
    0x1, 0x1,

    /* U+044A "ъ" */
    0xe0, 0x40, 0x81, 0xe2, 0x24, 0x48, 0x9e,

    /* U+044B "ы" */
    0x86, 0x18, 0x79, 0x96, 0x59, 0x79,

    /* U+044C "ь" */
    0x82, 0x8, 0x3e, 0x86, 0x18, 0x7e,

    /* U+044D "э" */
    0x72, 0x20, 0x5f, 0x4, 0x18, 0x9c,

    /* U+044E "ю" */
    0x9a, 0x9a, 0x79, 0xa6, 0x9a, 0x66,

    /* U+044F "я" */
    0x7e, 0x18, 0x61, 0x7c, 0x94, 0x61,

    /* U+0450 "ѐ" */
    0x60, 0x60, 0x0, 0x7a, 0x18, 0x7f, 0x82, 0x8,
    0x5e,

    /* U+0451 "ё" */
    0x48, 0x0, 0x0, 0x7a, 0x18, 0x7f, 0x82, 0x8,
    0x5e,

    /* U+0452 "ђ" */
    0x41, 0xe1, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x85, 0x8, 0x10, 0xc0,

    /* U+0453 "ѓ" */
    0x19, 0x80, 0x0, 0xfe, 0x8, 0x20, 0x82, 0x8,
    0x20,

    /* U+0454 "є" */
    0x39, 0x18, 0x3e, 0x82, 0x4, 0x4e,

    /* U+0455 "ѕ" */
    0x7a, 0x18, 0x18, 0x18, 0x18, 0x9e,

    /* U+0456 "і" */
    0x20, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0457 "ї" */
    0x90, 0x0, 0xc2, 0x10, 0x84, 0x21, 0x3e,

    /* U+0458 "ј" */
    0x8, 0x0, 0x30, 0x84, 0x21, 0x8, 0x43, 0x26,
    0x0,

    /* U+0459 "љ" */
    0x78, 0x48, 0x48, 0x4e, 0x49, 0x49, 0x49, 0x8e,

    /* U+045A "њ" */
    0x91, 0x22, 0x47, 0xe9, 0x32, 0x64, 0xce,

    /* U+045B "ћ" */
    0x41, 0xe1, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x42,
    0x85, 0x8,

    /* U+045C "ќ" */
    0x19, 0x80, 0x0, 0x8e, 0x4a, 0x30, 0xa2, 0x48,
    0xa1,

    /* U+045D "ѝ" */
    0x60, 0x60, 0x0, 0x8e, 0x59, 0x69, 0xa7, 0x1c,
    0x71,

    /* U+045E "ў" */
    0x86, 0x27, 0x80, 0x2, 0x18, 0x92, 0x50, 0xc2,
    0x8, 0x21, 0xc, 0x0,

    /* U+045F "џ" */
    0x8c, 0x63, 0x18, 0xc6, 0x3f, 0x21, 0x0,

    /* U+0460 "Ѡ" */
    0x93, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xc9, 0xb4,
    0x90,

    /* U+0461 "ѡ" */
    0x93, 0x26, 0x4c, 0x99, 0x36, 0x92, 0x0,

    /* U+0462 "Ѣ" */
    0x21, 0xf0, 0x81, 0x3, 0xc4, 0x48, 0x91, 0x22,
    0x78,

    /* U+0463 "ѣ" */
    0x41, 0xf, 0x10, 0x41, 0xe4, 0x51, 0x45, 0xe0,

    /* U+0464 "Ѥ" */
    0x8d, 0x26, 0x85, 0xf, 0xd4, 0x28, 0x50, 0x93,
    0x18,

    /* U+0465 "ѥ" */
    0x8d, 0x26, 0x87, 0xea, 0x14, 0x24, 0xc6,

    /* U+0466 "Ѧ" */
    0x10, 0x60, 0xa2, 0x44, 0x4d, 0x95, 0x49, 0x93,
    0x24,

    /* U+0467 "ѧ" */
    0x10, 0x51, 0x22, 0x26, 0xd2, 0x64, 0xc9,

    /* U+0468 "Ѩ" */
    0x91, 0x62, 0xa5, 0xa, 0x5d, 0xa5, 0x49, 0x93,
    0x24,

    /* U+0469 "ѩ" */
    0x91, 0x52, 0x85, 0x2e, 0xd2, 0x64, 0xc9,

    /* U+046A "Ѫ" */
    0x7c, 0x89, 0x11, 0x41, 0x7, 0x15, 0x49, 0x93,
    0x24,

    /* U+046B "ѫ" */
    0x7c, 0x88, 0xa0, 0x87, 0xd2, 0x64, 0xc9,

    /* U+046C "Ѭ" */
    0xbf, 0x46, 0x8c, 0xae, 0x93, 0xaa, 0xd5, 0xab,
    0x54,

    /* U+046D "ѭ" */
    0xbf, 0x46, 0x57, 0x49, 0xd5, 0x6a, 0xd5,

    /* U+046E "Ѯ" */
    0x48, 0xc7, 0xa1, 0x4, 0x17, 0x82, 0x4, 0x10,
    0x5e, 0x82, 0x0,

    /* U+046F "ѯ" */
    0x48, 0xc0, 0x1e, 0x84, 0x27, 0x82, 0x4, 0x27,
    0xa0, 0x80,

    /* U+0470 "Ѱ" */
    0x93, 0x26, 0x4c, 0x99, 0x2f, 0x84, 0x8, 0x10,
    0x20,

    /* U+0471 "ѱ" */
    0x93, 0x26, 0x4c, 0x99, 0x2f, 0x84, 0x8,

    /* U+0472 "Ѳ" */
    0x7a, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x85, 0xe0,

    /* U+0473 "ѳ" */
    0x7a, 0x18, 0x7f, 0x86, 0x18, 0x5e,

    /* U+0474 "Ѵ" */
    0x87, 0x12, 0x24, 0x45, 0xa, 0x14, 0x10, 0x20,
    0x40,

    /* U+0475 "ѵ" */
    0x8e, 0x49, 0x14, 0x51, 0x46, 0x8,

    /* U+0476 "Ѷ" */
    0xcc, 0x44, 0x0, 0x8, 0x71, 0x22, 0x44, 0x50,
    0xa1, 0x41, 0x2, 0x4, 0x0,

    /* U+0477 "ѷ" */
    0xcc, 0x44, 0x0, 0x8, 0xd2, 0x24, 0x28, 0x50,
    0xa1, 0x81, 0x0,

    /* U+0478 "Ѹ" */
    0x41, 0x42, 0xcd, 0x9b, 0x35, 0x6b, 0x54, 0xa8,
    0x90, 0x40, 0x80,

    /* U+0479 "ѹ" */
    0x53, 0x66, 0xcd, 0x5a, 0xd5, 0x2a, 0x24, 0x10,
    0x20,

    /* U+047A "Ѻ" */
    0x10, 0xfa, 0x4c, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x25, 0xf0, 0x80,

    /* U+047B "ѻ" */
    0x10, 0xfa, 0x4c, 0x18, 0x30, 0x60, 0xc9, 0x7c,
    0x20,

    /* U+047C "Ѽ" */
    0x79, 0xc, 0xc0, 0x41, 0x0, 0x1b, 0x41, 0x83,
    0x26, 0x4c, 0x99, 0x2d, 0x80,

    /* U+047D "ѽ" */
    0x79, 0xc, 0xc0, 0x41, 0x0, 0x1b, 0x41, 0x93,
    0x69, 0xb0,

    /* U+047E "Ѿ" */
    0xfe, 0x20, 0x4, 0x99, 0x32, 0x64, 0xc9, 0x93,
    0x26, 0x4d, 0xa4, 0x80,

    /* U+047F "ѿ" */
    0xfe, 0x20, 0x4, 0x99, 0x32, 0x64, 0xc9, 0xb4,
    0x90,

    /* U+0480 "Ҁ" */
    0x7a, 0x18, 0x20, 0x82, 0x8, 0x20, 0x78, 0x20,
    0x82,

    /* U+0481 "ҁ" */
    0x7a, 0x18, 0x20, 0x82, 0x7, 0x82, 0x8, 0x20,

    /* U+0482 "҂" */
    0x24, 0x32, 0x22, 0xa2, 0x26, 0x12, 0x0,

    /* U+0483 "҃" */
    0x1f, 0x80,

    /* U+0484 "҄" */
    0x11, 0x51, 0x0,

    /* U+0485 "҅" */
    0x78, 0x60,

    /* U+0486 "҆" */
    0xe1, 0x60,

    /* U+0487 "҇" */
    0x30, 0x4c, 0x83,

    /* U+0488 "҈" */
    0x1, 0x0, 0xa, 0x1, 0x1, 0xa, 0xa, 0x0,
    0x0, 0x0, 0x4, 0x0, 0xa8, 0x5, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x4, 0x4, 0x29, 0x28, 0xa,
    0x0,

    /* U+0489 "҉" */
    0x1, 0x0, 0x84, 0x21, 0x31, 0x6, 0x18, 0x0,
    0x0, 0x0, 0x0, 0x1, 0x38, 0x7, 0x20, 0x0,
    0x0, 0x0, 0x0, 0x6, 0x6c, 0x11, 0x10, 0x84,
    0x20,

    /* U+048A "Ҋ" */
    0x48, 0x60, 0x4, 0x28, 0xd2, 0xa5, 0x52, 0xa5,
    0x8b, 0x14, 0x28, 0x60, 0x82, 0x0,

    /* U+048B "ҋ" */
    0x48, 0x60, 0x0, 0x8, 0xd2, 0xa5, 0x52, 0xa5,
    0x8b, 0x16, 0x30, 0x41, 0x0,

    /* U+048C "Ҍ" */
    0x43, 0x84, 0x10, 0x41, 0xe4, 0x51, 0x4d, 0xe0,

    /* U+048D "ҍ" */
    0x43, 0x84, 0x1e, 0x45, 0x27, 0x80,

    /* U+048E "Ҏ" */
    0xfa, 0x19, 0xa2, 0xf6, 0x8, 0x20, 0x82, 0x0,

    /* U+048F "ҏ" */
    0xbb, 0x18, 0x61, 0x86, 0x5c, 0xad, 0x82, 0x0,

    /* U+0490 "Ґ" */
    0x4, 0x1f, 0xe0, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x20,

    /* U+0491 "ґ" */
    0x4, 0x1f, 0xe0, 0x82, 0x8, 0x20, 0x82, 0x0,

    /* U+0492 "Ғ" */
    0x3e, 0x40, 0x81, 0x2, 0x1f, 0x8, 0x10, 0x20,
    0x40,

    /* U+0493 "ғ" */
    0x3e, 0x40, 0x81, 0xf, 0x84, 0x8, 0x10,

    /* U+0494 "Ҕ" */
    0xfe, 0x8, 0x20, 0x83, 0xe8, 0x61, 0x86, 0x11,
    0x80,

    /* U+0495 "ҕ" */
    0xfe, 0x8, 0x20, 0xfa, 0x18, 0x61, 0x18,

    /* U+0496 "Җ" */
    0x93, 0x29, 0x52, 0xc3, 0x8b, 0x15, 0x4a, 0x93,
    0x24, 0x8, 0x10,

    /* U+0497 "җ" */
    0x93, 0x29, 0x51, 0xc5, 0x8a, 0xa4, 0xc9, 0x2,
    0x4,

    /* U+0498 "Ҙ" */
    0x7a, 0x10, 0x41, 0x78, 0x20, 0x41, 0x85, 0xe1,
    0x18,

    /* U+0499 "ҙ" */
    0x7a, 0x10, 0x9e, 0x8, 0x18, 0x9e, 0x11, 0x80,

    /* U+049A "Қ" */
    0x8e, 0x4a, 0x28, 0xc3, 0xa, 0x24, 0x8a, 0x10,
    0x41,

    /* U+049B "қ" */
    0x8e, 0x4a, 0x30, 0xa2, 0x48, 0xa1, 0x4, 0x10,

    /* U+049C "Ҝ" */
    0x87, 0x12, 0xa5, 0x4f, 0x15, 0x2a, 0x54, 0x85,
    0x4,

    /* U+049D "ҝ" */
    0x87, 0x52, 0xc7, 0x8a, 0x95, 0x21, 0x41,

    /* U+049E "Ҟ" */
    0x47, 0xd1, 0x42, 0x86, 0xc, 0x14, 0x24, 0x44,
    0x84,

    /* U+049F "ҟ" */
    0x47, 0xd1, 0x43, 0x5, 0x9, 0x11, 0x21,

    /* U+04A0 "Ҡ" */
    0xe3, 0x24, 0x28, 0x28, 0x30, 0x30, 0x28, 0x24,
    0x22, 0x21,

    /* U+04A1 "ҡ" */
    0xe3, 0x24, 0x28, 0x30, 0x28, 0x24, 0x22, 0x21,

    /* U+04A2 "Ң" */
    0x85, 0xa, 0x14, 0x2f, 0xd0, 0xa1, 0x42, 0x85,
    0xc, 0x8, 0x10,

    /* U+04A3 "ң" */
    0x85, 0xa, 0x17, 0xe8, 0x50, 0xa1, 0x43, 0x2,
    0x4,

    /* U+04A4 "Ҥ" */
    0x8f, 0x12, 0x24, 0x4f, 0x91, 0x22, 0x44, 0x89,
    0x10,

    /* U+04A5 "ҥ" */
    0x8f, 0x12, 0x27, 0xc8, 0x91, 0x22, 0x44,

    /* U+04A6 "Ҧ" */
    0xf1, 0x22, 0x44, 0x89, 0x13, 0xa4, 0xc9, 0x93,
    0x24, 0x8, 0x60,

    /* U+04A7 "ҧ" */
    0xf1, 0x22, 0x44, 0x89, 0xd2, 0x64, 0xc9, 0x2,
    0x18,

    /* U+04A8 "Ҩ" */
    0x42, 0x6a, 0x69, 0xa6, 0x9a, 0x69, 0xa5, 0x63,
    0x3,

    /* U+04A9 "ҩ" */
    0x42, 0x6a, 0x69, 0xa6, 0x95, 0x8c, 0xc,

    /* U+04AA "Ҫ" */
    0x7a, 0x18, 0x20, 0x82, 0x8, 0x21, 0x89, 0xe1,
    0x18,

    /* U+04AB "ҫ" */
    0x7a, 0x18, 0x20, 0x82, 0x18, 0x9e, 0x11, 0x80,

    /* U+04AC "Ҭ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x30, 0x20, 0x40,

    /* U+04AD "ҭ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0xc, 0x8,
    0x10,

    /* U+04AE "Ү" */
    0x83, 0x9, 0x12, 0x42, 0x82, 0x4, 0x8, 0x10,
    0x20,

    /* U+04AF "ү" */
    0x8c, 0x62, 0xa6, 0x10, 0x84, 0x21, 0x0,

    /* U+04B0 "Ұ" */
    0x83, 0x9, 0x12, 0x42, 0x82, 0x4, 0x3e, 0x10,
    0x20,

    /* U+04B1 "ұ" */
    0x8c, 0x62, 0xa6, 0x13, 0xe4, 0x21, 0x0,

    /* U+04B2 "Ҳ" */
    0x85, 0x11, 0x22, 0x83, 0xa, 0x12, 0x44, 0x89,
    0xc, 0x8, 0x10,

    /* U+04B3 "ҳ" */
    0x85, 0x11, 0x21, 0x85, 0x9, 0x22, 0x43, 0x2,
    0x4,

    /* U+04B4 "Ҵ" */
    0xfa, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x22, 0x3f, 0x1, 0x1,

    /* U+04B5 "ҵ" */
    0xfa, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x3f,
    0x1, 0x1,

    /* U+04B6 "Ҷ" */
    0x85, 0xa, 0x14, 0x28, 0x51, 0x9d, 0x2, 0x4,
    0xc, 0x8, 0x10,

    /* U+04B7 "ҷ" */
    0x85, 0xa, 0x14, 0x28, 0xce, 0x81, 0x3, 0x2,
    0x4,

    /* U+04B8 "Ҹ" */
    0x86, 0x18, 0x61, 0xa6, 0xb7, 0x49, 0x24, 0x10,

    /* U+04B9 "ҹ" */
    0x86, 0x18, 0x69, 0xad, 0xd2, 0x41,

    /* U+04BA "Һ" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x10,

    /* U+04BB "һ" */
    0x82, 0xb, 0xb1, 0x86, 0x18, 0x61,

    /* U+04BC "Ҽ" */
    0x9a, 0x9a, 0x69, 0x7c, 0x82, 0x9, 0x28, 0x60,

    /* U+04BD "ҽ" */
    0x9a, 0x9a, 0x5f, 0x20, 0x82, 0x46,

    /* U+04BE "Ҿ" */
    0x9a, 0x9a, 0x69, 0x7c, 0x82, 0x9, 0x28, 0x61,
    0x4,

    /* U+04BF "ҿ" */
    0x9a, 0x9a, 0x5f, 0x20, 0x82, 0x46, 0x10, 0x40,

    /* U+04C0 "Ӏ" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+04C1 "Ӂ" */
    0x83, 0x9, 0xf0, 0x9, 0x32, 0x95, 0x2c, 0x38,
    0xb1, 0x54, 0xa9, 0x52, 0x40,

    /* U+04C2 "ӂ" */
    0x44, 0x90, 0xe0, 0x0, 0x12, 0x65, 0x2a, 0x38,
    0xb1, 0x54, 0xa9, 0x20,

    /* U+04C3 "Ӄ" */
    0x8e, 0x4a, 0x28, 0xc3, 0xe8, 0x61, 0x86, 0x10,
    0x46,

    /* U+04C4 "ӄ" */
    0x8e, 0x4a, 0x30, 0xfa, 0x18, 0x61, 0x4, 0x60,

    /* U+04C5 "Ӆ" */
    0x3c, 0x48, 0x91, 0x22, 0x44, 0x89, 0x22, 0x85,
    0xc, 0x10, 0x40,

    /* U+04C6 "ӆ" */
    0x3c, 0x48, 0x91, 0x22, 0x48, 0xa1, 0x43, 0x4,
    0x10,

    /* U+04C7 "Ӈ" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x86, 0x10,
    0x46,

    /* U+04C8 "ӈ" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x61, 0x4, 0x60,

    /* U+04C9 "Ӊ" */
    0x85, 0xa, 0x14, 0x2f, 0xd0, 0xa1, 0x42, 0x85,
    0xc, 0x10, 0x40,

    /* U+04CA "ӊ" */
    0x85, 0xa, 0x17, 0xe8, 0x50, 0xa1, 0x43, 0x4,
    0x10,

    /* U+04CB "Ӌ" */
    0x86, 0x18, 0x61, 0x86, 0x37, 0x41, 0x4, 0x30,
    0x82,

    /* U+04CC "ӌ" */
    0x86, 0x18, 0x61, 0x8d, 0xd0, 0x43, 0x8, 0x20,

    /* U+04CD "Ӎ" */
    0x85, 0xb, 0x36, 0xab, 0x50, 0xa1, 0x42, 0x85,
    0xc, 0x10, 0x40,

    /* U+04CE "ӎ" */
    0x85, 0x9b, 0x55, 0xa8, 0x50, 0xa1, 0x43, 0x4,
    0x10,

    /* U+04CF "ӏ" */
    0xe9, 0x24, 0x92, 0x5c,

    /* U+04D0 "Ӑ" */
    0x86, 0x27, 0x80, 0x31, 0x28, 0xa1, 0x87, 0xf8,
    0x61, 0x86, 0x10,

    /* U+04D1 "ӑ" */
    0x86, 0x27, 0x80, 0x1, 0xe8, 0x41, 0x7e, 0x18,
    0x63, 0x74,

    /* U+04D2 "Ӓ" */
    0x48, 0x0, 0x0, 0x31, 0x28, 0xa1, 0x87, 0xf8,
    0x61, 0x86, 0x10,

    /* U+04D3 "ӓ" */
    0x48, 0x0, 0x0, 0x7a, 0x10, 0x5f, 0x86, 0x18,
    0xdd,

    /* U+04D4 "Ӕ" */
    0x3e, 0xa2, 0x44, 0x8f, 0xf2, 0x24, 0x48, 0x91,
    0x3c,

    /* U+04D5 "ӕ" */
    0x7d, 0x24, 0x4b, 0xf9, 0x12, 0x24, 0xbe,

    /* U+04D6 "Ӗ" */
    0x86, 0x27, 0x80, 0xfe, 0x8, 0x20, 0xfa, 0x8,
    0x20, 0x83, 0xf0,

    /* U+04D7 "ӗ" */
    0x86, 0x27, 0x80, 0x7a, 0x18, 0x7f, 0x82, 0x8,
    0x5e,

    /* U+04D8 "Ә" */
    0x31, 0x28, 0x41, 0x7, 0xf8, 0x61, 0x48, 0xc0,

    /* U+04D9 "ә" */
    0x7a, 0x10, 0x41, 0xfe, 0x18, 0x5e,

    /* U+04DA "Ӛ" */
    0x48, 0x0, 0x0, 0x31, 0x28, 0x41, 0x7, 0xf8,
    0x61, 0x48, 0xc0,

    /* U+04DB "ӛ" */
    0x48, 0x0, 0x0, 0x7a, 0x10, 0x41, 0xfe, 0x18,
    0x5e,

    /* U+04DC "Ӝ" */
    0x48, 0x0, 0x0, 0x9, 0x32, 0x95, 0x2c, 0x38,
    0xb1, 0x54, 0xa9, 0x52, 0x40,

    /* U+04DD "ӝ" */
    0x48, 0x0, 0x0, 0x9, 0x32, 0x95, 0x1c, 0x58,
    0xaa, 0x54, 0x90,

    /* U+04DE "Ӟ" */
    0x48, 0x0, 0x0, 0x7a, 0x10, 0x41, 0x78, 0x20,
    0x41, 0x85, 0xe0,

    /* U+04DF "ӟ" */
    0x48, 0x0, 0x0, 0x7a, 0x10, 0x9e, 0x8, 0x18,
    0x9e,

    /* U+04E0 "Ӡ" */
    0xfc, 0x21, 0x8, 0x38, 0x10, 0x41, 0x8d, 0xe0,

    /* U+04E1 "ӡ" */
    0x7c, 0x21, 0x8, 0x38, 0x10, 0x41, 0x85, 0xe0,

    /* U+04E2 "Ӣ" */
    0x78, 0x0, 0x21, 0x8e, 0x59, 0x69, 0xa7, 0x1c,
    0x61, 0x84,

    /* U+04E3 "ӣ" */
    0x78, 0x0, 0x23, 0x96, 0x5a, 0x69, 0xc7, 0x1c,
    0x40,

    /* U+04E4 "Ӥ" */
    0x48, 0x0, 0x0, 0x86, 0x39, 0x65, 0xa6, 0x9c,
    0x71, 0x86, 0x10,

    /* U+04E5 "ӥ" */
    0x48, 0x0, 0x0, 0x8e, 0x59, 0x69, 0xa7, 0x1c,
    0x71,

    /* U+04E6 "Ӧ" */
    0x48, 0x0, 0x0, 0x7a, 0x18, 0x61, 0x86, 0x18,
    0x61, 0x85, 0xe0,

    /* U+04E7 "ӧ" */
    0x48, 0x0, 0x0, 0x7a, 0x18, 0x61, 0x86, 0x18,
    0x5e,

    /* U+04E8 "Ө" */
    0x7a, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x85, 0xe0,

    /* U+04E9 "ө" */
    0x7a, 0x18, 0x7f, 0x86, 0x18, 0x5e,

    /* U+04EA "Ӫ" */
    0x48, 0x0, 0x0, 0x7a, 0x18, 0x61, 0xfe, 0x18,
    0x61, 0x85, 0xe0,

    /* U+04EB "ӫ" */
    0x48, 0x0, 0x0, 0x7a, 0x18, 0x7f, 0x86, 0x18,
    0x5e,

    /* U+04EC "Ӭ" */
    0x48, 0x7, 0xa1, 0x4, 0x17, 0xc1, 0x4, 0x18,
    0x5e,

    /* U+04ED "ӭ" */
    0x48, 0x7, 0xa1, 0x4, 0xf0, 0x41, 0x85, 0xe0,

    /* U+04EE "Ӯ" */
    0x7c, 0x0, 0x4, 0x18, 0x48, 0x92, 0x14, 0x30,
    0x20, 0x81, 0x6, 0x0,

    /* U+04EF "ӯ" */
    0x78, 0x0, 0x21, 0x89, 0x25, 0xc, 0x20, 0x82,
    0x10, 0xc0,

    /* U+04F0 "Ӱ" */
    0x48, 0x0, 0x0, 0x8, 0x30, 0x91, 0x24, 0x28,
    0x60, 0x41, 0x2, 0xc, 0x0,

    /* U+04F1 "ӱ" */
    0x48, 0x0, 0x0, 0x86, 0x24, 0x94, 0x30, 0x82,
    0x8, 0x43, 0x0,

    /* U+04F2 "Ӳ" */
    0x67, 0x10, 0x0, 0x8, 0x30, 0x91, 0x24, 0x28,
    0x60, 0x41, 0x2, 0xc, 0x0,

    /* U+04F3 "ӳ" */
    0x67, 0x10, 0x0, 0x8, 0x51, 0x12, 0x28, 0x30,
    0x40, 0x81, 0x4, 0x18, 0x0,

    /* U+04F4 "Ӵ" */
    0x48, 0x0, 0x0, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x41, 0x4, 0x10,

    /* U+04F5 "ӵ" */
    0x48, 0x0, 0x0, 0x86, 0x18, 0x61, 0x8d, 0xd0,
    0x41,

    /* U+04F6 "Ӷ" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x20, 0x83, 0x4,
    0x10,

    /* U+04F7 "ӷ" */
    0xfe, 0x8, 0x20, 0x82, 0x8, 0x30, 0x41, 0x0,

    /* U+04F8 "Ӹ" */
    0x48, 0x0, 0x0, 0x86, 0x18, 0x61, 0xe6, 0x59,
    0x65, 0x97, 0x90,

    /* U+04F9 "ӹ" */
    0x48, 0x0, 0x0, 0x86, 0x18, 0x79, 0x96, 0x59,
    0x79,

    /* U+04FA "Ӻ" */
    0x3e, 0x40, 0x81, 0x2, 0x1f, 0x8, 0x10, 0x20,
    0x60, 0x41, 0x80,

    /* U+04FB "ӻ" */
    0x3e, 0x40, 0x87, 0xc2, 0x4, 0x8, 0x18, 0x10,
    0x60,

    /* U+04FC "Ӽ" */
    0x86, 0x24, 0x94, 0x31, 0x44, 0xa2, 0x86, 0x10,
    0x42,

    /* U+04FD "ӽ" */
    0x86, 0x24, 0x8c, 0x51, 0x28, 0x61, 0x4, 0x20,

    /* U+04FE "Ӿ" */
    0x86, 0x24, 0x94, 0x33, 0x34, 0x92, 0x4a, 0x10,

    /* U+04FF "ӿ" */
    0x86, 0x24, 0x8c, 0xcd, 0x28, 0xa1,

    /* U+0500 "Ԁ" */
    0x4, 0x10, 0x5f, 0x86, 0x18, 0x61, 0x86, 0x37,
    0x40,

    /* U+0501 "ԁ" */
    0x4, 0x10, 0x41, 0x76, 0x38, 0x61, 0x86, 0x37,
    0x40,

    /* U+0502 "Ԃ" */
    0x8, 0x10, 0x23, 0xc8, 0x91, 0x22, 0xc5, 0x8b,
    0x35, 0xb0,

    /* U+0503 "ԃ" */
    0x8, 0x10, 0x20, 0x46, 0x93, 0x22, 0x44, 0x8b,
    0x39, 0xb0,

    /* U+0504 "Ԅ" */
    0x71, 0x10, 0x20, 0x43, 0x1, 0x42, 0x85, 0xa,
    0x8,

    /* U+0505 "ԅ" */
    0x71, 0x10, 0x20, 0x43, 0x1, 0x43, 0x2,

    /* U+0506 "Ԇ" */
    0x71, 0x10, 0x20, 0x43, 0x1, 0x2, 0x4, 0x8,
    0xc, 0x8, 0x10,

    /* U+0507 "ԇ" */
    0x71, 0x10, 0x20, 0x43, 0x1, 0x2, 0x3, 0x2,
    0x4,

    /* U+0508 "Ԉ" */
    0x3c, 0x24, 0x24, 0x24, 0x24, 0x25, 0x25, 0x45,
    0x85, 0x82,

    /* U+0509 "ԉ" */
    0x3c, 0x24, 0x24, 0x24, 0x24, 0x45, 0x86, 0x82,

    /* U+050A "Ԋ" */
    0x89, 0x12, 0x24, 0x4f, 0x91, 0x62, 0xc5, 0x8b,
    0x18,

    /* U+050B "ԋ" */
    0x89, 0x12, 0x27, 0xc8, 0x91, 0x63, 0x46,

    /* U+050C "Ԍ" */
    0x7a, 0x18, 0x20, 0x82, 0x78, 0x61, 0x89, 0xc0,

    /* U+050D "ԍ" */
    0x7a, 0x18, 0x20, 0x9e, 0x18, 0x5e,

    /* U+050E "Ԏ" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x44, 0x89, 0x12,
    0x38,

    /* U+050F "ԏ" */
    0xfe, 0x20, 0x40, 0x81, 0x22, 0x44, 0x8e,

    /* U+0510 "Ԑ" */
    0x7a, 0x18, 0x20, 0x79, 0x8, 0x20, 0x85, 0xe0,

    /* U+0511 "ԑ" */
    0x7a, 0x18, 0x1e, 0x42, 0x8, 0x5e,

    /* U+0512 "Ԓ" */
    0x3c, 0x92, 0x49, 0x24, 0x92, 0x51, 0x86, 0x11,
    0x46,

    /* U+0513 "ԓ" */
    0x3c, 0x92, 0x49, 0x25, 0x18, 0x61, 0x14, 0x60,

    /* U+0514 "Ԕ" */
    0x71, 0x52, 0x4a, 0x4c, 0x44, 0x4c, 0x4a, 0x52,
    0x52, 0x91,

    /* U+0515 "ԕ" */
    0x71, 0x52, 0x4a, 0x44, 0x4c, 0x4a, 0x52, 0x91,

    /* U+0516 "Ԗ" */
    0xe1, 0x92, 0x94, 0x98, 0x90, 0xf0, 0x88, 0x84,
    0x82, 0x81,

    /* U+0517 "ԗ" */
    0x1, 0xe2, 0x94, 0x98, 0x98, 0x94, 0xe2, 0x81,
    0x80, 0x80,

    /* U+0518 "Ԙ" */
    0x7f, 0x88, 0x88, 0x88, 0x7f, 0x28, 0x48, 0x88,
    0x88, 0x8f,

    /* U+0519 "ԙ" */
    0x7e, 0x89, 0x9, 0x7f, 0x28, 0x28, 0x29, 0x46,

    /* U+051A "Ԛ" */
    0x79, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x5a, 0xcc,
    0xf0, 0x18,

    /* U+051B "ԛ" */
    0x76, 0x38, 0x61, 0x86, 0x18, 0xdd, 0x4, 0x10,

    /* U+051C "Ԝ" */
    0x86, 0x18, 0x61, 0xb7, 0x5c, 0xe1, 0x86, 0x10,

    /* U+051D "ԝ" */
    0x83, 0x26, 0x4c, 0x99, 0x32, 0x64, 0xb6,

    /* U+051E "Ԟ" */
    0x96, 0x29, 0x68, 0xc3, 0xa, 0x24, 0x8a, 0x10,

    /* U+051F "ԟ" */
    0xaa, 0x4a, 0xb0, 0xa2, 0x48, 0xa1,

    /* U+0520 "Ԡ" */
    0x70, 0x50, 0x50, 0x50, 0x5e, 0x51, 0x51, 0x91,
    0x91, 0x91, 0x5, 0x2,

    /* U+0521 "ԡ" */
    0x70, 0x50, 0x50, 0x5e, 0x51, 0x51, 0x51, 0x91,
    0x5, 0x2,

    /* U+0522 "Ԣ" */
    0x90, 0x90, 0x90, 0x90, 0xfe, 0x91, 0x91, 0x91,
    0x91, 0x91, 0x5, 0x2,

    /* U+0523 "ԣ" */
    0x90, 0x90, 0x90, 0xfe, 0x91, 0x91, 0x91, 0x91,
    0x5, 0x2,

    /* U+0524 "Ԥ" */
    0xfa, 0x28, 0xa2, 0x8a, 0x28, 0xa2, 0x8a, 0x30,
    0x41,

    /* U+0525 "ԥ" */
    0xfa, 0x28, 0xa2, 0x8a, 0x28, 0xa3, 0x4, 0x10,

    /* U+0526 "Ԧ" */
    0x81, 0x2, 0x5, 0xcc, 0x50, 0xa1, 0x42, 0x85,
    0xc, 0x8, 0x10,

    /* U+0527 "ԧ" */
    0x81, 0x2, 0xe6, 0x28, 0x50, 0xa1, 0x43, 0x2,
    0x4,

    /* U+0528 "Ԩ" */
    0x22, 0x44, 0x89, 0x13, 0xe4, 0x48, 0x91, 0x22,
    0x46, 0x82, 0x0,

    /* U+0529 "ԩ" */
    0x22, 0x44, 0x89, 0xf2, 0x24, 0x48, 0x91, 0xa0,
    0x80,

    /* U+052A "Ԫ" */
    0xc, 0x46, 0x28, 0x90, 0x51, 0x40, 0xa2, 0x82,
    0x7e, 0x4, 0x8c, 0x9, 0x14, 0x22, 0x24, 0x44,
    0x45, 0xfc, 0x86, 0x8, 0x4, 0x10, 0x0,

    /* U+052B "ԫ" */
    0x38, 0x8c, 0xa2, 0x44, 0x8a, 0x23, 0xf0, 0x88,
    0xa2, 0x22, 0x48, 0x88, 0xbf, 0x21, 0x80, 0x0,

    /* U+052C "Ԭ" */
    0x9a, 0x2a, 0xaa, 0xab, 0xe4, 0x92, 0x8b, 0xf8,
    0x61,

    /* U+052D "ԭ" */
    0x9a, 0xaa, 0xaa, 0xf9, 0x24, 0x92, 0x7a, 0x10,

    /* U+052E "Ԯ" */
    0x3c, 0x48, 0x91, 0x22, 0x44, 0x89, 0x22, 0x85,
    0xc, 0x8, 0x10,

    /* U+052F "ԯ" */
    0x3c, 0x48, 0x91, 0x22, 0x48, 0xa1, 0x43, 0x2,
    0x4
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 128, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 154, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 128, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 198, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 265, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 352, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 128, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 381, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 451, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 128, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 465, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 471, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 499, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 505, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 523, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 529, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 543, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 564, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 576, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 591, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 614, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 621, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 627, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 633, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 681, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 690, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 696, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 702, .adv_w = 128, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 128, .box_w = 5, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 128, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 725, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 733, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 740, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 750, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 768, .adv_w = 128, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 780, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 787, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 803, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 812, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 845, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 852, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 861, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 868, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 877, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 893, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 900, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 911, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 921, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 930, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 937, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 945, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 951, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 960, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 966, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 979, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 990, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1001, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1010, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1021, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1030, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1053, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1065, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1074, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1083, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1091, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1098, .adv_w = 0, .box_w = 4, .box_h = 3, .ofs_x = -7, .ofs_y = 10},
    {.bitmap_index = 1100, .adv_w = 0, .box_w = 5, .box_h = 4, .ofs_x = -5, .ofs_y = 10},
    {.bitmap_index = 1103, .adv_w = 0, .box_w = 4, .box_h = 3, .ofs_x = -7, .ofs_y = 10},
    {.bitmap_index = 1105, .adv_w = 0, .box_w = 4, .box_h = 3, .ofs_x = -6, .ofs_y = 10},
    {.bitmap_index = 1107, .adv_w = 0, .box_w = 8, .box_h = 3, .ofs_x = -8, .ofs_y = 10},
    {.bitmap_index = 1110, .adv_w = 0, .box_w = 14, .box_h = 14, .ofs_x = -12, .ofs_y = -2},
    {.bitmap_index = 1135, .adv_w = 0, .box_w = 14, .box_h = 14, .ofs_x = -11, .ofs_y = -2},
    {.bitmap_index = 1160, .adv_w = 128, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1174, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1187, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1195, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1201, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1209, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1217, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1226, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1234, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1243, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1250, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1259, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1266, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1277, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1286, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1295, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1303, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1312, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1320, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1329, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1336, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1345, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1352, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1362, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1370, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1381, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1390, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1399, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1406, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1417, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1426, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1435, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1442, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1451, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1459, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1470, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1479, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1488, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1495, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1504, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1511, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1522, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1531, .adv_w = 128, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1543, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1553, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1564, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1573, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1581, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1587, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1595, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1601, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1609, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1615, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1624, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1632, .adv_w = 128, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1639, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1652, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1664, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1673, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1681, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1692, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1701, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1710, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1718, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1729, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1738, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1747, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1755, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1766, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1775, .adv_w = 128, .box_w = 3, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1779, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1790, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1800, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1811, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1820, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1829, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1836, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1847, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1856, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1864, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1870, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1881, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1890, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1903, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1914, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1925, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1934, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1942, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1950, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1960, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1969, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1980, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1989, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2000, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2009, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2017, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2023, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2034, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2043, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2052, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2060, .adv_w = 128, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2072, .adv_w = 128, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2082, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2095, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2106, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2119, .adv_w = 128, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2132, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2143, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2152, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2161, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2169, .adv_w = 128, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2180, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2189, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2200, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2209, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2218, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2226, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2234, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2240, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2249, .adv_w = 128, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2258, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2268, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2278, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2287, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2294, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2305, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2314, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2324, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2332, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2341, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2348, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2356, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2362, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2371, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2378, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2386, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2392, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2401, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2409, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2419, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2427, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2437, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2447, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2457, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2465, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2475, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2483, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2491, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2498, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2506, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2512, .adv_w = 128, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2524, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2534, .adv_w = 128, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2546, .adv_w = 128, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2556, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2565, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2573, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2584, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2593, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2604, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2613, .adv_w = 256, .box_w = 15, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2636, .adv_w = 256, .box_w = 14, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 2652, .adv_w = 128, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2661, .adv_w = 128, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2669, .adv_w = 128, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 2680, .adv_w = 128, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 1024, .range_length = 48, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1072, .range_length = 256, .glyph_id_start = 49,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t stackchan_font_cyr_16 = {
#else
lv_font_t stackchan_font_cyr_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if STACKCHAN_FONT_CYR_16*/

