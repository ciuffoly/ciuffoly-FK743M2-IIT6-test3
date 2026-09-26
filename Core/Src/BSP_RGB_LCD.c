#include "BSP_RGB_LCD.h"

#define XSIZE_PHYS 800
#define YSIZE_PHYS 480


#define POLY_X(Z)              ((int32_t)((Points + Z)->X))
#define POLY_Y(Z)              ((int32_t)((Points + Z)->Y))      

#define ABS(X)  ((X) > 0 ? (X) : -(X))      

extern DMA2D_HandleTypeDef hdma2d;
extern LTDC_HandleTypeDef  hltdc;
    
static uint32_t            ActiveLayer = 0;

static LCD_DrawPropTypeDef DrawProp[LTDC_MAX_LAYER_NUMBER];

void LL_FillBuffer(uint32_t LayerIndex, void *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex);
void LL_ConvertLineToARGB8888(void *pSrc, void *pDst, uint32_t xSize, uint32_t ColorMode);
void LL_ConvertLineToRGB565(void *pSrc, void *pDst, uint32_t xSize, uint32_t ColorMode);
void DrawSafeRect(uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height, uint16_t color);

//#define FRAMEBUFFER_ADDRESS (0xD0000000)
#define FRAMEBUFFER_ADDRESS (0xC0000000)


uint8_t BSP_LCD_Init(void)
{  
  BSP_LCD_SelectLayer(0);
  BSP_LCD_SetLayerVisible(0, ENABLE);
  return LCD_OK;
}

int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t Xaddress;

  if(hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565)
  {
    Xaddress = FRAMEBUFFER_ADDRESS + (2 * ((BSP_LCD_GetXSize() * Ypos) + Xpos));
  }
  else
  {
    Xaddress = FRAMEBUFFER_ADDRESS + (4 * ((BSP_LCD_GetXSize() * Ypos) + Xpos));
  }

  if((Xpos + Length) > BSP_LCD_GetXSize())
  {
    Length = BSP_LCD_GetXSize() - Xpos;
  }
  LL_FillBuffer(Instance, (uint32_t *)Xaddress, Length, 1, 0, Color);

  return LCD_OK;
}

uint32_t BSP_LCD_GetXSize(void)
{
  return hltdc.LayerCfg[ActiveLayer].ImageWidth;
}

uint32_t BSP_LCD_GetYSize(void)
{
  return hltdc.LayerCfg[ActiveLayer].ImageHeight;
}

void BSP_LCD_SelectLayer(uint32_t LayerIndex)
{
  ActiveLayer = LayerIndex;

  hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565;
} 

void BSP_LCD_SetLayerVisible(uint32_t LayerIndex, FunctionalState State)
{
  if(State == ENABLE)
  {
    __HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex);
  }
  else
  {
    __HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);
  }
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);
} 

void BSP_LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency)
{    
  HAL_LTDC_SetAlpha(&hltdc, Transparency, LayerIndex);
}

void BSP_LCD_SetLayerAddress(uint32_t LayerIndex, uint32_t Address)
{
  HAL_LTDC_SetAddress(&hltdc, Address, LayerIndex);
}

void BSP_LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  HAL_LTDC_SetWindowSize(&hltdc, Width, Height, LayerIndex);
  HAL_LTDC_SetWindowPosition(&hltdc, Xpos, Ypos, LayerIndex);
}

void BSP_LCD_SetColorKeying(uint32_t LayerIndex, uint32_t RGBValue)
{  
  HAL_LTDC_ConfigColorKeying(&hltdc, RGBValue, LayerIndex);
  HAL_LTDC_EnableColorKeying(&hltdc, LayerIndex);
}

void BSP_LCD_ResetColorKeying(uint32_t LayerIndex)
{   
  HAL_LTDC_DisableColorKeying(&hltdc, LayerIndex);
}

