#ifndef ANALOG_IP
#define ANALOG_IP

#include "util.h"

typedef struct
{
  __IO uint32_t CTRL;         // 0x00
  __IO uint32_t STAT;         // 0x04
  __IO uint32_t DATA;         // 0x08
  __IO uint32_t RESERVED[12]; // 0x0C - 0x38
  __IO uint32_t CHNL;         // 0x3C
  __IO uint32_t SEQ[16];      // 0x40 - 0x7C
} ADC_TypeDef;
#define ADC0 ((ADC_TypeDef *) 0x60000000)
#define ADC1 ((ADC_TypeDef *) 0x60001000)
#define ADC2 ((ADC_TypeDef *) 0x60002000)

#define ADC_CTRL_START (1 << 0) 
#define ADC_CTRL_STOP  (1 << 1) 
#define ADC_CTRL_CONT  (1 << 2) 
#define ADC_CTRL_DMAEN (1 << 3) 
#define ADC_CTRL_SCLK_DIV(__div) (__div << 16)

#define ADC_STAT_EN  (1 << 0)
#define ADC_STAT_EOC (1 << 1)

#define ADC_MAX_VALUE 0xfff

typedef enum
{
  ADC_CHANNEL0  = 1 ,
  ADC_CHANNEL1  = 2 ,
  ADC_CHANNEL2  = 3 ,
  ADC_CHANNEL3  = 4 ,
  ADC_CHANNEL4  = 5 ,
  ADC_CHANNEL5  = 6 ,
  ADC_CHANNEL6  = 7 ,
  ADC_CHANNEL7  = 8 ,
  ADC_CHANNEL8  = 9 ,
  ADC_CHANNEL9  = 10,
  ADC_CHANNEL10 = 11,
  ADC_CHANNEL11 = 12,
  ADC_CHANNEL12 = 13,
  ADC_CHANNEL13 = 14,
  ADC_CHANNEL14 = 15,
  ADC_CHANNEL15 = 16,
  ADC_CHANNEL16 = 17,
} ADC_ChannelNumTypeDef;

typedef enum
{
  ADC_SEQ_LENGTH1  = 0 ,
  ADC_SEQ_LENGTH2  = 1 ,
  ADC_SEQ_LENGTH3  = 2 ,
  ADC_SEQ_LENGTH4  = 3 ,
  ADC_SEQ_LENGTH5  = 4 ,
  ADC_SEQ_LENGTH6  = 5 ,
  ADC_SEQ_LENGTH7  = 6 ,
  ADC_SEQ_LENGTH8  = 7 ,
  ADC_SEQ_LENGTH9  = 8,
  ADC_SEQ_LENGTH10 = 9,
  ADC_SEQ_LENGTH11 = 10,
  ADC_SEQ_LENGTH12 = 11,
  ADC_SEQ_LENGTH13 = 12,
  ADC_SEQ_LENGTH14 = 13,
  ADC_SEQ_LENGTH15 = 14,
  ADC_SEQ_LENGTH16 = 15,
} ADC_SeqLengthTypeDef;

// ADC sample rate is: APB clock frequency / (1 + sclk_div) / 2 / 13
static inline void ADC_Start(ADC_TypeDef *adc, uint32_t sclk_div) { adc->CTRL = ADC_CTRL_START | ADC_CTRL_SCLK_DIV(sclk_div); }
static inline void ADC_StartContinuous(ADC_TypeDef *adc, uint32_t sclk_div) { adc->CTRL = ADC_CTRL_START | ADC_CTRL_CONT | ADC_CTRL_SCLK_DIV(sclk_div); }
static inline void ADC_StartDma(ADC_TypeDef *adc, uint32_t sclk_div) { adc->CTRL = ADC_CTRL_START | ADC_CTRL_CONT | ADC_CTRL_DMAEN | ADC_CTRL_SCLK_DIV(sclk_div); }
static inline void ADC_Stop(ADC_TypeDef *adc) { adc->CTRL |= ADC_CTRL_STOP; }
static inline void ADC_WaitForEoc(ADC_TypeDef *adc) { while (!(adc->STAT & ADC_STAT_EOC)); }
static inline void ADC_SetChannel(ADC_TypeDef *adc, ADC_ChannelNumTypeDef channel) { adc->SEQ[0] = channel; }
static inline void ADC_SetSeqLength(ADC_TypeDef *adc, ADC_SeqLengthTypeDef seq_length) { adc->CHNL = seq_length; }
static inline void ADC_SetSeqChannel(ADC_TypeDef *adc, uint32_t seq, ADC_ChannelNumTypeDef channel) { adc->SEQ[seq] = channel; }

static inline uint32_t ADC_GetStat(ADC_TypeDef *adc) { return adc->STAT; }
static inline uint32_t ADC_GetData(ADC_TypeDef *adc) { return adc->DATA; }

typedef struct
{
  __IO uint32_t CTRL; // 0x00
  __IO uint32_t DATA; // 0x04
} DAC_TypeDef;
#define DAC0 ((DAC_TypeDef *) 0x60003000)
#define DAC1 ((DAC_TypeDef *) 0x60004000)

