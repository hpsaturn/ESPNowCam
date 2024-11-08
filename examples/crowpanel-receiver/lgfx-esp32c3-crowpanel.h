#define LGFX_USE_V1

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_GC9A01 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
  public:
    LGFX(void)
    {
      {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST; 
        cfg.spi_mode = 0;                  
        cfg.freq_write = 80000000;         
        cfg.freq_read = 20000000;          
        cfg.spi_3wire = true;              
        cfg.use_lock = true;               
        cfg.dma_channel = SPI_DMA_CH_AUTO; 
        cfg.pin_sclk = 6;  
        cfg.pin_mosi = 7;  
        cfg.pin_miso = -1; 
        cfg.pin_dc = 2;    
        _bus_instance.config(cfg);              
        _panel_instance.setBus(&_bus_instance); 
      }
      { 
        auto cfg = _panel_instance.config();
        cfg.pin_cs = 10;   
        cfg.pin_rst = -1;  
        cfg.pin_busy = -1; 
        cfg.memory_width = 240;   
        cfg.memory_height = 240;  
        cfg.panel_width = 240;    
        cfg.panel_height = 240;   
        cfg.offset_x = 0;         
        cfg.offset_y = 0;         
        cfg.offset_rotation = 0;  
        cfg.dummy_read_pixel = 8; 
        cfg.dummy_read_bits = 1;  
        cfg.readable = false;     
        cfg.invert = true;        
        cfg.rgb_order = false;   
        cfg.dlen_16bit = false;   
        cfg.bus_shared = false;   
        _panel_instance.config(cfg);
      }
      setPanel(&_panel_instance); 
    }
};