uint32_t BSP_LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint32_t ret = 0;
  
  if(hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
	  ret = *(__IO uint32_t*) (hltdc.LayerCfg[ActiveLayer].FBStartAdress + (4*(Ypos*BSP_LCD_GetXSize() + Xpos)));
  }
  else if(hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB888)
  {
    ret = (*(__IO uint32_t*) (hltdc.LayerCfg[ActiveLayer].FBStartAdress + (2*(Ypos*BSP_LCD_GetXSize() + Xpos))) & 0x00FFFFFF);
  }
  else if((hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) || \
          (hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB4444) || \
          (hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_AL88))
  {
    ret = *(__IO uint16_t*) (hltdc.LayerCfg[ActiveLayer].FBStartAdress + (2*(Ypos*BSP_LCD_GetXSize() + Xpos)));
  }
  else
  {
    ret = *(__IO uint8_t*) (hltdc.LayerCfg[ActiveLayer].FBStartAdress + (2*(Ypos*BSP_LCD_GetXSize() + Xpos)));
  }
  
  return ret;
}

uint16_t SwapRedBlue_RGB565(uint16_t color)
{
    uint16_t r = (color & 0xF800) >> 11; // Estrae il rosso (0-31)
    uint16_t g = (color & 0x07E0);       // Mantiene il verde intatto nella sua posizione centrale (6 bit)
    uint16_t b = (color & 0x001F);       // Estrae il blu (0-31)

    /* Sposta il blu nella posizione del rosso e il rosso nella posizione del blu */
    return (b << 11) | g | r;
}

#define RGB565_TO_BGR565(c) ((((c) & 0x001F) << 11) | ((c) & 0x07E0) | (((c) & 0xF800) >> 11))

void BSP_LCD_Clear(uint32_t Color)
{
  LL_FillBuffer(ActiveLayer, (uint32_t *)(hltdc.LayerCfg[ActiveLayer].FBStartAdress), BSP_LCD_GetXSize(), BSP_LCD_GetYSize(), 0, Color);
}


//#ifdef ORIG_BSP_LCD_DrawPixel
void BSP_LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos, uint32_t RGB_Code)
{
  if(hltdc.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565)
  {
    *(__IO uint16_t*) (hltdc.LayerCfg[ActiveLayer].FBStartAdress + (2*(Ypos*BSP_LCD_GetXSize() + Xpos))) = (uint16_t)RGB_Code;
  }
  else
  {
    *(__IO uint32_t*) (hltdc.LayerCfg[ActiveLayer].FBStartAdress + (4*(Ypos*BSP_LCD_GetXSize() + Xpos))) = RGB_Code;
  }
}
//#endif

#ifdef NEW_BSP_LCD_DrawPixel
void BSP_LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos, uint32_t Color)
{
    if (Xpos >= 800 || Ypos >= 480) return;

    uint16_t color16 = ((Color >> 19) << 11) | ((Color >> 10) << 5) | ((Color >> 3) & 0x1F); // Conversione robusta RGB565

    // Preleva l'indirizzo direttamente dalla configurazione attiva dell'LTDC
    volatile uint16_t *pFB = (volatile uint16_t *)hltdc.LayerCfg[ActiveLayer].FBStartAdress;

    pFB[((uint32_t)Ypos * hltdc.LayerCfg[ActiveLayer].ImageWidth) + Xpos] = color16;
    __DSB();
}
#endif