#define DAC_CTRL_EN    (1 << 0)
#define DAC_CTRL_BUFEN (1 << 1)
#define DAC_CTRL_DMAEN (1 << 2)
#define DAC_CTRL_SCLK_DIV(__div) (__div << 16)

#define DAC_MAX_VALUE 0x3ff

static inline void DAC_Enable(DAC_TypeDef *dac) { dac->CTRL = DAC_CTRL_EN | DAC_CTRL_BUFEN; }
static inline void DAC_EnableNoBuf(DAC_TypeDef *dac) { dac->CTRL = DAC_CTRL_EN; }
static inline void DAC_Disable(DAC_TypeDef *dac) { dac->CTRL = 0; }

// DAC sample rate is: APB clock frequency / (1 + sclk_div)
static inline void DAC_EnableDma(DAC_TypeDef *dac, uint32_t sclk_div) { dac->CTRL = dac->CTRL & 0xffff | DAC_CTRL_DMAEN | DAC_CTRL_SCLK_DIV(sclk_div); }
static inline void DAC_DisableDma(DAC_TypeDef *dac) { dac->CTRL &= ~DAC_CTRL_DMAEN; }

static inline void DAC_SetData(DAC_TypeDef *dac, uint32_t data) { dac->DATA = data; }

typedef struct
{
  __IO uint32_t CTRL; // 0x00
  __IO uint32_t CHNL; // 0x04
  __IO uint32_t DATA; // 0x08
} CMP_TypeDef;
#define CMP0 ((CMP_TypeDef *) 0x60005000)

#define CMP_CTRL_EN1 (1 << 0)
#define CMP_CTRL_EN2 (1 << 8)

#define CMP_CHNL_PSEL1_OFFSET 0
#define CMP_CHNL_MSEL1_OFFSET 4
#define CMP_CHNL_PSEL2_OFFSET 8
#define CMP_CHNL_MSEL2_OFFSET 12

#define CMP_CHNL_PSEL1_MASK (3 << CMP_CHNL_PSEL1_OFFSET)
#define CMP_CHNL_MSEL1_MASK (7 << CMP_CHNL_MSEL1_OFFSET)
#define CMP_CHNL_PSEL2_MASK (3 << CMP_CHNL_PSEL2_OFFSET)
#define CMP_CHNL_MSEL2_MASK (7 << CMP_CHNL_MSEL2_OFFSET)

#define CMP_DATA1 (1 << 0)
#define CMP_DATA2 (1 << 8)

#define ADC0_DMA_REQ EXT_DMA0_REQ
#define ADC1_DMA_REQ EXT_DMA1_REQ
#define ADC2_DMA_REQ EXT_DMA2_REQ // Shared with DAC1
#define DAC0_DMA_REQ EXT_DMA3_REQ
#define DAC1_DMA_REQ EXT_DMA2_REQ // Shared with ADC2

typedef enum
{
  CMP_PSEL_CHNL1 = 1,
  CMP_PSEL_CHNL2 = 2,
} CMP_PselChnlTypeDef;

typedef enum
{
  CMP_MSEL_CHNL1 = 1,
  CMP_MSEL_CHNL2 = 2,
  CMP_MSEL_CHNL3 = 3,
  CMP_MSEL_CHNL4 = 4,
  CMP_MSEL_CHNL5 = 5,
  CMP_MSEL_CHNL6 = 6,
  CMP_MSEL_CHNL7 = 7,
} CMP_MselChnlTypeDef;

static inline void CMP_Enable1 (CMP_TypeDef *cmp) { cmp->CTRL |=  CMP_CTRL_EN1; }
static inline void CMP_Enable2 (CMP_TypeDef *cmp) { cmp->CTRL |=  CMP_CTRL_EN2; }
static inline void CMP_Disable1(CMP_TypeDef *cmp) { cmp->CTRL &= ~CMP_CTRL_EN1; }
static inline void CMP_Disable2(CMP_TypeDef *cmp) { cmp->CTRL &= ~CMP_CTRL_EN2; }

static inline void CMP_SetPsel1(CMP_TypeDef *cmp, CMP_PselChnlTypeDef psel) { MODIFY_REG(cmp->CHNL, CMP_CHNL_PSEL1_MASK, psel << CMP_CHNL_PSEL1_OFFSET); }
static inline void CMP_SetMsel1(CMP_TypeDef *cmp, CMP_MselChnlTypeDef msel) { MODIFY_REG(cmp->CHNL, CMP_CHNL_MSEL1_MASK, msel << CMP_CHNL_MSEL1_OFFSET); }
static inline void CMP_SetPsel2(CMP_TypeDef *cmp, CMP_PselChnlTypeDef psel) { MODIFY_REG(cmp->CHNL, CMP_CHNL_PSEL2_MASK, psel << CMP_CHNL_PSEL2_OFFSET); }
static inline void CMP_SetMsel2(CMP_TypeDef *cmp, CMP_MselChnlTypeDef msel) { MODIFY_REG(cmp->CHNL, CMP_CHNL_MSEL2_MASK, msel << CMP_CHNL_MSEL2_OFFSET); }

static inline bool CMP_GetData1(CMP_TypeDef *cmp) { return cmp->DATA & CMP_DATA1; }
static inline bool CMP_GetData2(CMP_TypeDef *cmp) { return cmp->DATA & CMP_DATA2; }

#endif
