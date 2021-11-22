/*!
 * @file
 */
#ifndef __DS3231_H__
#define __DS3231_H__

#include <esp_types.h>
#include <driver/i2c.h>

struct DS3231_Cfg; //!< Configuration structure for DS3231 component

typedef struct DS3231_Cfg* DS3231_Cfg_t; //!< Configuration structure for DS3231 component

/**
 * @enum DS3231_ClockType_t
 * @brief Defines use of 12 or 24 hour clock.
 */
typedef enum __attribute__((__packed__))
{
  DS3231_ClockType_12_Hour, //!< 12 Hour Clock
  DS3231_ClockType_24_Hour  //!< 24 Hour Clock
} DS3231_ClockType_t;

/**
 * @enum DS3231_AM_PM_t
 * @brief AM and PM values for 12 Hour Clock
 */
typedef enum __attribute__((__packed__))
{
  DS3231_AM,    //!< AM
  DS3231_PM     //!< PM
} DS3231_AM_PM_t;

/**
 * @brief Structure for DS3231 Calendar
 */
typedef struct
{
  uint8_t seconds;                //!< Seconds in the minute 0-59
  uint8_t minutes;                //!< Minutes in the hour 0-59
  uint8_t hour;                   //!< The hour of the day, 0-23 for 24-hour clock, 1-12 for 12 hour clock
  uint8_t day_of_week;            //!< User defined day of week, 1-7
  uint8_t day_of_month;           //!< Day of the month, 1-31
  uint8_t month;                  //!< Month of the year 1-12
  uint16_t year;                  //!< The year, 2000-2199
  DS3231_ClockType_t clock_type;  //!< The type of the clock, either 12 or 24-hour clock. Defines how hour is interpreted
  DS3231_AM_PM_t am_pm;           //!< Either AM or PM. Only used when clock type is 12-hour clock
} DS3231_Calendar_t;

/**
 * @enum DS3231_AlarmDayType_t
 * @brief Defines how to interpret DS3231_AlarmSetting_t.day
 */
typedef enum __attribute__((__packed__))
{
  DS3231_AlarmDayType_DayOfWeek,  //!< Day is day of week 1-7
  DS3231_AlarmDayType_DayOfMonth  //!< Day is day of month 1-31
} DS3231_AlarmDayType_t;

/**
 * @enum DS3231_AlarmRate_t
 * @brief The rate at which the alarm will be fired. 
 */
typedef enum __attribute__((__packed__))
{
  DS3231_AlarmRate_PerSecond  = 0x0F, //!< Once per second (Alarm 1)
  DS3231_AlarmRate_S_Match    = 0x0E, //!< Seconds Match (Alarm 1)
  DS3231_AlarmRate_MS_Match   = 0x0C, //!< Minutes and Seconds Match (Alarm 1)
  DS3231_AlarmRate_HMS_Match  = 0x08, //!< Hours, Minutes, and Seconds Match (Alarm 1)
  DS3231_AlarmRate_DHMS_Match = 0x00, //!< Day, Hours, Minutes, and Seconds Match (Alarm 1)

  DS3231_AlarmRate_PerMinute  = 0x07, //!< Once per minute (Alarm 2)
  DS3231_AlarmRate_M_Match    = 0x06, //!< Minutes Match (Alarm 2)
  DS3231_AlarmRate_HM_Match   = 0x04, //!< Hours and Minutes Match (Alarm 2)
  DS3231_AlarmRate_DHM_Match  = 0x00, //!< Day, Hours, and Minutes Match (Alarm 2)
} DS3231_AlarmRate_t;

/**
 * @brief The selection of alarm in DS3231_AlarmSetting_t
 */
typedef enum __attribute__((__packed__))
{
  DS3231_AlarmType_Alarm1 = 1, //!< Use Alarm 1
  DS3231_AlarmType_Alarm2 = 2  //!< Use Alarm 2
} DS3231_AlarmType_t;

/**
 * @brief Alarm configuration.
 */