static void DrawChar(uint16_t Xpos, uint16_t Ypos, const uint8_t *c)
{
  uint32_t i = 0, j = 0;
  uint16_t height, width;
  uint8_t  offset;
  uint8_t  *pchar;
  uint32_t line;

  height = DrawProp[ActiveLayer].pFont->Height;
  width  = DrawProp[ActiveLayer].pFont->Width;
  offset = 8 *((width + 7)/8) - width ;

  for(i = 0; i < height; i++)
  {
    pchar = ((uint8_t *)c + (width + 7)/8 * i);

    switch(((width + 7)/8))
    {
    case 1:  line =  pchar[0]; break;
    case 2:  line =  (pchar[0]<< 8) | pchar[1]; break;
    case 3:
    default: line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2]; break;
    }

    for (j = 0; j < width; j++)
    {
      if(line & (1 << (width- j + offset- 1)))
      {
        BSP_LCD_DrawPixel((Xpos + j), Ypos, DrawProp[ActiveLayer].TextColor);
      }
      else // if (DrawProp[ActiveLayer].BackColor != LCD_COLOR_TRANSPARENT)
      {
        BSP_LCD_DrawPixel((Xpos + j), Ypos, DrawProp[ActiveLayer].BackColor);
      }
    }
    Ypos++;
  }
}

void BSP_LCD_DisplayChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii)
{
  DrawChar(Xpos, Ypos, &DrawProp[ActiveLayer].pFont->table[(Ascii-' ') *\
    DrawProp[ActiveLayer].pFont->Height * ((DrawProp[ActiveLayer].pFont->Width + 7) / 8)]);
}

void BSP_LCD_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode)
{
  uint16_t refcolumn = 1, i = 0;
  uint32_t size = 0, xsize = 0;
  uint8_t  *ptr = Text;

  while (*ptr++) size ++ ;
  xsize = (BSP_LCD_GetXSize()/DrawProp[ActiveLayer].pFont->Width);

  switch (Mode)
  {
    case CENTER_MODE: refcolumn = Xpos + ((xsize - size)* DrawProp[ActiveLayer].pFont->Width) / 2; break;
    case LEFT_MODE:   refcolumn = Xpos; break;
    case RIGHT_MODE:  refcolumn = - Xpos + ((xsize - size)*DrawProp[ActiveLayer].pFont->Width); break;
    default:          refcolumn = Xpos; break;
  }

  if ((refcolumn < 1) || (refcolumn >= 0x8000))
  {
    refcolumn = 1;
  }

  while ((*Text != 0) & (((BSP_LCD_GetXSize() - (i*DrawProp[ActiveLayer].pFont->Width)) & 0xFFFF) >= DrawProp[ActiveLayer].pFont->Width))
  {
    BSP_LCD_DisplayChar(refcolumn, Ypos, *Text);
    refcolumn += DrawProp[ActiveLayer].pFont->Width;
    Text++;
    i++;
  }
}

void BSP_LCD_SetTextColor(uint32_t Color)
{
  DrawProp[ActiveLayer].TextColor = Color;
}

void BSP_LCD_SetBackColor(uint32_t Color)
{
  DrawProp[ActiveLayer].BackColor = Color;
}

void BSP_LCD_SetFont(sFONT *fonts)
{
  DrawProp[ActiveLayer].pFont = fonts;
}

void BSP_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
  yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, curpixel = 0;

  deltax = ABS(x2 - x1);
  deltay = ABS(y2 - y1);
  x = x1;
  y = y1;

  xinc1 = (x2 >= x1) ? 1 : -1;
  xinc2 = (x2 >= x1) ? 1 : -1;
  yinc1 = (y2 >= y1) ? 1 : -1;
  yinc2 = (y2 >= y1) ? 1 : -1;

  if (deltax >= deltay)
  {
    xinc1 = 0;
    yinc2 = 0;
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;
  }
  else
  {
    xinc2 = 0;
    yinc1 = 0;
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++)
  {
     BSP_LCD_DrawPixel(x, y, DrawProp[ActiveLayer].TextColor);
     num += numadd;
     if (num >= den)
     {
       num -= den;
       x += xinc1;
       y += yinc1;
     }
     x += xinc2;
     y += yinc2;
  }
}

void BSP_LCD_FirstLine()
{
	LL_FillBuffer(ActiveLayer, (uint32_t *)hltdc.LayerCfg[ActiveLayer].FBStartAdress, 1, BSP_LCD_GetXSize(), 0, 0);
}


