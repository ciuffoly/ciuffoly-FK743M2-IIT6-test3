#ifndef __STM32746G_LCD_H
#define __STM32746G_LCD_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
/* Include SDRAM Driver */
//#include "BSP_SDRAM.h"
#include "DEV_Config.h"
#include "fonts.h"


/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32746G_DISCOVERY
  * @{
  */
    
/** @addtogroup STM32746G_DISCOVERY_LCD
  * @{
  */ 

/** @defgroup STM32746G_DISCOVERY_LCD_Exported_Types STM32746G_DISCOVERY_LCD Exported Types
  * @{
  */  
typedef struct 
{
  int16_t X;
  int16_t Y;
}Point, * pPoint; 

/**
  * @}
  */ 

/** @defgroup STM32746G_DISCOVERY_LCD_Exported_Constants STM32746G_DISCOVERY_LCD Exported Constants
  * @{
  */ 
#define MAX_LAYER_NUMBER       ((uint32_t)2)

#define LCD_LayerCfgTypeDef    LTDC_LayerCfgTypeDef

#define LTDC_ACTIVE_LAYER	     ((uint32_t)1) /* Layer 1 */
/** 
  * @brief  LCD status structure definition  
  */     
#define LCD_OK                 ((uint8_t)0x00)
#define LCD_ERROR              ((uint8_t)0x01)
#define LCD_TIMEOUT            ((uint8_t)0x02)

/** 
  * @brief  LCD FB_StartAddress  
  */
//#define LCD_FB_START_ADDRESS       ((uint32_t)0xD0000000)

/** 
  * @brief  LCD color  
  */ 
/* --------------------------------RGB565 -------------------------------------*/
#define LCD_COLOR_BLUE          0x001F
#define LCD_COLOR_GREEN         0x07E0
#define LCD_COLOR_RED           0xF800
#define LCD_COLOR_CYAN          0x07FF
#define LCD_COLOR_MAGENTA       0xF81F
#define LCD_COLOR_YELLOW        0xFFE0
#define LCD_COLOR_LIGHTBLUE     0x841F
#define LCD_COLOR_LIGHTGREEN    0x87F0
#define LCD_COLOR_LIGHTRED      0xFC10
#define LCD_COLOR_LIGHTCYAN     0x87FF
#define LCD_COLOR_LIGHTMAGENTA  0xFC1F
#define LCD_COLOR_LIGHTYELLOW   0xFFF0
#define LCD_COLOR_DARKBLUE      0x0010
#define LCD_COLOR_DARKGREEN     0x0400
#define LCD_COLOR_DARKRED       0x8000
#define LCD_COLOR_DARKCYAN      0x0410
#define LCD_COLOR_DARKMAGENTA   0x8010
#define LCD_COLOR_DARKYELLOW    0x8400
#define LCD_COLOR_WHITE         0xFFFF
#define LCD_COLOR_LIGHTGRAY     0xD69A
#define LCD_COLOR_GRAY          0x8410
#define LCD_COLOR_DARKGRAY      0x4208
#define LCD_COLOR_BLACK         0x0000
#define LCD_COLOR_BROWN         0xA145
#define LCD_COLOR_ORANGE        0xFD20

/* ARGB8888 colors definitions */
#ifdef ARGB8888_COLORS
#define LCD_COLOR_ARGB8888_BLUE               0xFF0000FFUL
#define LCD_COLOR_ARGB8888_GREEN              0xFF00FF00UL
#define LCD_COLOR_ARGB8888_RED                0xFFFF0000UL
#define LCD_COLOR_ARGB8888_CYAN               0xFF00FFFFUL
#define LCD_COLOR_ARGB8888_MAGENTA            0xFFFF00FFUL
#define LCD_COLOR_ARGB8888_YELLOW             0xFFFFFF00UL
#define LCD_COLOR_ARGB8888_LIGHTBLUE          0xFF8080FFUL
#define LCD_COLOR_ARGB8888_LIGHTGREEN         0xFF80FF80UL
#define LCD_COLOR_ARGB8888_LIGHTRED           0xFFFF8080UL
#define LCD_COLOR_ARGB8888_LIGHTCYAN          0xFF80FFFFUL
#define LCD_COLOR_ARGB8888_LIGHTMAGENTA       0xFFFF80FFUL
#define LCD_COLOR_ARGB8888_LIGHTYELLOW        0xFFFFFF80UL
#define LCD_COLOR_ARGB8888_DARKBLUE           0xFF000080UL
#define LCD_COLOR_ARGB8888_DARKGREEN          0xFF008000UL
#define LCD_COLOR_ARGB8888_DARKRED            0xFF800000UL
#define LCD_COLOR_ARGB8888_DARKCYAN           0xFF008080UL
#define LCD_COLOR_ARGB8888_DARKMAGENTA        0xFF800080UL
#define LCD_COLOR_ARGB8888_DARKYELLOW         0xFF808000UL
#define LCD_COLOR_ARGB8888_WHITE              0xFFFFFFFFUL
#define LCD_COLOR_ARGB8888_LIGHTGRAY          0xFFD3D3D3UL
#define LCD_COLOR_ARGB8888_GRAY               0xFF808080UL
#define LCD_COLOR_ARGB8888_DARKGRAY           0xFF404040UL
#define LCD_COLOR_ARGB8888_BLACK              0xFF000000UL
#define LCD_COLOR_ARGB8888_BROWN              0xFFA52A2AUL
#define LCD_COLOR_ARGB8888_ORANGE             0xFFFFA500UL
/* Definition of Official ST Colors */
#define LCD_COLOR_ARGB8888_ST_BLUE_DARK       0xFF002052UL
#define LCD_COLOR_ARGB8888_ST_BLUE            0xFF39A9DCUL
#define LCD_COLOR_ARGB8888_ST_BLUE_LIGHT      0xFFD1E4F3UL
#define LCD_COLOR_ARGB8888_ST_GREEN_LIGHT     0xFFBBCC01UL
#define LCD_COLOR_ARGB8888_ST_GREEN_DARK      0xFF003D14UL
#define LCD_COLOR_ARGB8888_ST_YELLOW          0xFFFFD300UL
#define LCD_COLOR_ARGB8888_ST_BROWN           0xFF5C0915UL
#define LCD_COLOR_ARGB8888_ST_PINK            0xFFD4007AUL
#define LCD_COLOR_ARGB8888_ST_PURPLE          0xFF590D58UL
#define LCD_COLOR_ARGB8888_ST_GRAY_DARK       0xFF4F5251UL
#define LCD_COLOR_ARGB8888_ST_GRAY            0xFF90989EUL
#define LCD_COLOR_ARGB8888_ST_GRAY_LIGHT      0xFFB9C4CAUL
#endif

typedef enum
{
  CENTER_MODE             = 0x01,    /*!< Center mode */
  RIGHT_MODE              = 0x02,    /*!< Right mode  */
  LEFT_MODE               = 0x03     /*!< Left mode   */

} Text_AlignModeTypdef;


uint8_t  BSP_LCD_Init(void);

uint32_t BSP_LCD_GetXSize(void);
uint32_t BSP_LCD_GetYSize(void);


/* Functions using the LTDC controller */
void     BSP_LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency);
void     BSP_LCD_SetLayerAddress(uint32_t LayerIndex, uint32_t Address);
void     BSP_LCD_SetColorKeying(uint32_t LayerIndex, uint32_t RGBValue);
void     BSP_LCD_ResetColorKeying(uint32_t LayerIndex);
void     BSP_LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);