typedef struct
{
  uint8_t seconds;                  //!< The seconds on which the alarm should match. Alarm 1 only.
  uint8_t minutes;                  //!< The minutes on which the alarm should match.
  uint8_t hour;                     //!< The hour on which the alarm should match.
  uint8_t day;                      //!< The day on which the alarm should match; day_type defines whether this is day of week or day of month.

  DS3231_AlarmType_t alarm_type;    //!< Selection of alarm 1 or alarm 2.
  DS3231_ClockType_t clock_type;    //!< Whether alarm hour should be interpreted on a 12 or 24-hour clock
  DS3231_AM_PM_t am_pm;             //!< If clock_type == DS3231_ClockType_12_Hour then this defines whether the hour is AM or PM.
  DS3231_AlarmDayType_t day_type;   //!< Flag for whether day is day of week or day of month
  DS3231_AlarmRate_t alarm_rate;    //!< The alarm rate on which the alarm will fire.
} DS3231_AlarmSetting_t;

/**
 * @brief Flags for interrupt enable or interrupt fired.
 */
typedef enum __attribute__((__packed__))
{
  DS3231_Interrupt_None     = 0x00, //!< No interrupts enabled or no interrupt fired.
  DS3231_Interrupt_Alarm_1  = 0x01, //!< Alarm 1 interrupt enable or Alarm 1 interrupt fired.
  DS3231_Interrupt_Alarm_2  = 0x02  //!< Alarm 1 interrupt enable or Alarm 2 interrupt fired.
} DS3231_Interrupt_t;

/**
 * @brief Square wave frequencies
 */
typedef enum __attribute__((__packed__))
{
  DS3231_SquareWave_1Hz       = 0x00, //!< Output 1Hz square wave
  DS3231_SquareWave_1p024kHz  = 0x01, //!< Output 1.024kHz square wave
  DS3231_SquareWave_4p096kHz  = 0x02, //!< Output 4.096kHz square wave
  DS3231_SquareWave_8p192kHz  = 0x03, //!< Output 8.192kHz square wave
  DS3231_SquareWave_Off       = 0xFF  //!< Disable square wave generation
} DS3231_SquareWave_t;

/**
 * @brief Flag for enabling the oscillator.
 */
typedef enum __attribute__((__packed__))
{
  DS3231_Oscillator_Enable  = 0,  //!< Oscillator is enabled
  DS3231_Oscillator_Disable = 1   //!< Oscillator is disabled
} DS3231_Oscillator_t;

/**
 * @brief Flag for enabling the 32kHz signal generation
 */
typedef enum __attribute__((__packed__))
{
  DS3231_32kHz_Disable  = 0,  //!< 32kHz signal generation is disabled
  DS3231_32kHz_Enable   = 1   //!< 32kHz signal generation is enabled
} DS3231_32kHz_t;

/**
 * @brief Construct configuration for DS3231. This does not initialize the i2c system. Use ds3231_delete to free the returned pointer.
 * 
 * @param i2c_port The i2c port to use, either I2C_NUM_0 or I2C_NUM_1
 * @return DS3231_Cfg_t 
 */
DS3231_Cfg_t ds3231_create(i2c_port_t i2c_port);

/**
 * @brief Return the current calendar from the DS3231.
 * 
 * @param cfg The configuration for the DS3231 component.
 * @param[out] calendar The calendar to populate.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t ESP_OK if calendar was populated, ESP_ERR_* otherwise.
 */
esp_err_t ds3231_get_calendar(DS3231_Cfg_t cfg, DS3231_Calendar_t* calendar, TickType_t timeout);