void BSP_LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pbmp)
{
  uint32_t index = 0, width = 0, height = 0, bit_pixel = 0;
  uint32_t Address;
  uint32_t InputColorMode = 0;

  /* Get bitmap data address offset */
  index = pbmp[10] + (pbmp[11] << 8) + (pbmp[12] << 16)  + (pbmp[13] << 24);

  /* Read bitmap width */
  width = pbmp[18] + (pbmp[19] << 8) + (pbmp[20] << 16)  + (pbmp[21] << 24);

  /* Read bitmap height */
  height = pbmp[22] + (pbmp[23] << 8) + (pbmp[24] << 16)  + (pbmp[25] << 24);

  /* Read bit/pixel */
  bit_pixel = pbmp[28] + (pbmp[29] << 8);

  /* Set the address: RGB565 usa 2 byte per pixel (* 2 anziché * 4) */
  Address = hltdc.LayerCfg[ActiveLayer].FBStartAdress + (((BSP_LCD_GetXSize() * Ypos) + Xpos) * 2);

  /* Get the layer pixel format */
  if ((bit_pixel/8) == 4)
  {
    InputColorMode = CM_ARGB8888;
  }
  else if ((bit_pixel/8) == 2)
  {
    InputColorMode = CM_RGB565;
  }
  else
  {
    InputColorMode = CM_RGB888;
  }

  /* Bypass the bitmap header */
  pbmp += (index + (width * (height - 1) * (bit_pixel/8)));

  /* Convert picture to RGB565 pixel format */
  for(index=0; index < height; index++)
  {
    /* Pixel format conversion */
    LL_ConvertLineToRGB565((uint32_t *)pbmp, (uint32_t *)Address, width, InputColorMode);

    /* Increment the source and destination buffers: * 2 per RGB565 */
    Address += (BSP_LCD_GetXSize() * 2);
    pbmp -= width * (bit_pixel / 8);
  }
}

void LL_ConvertLineToRGB565(void *pSrc, void *pDst, uint32_t xSize, uint32_t ColorMode)
{
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode         = DMA2D_M2M_PFC;
    hdma2d.Init.ColorMode    = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = 0;

    /* Usa LayerCfg[1] come da standard STM32 HAL per il DMA2D */
    hdma2d.LayerCfg[1].AlphaMode      = DMA2D_NO_MODIF_ALPHA;
    hdma2d.LayerCfg[1].InputAlpha     = 0xFF;
    hdma2d.LayerCfg[1].InputColorMode = ColorMode;
    hdma2d.LayerCfg[1].InputOffset    = 0;
    hdma2d.LayerCfg[1].RedBlueSwap    = DMA2D_RB_REGULAR;
    hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
    {
        if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) == HAL_OK)
        {
            if (HAL_DMA2D_Start(&hdma2d, (uint32_t)pSrc, (uint32_t)pDst, xSize, 1) == HAL_OK)
            {
                HAL_DMA2D_PollForTransfer(&hdma2d, 10);
            }
        }
    }
}



void LL_FillBuffer(uint32_t LayerIndex, void *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color)
{
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode         = DMA2D_R2M; // Modalità Register-to-Memory (riempimento colore)
  hdma2d.Init.ColorMode    = DMA2D_OUTPUT_RGB565; // Output a 16-bit RGB565
  hdma2d.Init.OutputOffset = OffLine;

  // Converte il colore da RGB565 (16-bit) ad ARGB8888 (32-bit) richiesto dalla DMA2D in R2M
    uint32_t argb_color = 0xFF000000 |
                          ((Color & 0xF800) << 8) |
                          ((Color & 0x07E0) << 5) |
                          ((Color & 0x001F) << 3);

  if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
  {
    if (HAL_DMA2D_Start(&hdma2d, argb_color, (uint32_t)pDst, xSize, ySize) == HAL_OK)
    {
      HAL_DMA2D_PollForTransfer(&hdma2d, 10);
    }
  }
}