void     BSP_LCD_SelectLayer(uint32_t LayerIndex);
void     BSP_LCD_SetLayerVisible(uint32_t LayerIndex, FunctionalState State);

uint32_t BSP_LCD_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     BSP_LCD_DrawPixel(uint16_t Xpos, uint16_t Ypos, uint32_t pixel);
void     BSP_LCD_Clear(uint32_t );
void     BSP_LCD_SetTextColor(uint32_t );
void     BSP_LCD_SetBackColor(uint32_t );
void     BSP_LCD_SetFont(sFONT *);
void     BSP_LCD_DisplayStringAt(uint16_t , uint16_t , uint8_t *, Text_AlignModeTypdef );
void     BSP_LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pbmp);
void     BSP_LCD_DrawBitmapFromStruct(uint32_t Xpos, uint32_t Ypos, uint32_t width, uint32_t height,  uint16_t *bmp_data);
int32_t  BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color);
void     BSP_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

//#ifdef __cplusplus/
//}
//#endif

//###########################################################################################
//# NEW


/** @brief Green value in ARGB8888 format  */
#ifdef ARGB8888_COLORS
#define LCD_COLOR_BLUE          ((uint32_t) 0xFF0000FF)
#define LCD_COLOR_GREEN         ((uint32_t) 0xFF00FF00)
#define LCD_COLOR_RED           ((uint32_t) 0xFFFF0000)
#define LCD_COLOR_CYAN          ((uint32_t) 0xFF00FFFF)
#define LCD_COLOR_MAGENTA       ((uint32_t) 0xFFFF00FF)
#define LCD_COLOR_YELLOW        ((uint32_t) 0xFFFFFF00)
#define LCD_COLOR_LIGHTBLUE     ((uint32_t) 0xFF8080FF)
#define LCD_COLOR_LIGHTGREEN    ((uint32_t) 0xFF80FF80)
#define LCD_COLOR_LIGHTRED      ((uint32_t) 0xFFFF8080)
#define LCD_COLOR_LIGHTCYAN     ((uint32_t) 0xFF80FFFF)
#define LCD_COLOR_LIGHTMAGENTA  ((uint32_t) 0xFFFF80FF)
#define LCD_COLOR_LIGHTYELLOW   ((uint32_t) 0xFFFFFF80)
#define LCD_COLOR_DARKBLUE      ((uint32_t) 0xFF000080)
#define LCD_COLOR_DARKGREEN     ((uint32_t) 0xFF008000)
#define LCD_COLOR_DARKRED       ((uint32_t) 0xFF800000)
#define LCD_COLOR_DARKCYAN      ((uint32_t) 0xFF008080)
#define LCD_COLOR_DARKMAGENTA   ((uint32_t) 0xFF800080)
#define LCD_COLOR_DARKYELLOW    ((uint32_t) 0xFF808000)
#define LCD_COLOR_WHITE         ((uint32_t) 0xFFFFFFFF)
#define LCD_COLOR_LIGHTGRAY     ((uint32_t) 0xFFD3D3D3)
#define LCD_COLOR_GRAY          ((uint32_t) 0xFF808080)
#define LCD_COLOR_DARKGRAY      ((uint32_t) 0xFF404040)
#define LCD_COLOR_BLACK         ((uint32_t) 0xFF000000)
#define LCD_COLOR_BROWN         ((uint32_t) 0xFFA52A2A)
#define LCD_COLOR_ORANGE        ((uint32_t) 0xFFFFA500)
#define LCD_COLOR_TRANSPARENT   ((uint32_t) 0xFF000000)
#endif

#define LTDC_MAX_LAYER_NUMBER             ((uint32_t) 2)

 typedef struct
 {
   uint32_t TextColor; /*!< Specifies the color of text */
   uint32_t BackColor; /*!< Specifies the background color below the text */
   sFONT    *pFont;    /*!< Specifies the font used for the text */

 } LCD_DrawPropTypeDef;

#endif /* __STM32746G_DISCOVERY_LCD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
