#ifndef LGFX_CUSTOMBOARD_CONF_HPP
#define LGFX_CUSTOMBOARD_CONF_HPP

#define LGFX_USE_V1

#include "LovyanGFX.hpp"

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7735S _panel_instance;
    lgfx::Bus_SPI _bus_instance;
   
public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = VSPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 27000000;
            cfg.freq_read =  10000000;
            cfg.spi_3wire = true;
            cfg.use_lock = false;
            //cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 18;  //  CLK
            cfg.pin_mosi = 23;  //  N/A
            cfg.pin_miso = 19;  //  N/A
            cfg.pin_dc = 27;    //  RS
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 14;    //  CS
            cfg.pin_rst = 33;   //  RST
            cfg.pin_busy = -1;
            cfg.panel_width = 128;
            cfg.panel_height = 160;
            cfg.memory_width = 128;
            cfg.memory_height = 160;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.dummy_read_pixel = 9;
            cfg.readable = true;
			cfg.invert = false;
            cfg.rgb_order = true;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

#endif
