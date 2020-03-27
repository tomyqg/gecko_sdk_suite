/***************************************************************************//**
 * @file
 * @brief Filters implementation
 * @version
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef FILTER_H_
#define FILTER_H_

#include <stdint.h>
#include <stdbool.h>

/***************************************************************************//**
 * @addtogroup Filters
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup Filters_Config_Settings Filters Configuration Settings
 * @{
 * @brief Filters configuration setting macro definitions
 ******************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/
#ifndef M_LN2
#define M_LN2    0.69314718055994530942
#endif

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

/** Default filter */
#define DEFAULT_FILTER                       \
  {                                          \
    HPF,  /** Default filter type */         \
    0,    /** Default gain */                \
    100,  /** Default frequency */           \
    8000, /** Default sampling rate */       \
    2,    /** Default bandwidth in octaves*/ \
  }

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

/** Sample type */
typedef double sample_t;

/** This structure holds the data required to update samples through a filter */
typedef struct {
  sample_t a0, a1, a2, a3, a4;
  sample_t x1, x2, y1, y2;
}biquad_t;

/** Calculated filter coefficients */
typedef struct {
  sample_t A, omega, sn, cs, alpha, beta;
  sample_t a0, a1, a2, b0, b1, b2;
}filter_coefficient_t;

/** Filter types */
typedef enum {
  LPF,     /**< Low Pass Filter        */
  HPF,     /**< High Pass Filter       */
  BPF,     /**< Band Pass Filter       */
  NOTCH,   /**< Notch Filter           */
  PEQ,     /**< Peaking Band EQ Filter */
  LSH,     /**< Low Shelf Filter       */
  HSH      /**< High Shelf Filter      */
}filter_type_t;

/** Filter parameters */
typedef struct {
  filter_type_t type;           /**< Filter type */
  sample_t      dbGain;         /**< Gain of filter */
  sample_t      freq;           /**< Center frequency */
  sample_t      srate;          /**< Sampling rate */
  sample_t      bandwidth;      /**< Bandwidth in octaves */
}filter_parameters_t;

/** @} {end defgroup Filters_Config_Settings} */

/***************************************************************************//**
 * @defgroup Filters_Functions Filters Functions
 * @{
 * @brief Filters support functions
 ******************************************************************************/
biquad_t *FIL_Init(filter_parameters_t *fp);
bool FIL_filter(int16_t *buffOut, uint16_t *buffIn, uint16_t size);

/** @} {end defgroup Filters_Functions}*/

/** @} {end addtogroup Filters} */

#endif /* FILTER_H_ */