/**
 * @brief Set the calendar in the DS3231. 
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[in] calendar The calendar to use to configure the DS3231.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_calendar(DS3231_Cfg_t cfg, DS3231_Calendar_t* calendar, TickType_t timeout);

/**
 * @brief Get the temperature from the DS3231.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] temperature The temperture as reported by the DS3231.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_temperature(DS3231_Cfg_t cfg, float* temperature, TickType_t timeout);

/**
 * @brief Get an alarm configuration. The parameter alarm must have alarm_type set in order to get an alarm.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[in, out] alarm The alarm structure in which store the alarm configuration. 
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_alarm(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout);

/**
 * @brief Set an alarm configuration in the DS3231.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[in] alarm The alarm structure from which the DS3231 will be configured.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_alarm(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout);

/**
 * @brief Set the frequency of the square wave generated. Changing this setting from 1Hz is not supported on DS3231M chips.
 *  
 * @param cfg The configuration of the DS3231 component.
 * @param square_wave_setting The frequency of the square wave generated.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_square_wave(DS3231_Cfg_t cfg, DS3231_SquareWave_t square_wave_setting, TickType_t timeout);

/**
 * @brief Get the frequency of the square wave generated. This setting will always be 1Hz on DS3231M chips.
 *  
 * @param cfg The configuration of the DS3231 component.
 * @param[out] square_wave_setting The frequency of the square wave generated.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_square_wave(DS3231_Cfg_t cfg, DS3231_SquareWave_t* square_wave_setting, TickType_t timeout);

/**
 * @brief Assert the convert bit and trigger a temperature conversion.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_convert_temperature(DS3231_Cfg_t cfg, TickType_t timeout);

/**
 * @brief Get the value of the convert bit - will be zero when conversion is complete.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] conv The current value of the conv bit in the control/status register.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_convert_temperature(DS3231_Cfg_t cfg, uint8_t* conv, TickType_t timeout);

/**
 * @brief Enable or disable the oscillator in the DS3231 chip.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param oscillator_en Flag indicating whether the oscillator should be active or not. 
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_osc(DS3231_Cfg_t cfg, DS3231_Oscillator_t oscillator_en, TickType_t timeout);

/**
 * @brief Get the current oscillator status
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] oscillator_en Either DS3231_Oscillator_Enable or DS3231_Oscillator_Disable
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_osc(DS3231_Cfg_t cfg, DS3231_Oscillator_t* oscillator_en, TickType_t timeout);

/**
 * @brief Enable or disable the 32kHz square wave generation.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param en_32kHz Flag indicating whether the 32kHz square generation should be enabled or disabled.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_32kHz(DS3231_Cfg_t cfg, DS3231_32kHz_t en_32kHz, TickType_t timeout);

/**
 * @brief Get the status of the 32kHz square wave generation.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] en_32kHz The value will be non-zero if the 32kHz output is enabled.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_32kHz(DS3231_Cfg_t cfg, DS3231_32kHz_t* en_32kHz, TickType_t timeout);

/**
 * @brief Get the busy status of the DS3231.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] busy The value will be non-zero if the busy bit is asserted.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_is_busy(DS3231_Cfg_t cfg, uint8_t* busy, TickType_t timeout);

/**
 * @brief Get the oscillator stop flag.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] osc_stop_flag Value will be non-zero if the oscillator had stopped.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_osc_stop_flag(DS3231_Cfg_t cfg, uint8_t* osc_stop_flag, TickType_t timeout);

/**
 * @brief Clear the oscillator stopped flag.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_clear_osc_stop_flag(DS3231_Cfg_t cfg, TickType_t timeout);

/**
 * @brief Clear the alarm interrupt fired flag.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param intr_flag The interrupt(s) to clear.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_clear_intr_flag(DS3231_Cfg_t cfg, DS3231_Interrupt_t intr_flag, TickType_t timeout);

/**
 * @brief Get the status of the alarm interrupt.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] intr_flag The status of interrupt fired for each alarm. A bit set defines which alarm has fired.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_intr_flag(DS3231_Cfg_t cfg, DS3231_Interrupt_t* intr_flag, TickType_t timeout);

/**
 * @brief Set which alarm interrupts are enabled/disabled. If alarm interrupts are disabled then the interrupt control
 * bit is de-asserted.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param intr_flag Enable or disable alarm interrupts.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_intr_en(DS3231_Cfg_t cfg, DS3231_Interrupt_t intr_flag, TickType_t timeout);

/**
 * @brief Get which alarm interrupts are enabled.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] intr_flag The status of which alarm interrupts are enabled.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_intr_en(DS3231_Cfg_t cfg, DS3231_Interrupt_t* intr_flag, TickType_t timeout);

/**
 * @brief Write an aging offset to the DS3231.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param aging_offset The aging offset as it will be written to the DS3231.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_get_aging_offset(DS3231_Cfg_t cfg, uint8_t* aging_offset, TickType_t timeout);

/**
 * @brief Retrieve the current aging offset on the DS3231.
 * 
 * @param cfg The configuration of the DS3231 component.
 * @param[out] aging_offset The aging offset as read from the DS3231.
 * @param timeout The number of ticks to wait for the DS3231 to respond.
 * @return esp_err_t 
 */
esp_err_t ds3231_set_aging_offset(DS3231_Cfg_t cfg, uint8_t aging_offset, TickType_t timeout);

/**
 * @brief Free the resources used by the cfg parameter.
 * 
 * @param cfg The configuration of the DS3231 component to delete.
 */
void ds3231_delete(DS3231_Cfg_t cfg);

#endif // __DS3231_H__