#ifdef DIRECT_WRITE
void BSP_LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pbmp)
{
    uint32_t index = 0, width = 0, height = 0, bit_pixel = 0;

    // Estrai i metadati dall'header standard BMP (offset corretti)
    index     = pbmp[10] + (pbmp[11] << 8) + (pbmp[12] << 16) + (pbmp[13] << 24);
    width     = pbmp[18] + (pbmp[19] << 8) + (pbmp[20] << 16) + (pbmp[21] << 24);
    height    = pbmp[22] + (pbmp[23] << 8) + (pbmp[24] << 16) + (pbmp[25] << 24);
    bit_pixel = pbmp[28] + (pbmp[29] << 8);

    volatile uint16_t *pFB = (volatile uint16_t *)0xC0000000;
    uint16_t *pSrc = (uint16_t *)(pbmp + index);

    // I BMP sono memorizzati dal basso verso l'alto
    for(int32_t y = height - 1; y >= 0; y--)
    {
        uint32_t target_y = Ypos + (height - 1 - y);
        if(target_y >= 480) continue; // Evita sforamenti verticali

        for(uint32_t x = 0; x < width; x++)
        {
            uint32_t target_x = Xpos + x;
            if(target_x >= 800) continue; // Evita sforamenti orizzontali

            // Copia il pixel convertendo o direttamente a seconda del formato
            pFB[(target_y * 800) + target_x] = pSrc[(y * width) + x];
        }
    }
}
#endif


void LL_ConvertLineToARGB8888(void *pSrc, void *pDst, uint32_t xSize, uint32_t ColorMode)
{
	hdma2d.Init.Mode         = DMA2D_M2M_PFC;
	hdma2d.Init.ColorMode    = DMA2D_OUTPUT_ARGB8888;
	hdma2d.Init.OutputOffset = 0;

	hdma2d.LayerCfg[0].AlphaMode = DMA2D_NO_MODIF_ALPHA; // <-- Modificato a 0
	hdma2d.LayerCfg[0].InputAlpha = 0xFF;                 // <-- Modificato a 0[cite: 4, 6, 9, 11]
	hdma2d.LayerCfg[0].InputColorMode = ColorMode;       // <-- Modificato a 0[cite: 4, 6, 9, 11]
	hdma2d.LayerCfg[0].InputOffset = 0;                  // <-- Modificato a 0[cite: 4, 6, 9, 11]

	hdma2d.Instance = DMA2D;

  if(HAL_DMA2D_Init(&hdma2d) == HAL_OK)
  {
    if(HAL_DMA2D_ConfigLayer(&hdma2d, 0) == HAL_OK)     // <-- Modificato a 0[cite: 4, 6, 9, 11]
    {
      if (HAL_DMA2D_Start(&hdma2d, (uint32_t)pSrc, (uint32_t)pDst, xSize, 1) == HAL_OK)
      {
        HAL_DMA2D_PollForTransfer(&hdma2d, 10);
      }
    }
  }
}



// Funzione per disegnare un rettangolo pieno in modo sicuro e lineare
//#ifdef OLD
void DrawSafeRect(uint16_t x_start, uint16_t y_start, uint16_t width, uint16_t height, uint16_t color)
{
    if (x_start >= 800 || y_start >= 480) return;
    if ((x_start + width) > 800) width = 800 - x_start;
    if ((y_start + height) > 480) height = 480 - y_start;

    volatile uint16_t *pFB = (volatile uint16_t *)0xC0000000;

    for(uint16_t y = 0; y < height; y++)
    {
        volatile uint16_t *pRow = &pFB[((y_start + y) * 800) + x_start];
        for(uint16_t x = 0; x < width; x++)
        {
            pRow[x] = color;
        }
    }

    __DSB();
    __ISB();
}
//#endif





