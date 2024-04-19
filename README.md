# lvgl file manager
A lightweight, simple file explorer with lvgl gui, targeted for embedded systems.
# How to use
If you are using ESP-IDF framework, just clone the repo, In menuconfig, adjust spi connection according to your board then build and flash.
If you are using another framework just copy the lvgl_file_manager from components directory to your project, call ui_init from main function.
# Important
This project uses esp32 wroom with ESP-IDF framework. But you can use other platform too.
TFT and Touch shares same SPI, pins are specified in menuconfig, SD SPI pins are specified in sd_card component. On lv_conf.h, attach a file system with drive letter 'S', also enable libpng or lodepng decoder for png image support. This project uses libpng.

- display 320*240 ili9341
    - HSPI port
    - CS 15
- touch driver xpt2046
  - HSPI port
  - CS 33
  - IRQ 36
- sd card
  - VSPI port
  - CS 5
# Current features
- browse files
- text viewer
- png decoder
- create/delete files
# To-do
- all image format support
- video decoder
- more basic file handeling

In case you face any problem, raise an Issue.

