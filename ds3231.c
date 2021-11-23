#include <ds3231.h>
#include <stdlib.h>

#define DS3231_ADDR     0x68
#define DS3231_CAL_REG  0x00
#define DS3231_ALM1_REG 0x07
#define DS3231_ALM2_REG 0x0B
#define DS3231_CTRL_REG 0x0E
#define DS3231_CS_REG   0x0F
#define DS3231_AGE_REG  0x10
#define DS3231_TEMP_REG 0x11

struct DS3231_Cfg
{
  i2c_port_t i2c_port;
};

typedef struct _internal_ds3231_calendar_s
{
  uint8_t seconds_1s  : 4;
  uint8_t seconds_10s : 3;
  uint8_t             : 1;

  uint8_t minutes_1s  : 4;
  uint8_t minutes_10s : 3;
  uint8_t             : 1;

  uint8_t hour_1s         : 4;
  uint8_t hour_10s        : 1;
  uint8_t am_pm_hour_20   : 1; // if mode_12_24h == 1, then AM/PM, else 20 hour
  uint8_t mode_12_24h     : 1; // if 0 -> 24h clock, 1 -> 12h clock
  uint8_t                 : 1;

  uint8_t day_of_week : 3;
  uint8_t             : 5;

  uint8_t day_of_month_1s   : 4;
  uint8_t day_of_month_10s  : 2;
  uint8_t                   : 2;

  uint8_t month_1s    : 4;
  uint8_t month_10s   : 1;
  uint8_t             : 2;
  uint8_t century     : 1;

  uint8_t year_1s     : 4;
  uint8_t year_10s    : 4;
} __attribute__((packed)) Internal_DS3231_Calendar_t;

typedef struct _internal_ds3231_alarm1_s
{
  uint8_t seconds_1s  : 4;
  uint8_t seconds_10s : 3;
  uint8_t a1m1        : 1;

  uint8_t minutes_1s  : 4;
  uint8_t minutes_10s : 3;
  uint8_t a1m2        : 1;

  uint8_t hour_1s      : 4;
  uint8_t hour_10s     : 1;
  uint8_t am_pm_hour_20 : 1; // if mode_12_24h == 1, then AM/PM, else 20 hour
  uint8_t mode_12_24h   : 1; // if 0 -> 24h clock, 1 -> 12h clock
  uint8_t a1m3          : 1;

  uint8_t day_1s                : 4;
  uint8_t day_10s               : 2;
  uint8_t day_of_week_or_month  : 1;
  uint8_t a1m4                  : 1;
} __attribute__((packed)) Internal_DS3231_Alarm1_t;

typedef struct _internal_ds3231_alarm2_s
{
  uint8_t minutes_1s  : 4;
  uint8_t minutes_10s : 3;
  uint8_t a2m2        : 1;

  uint8_t hour_1s      : 4;
  uint8_t hour_10s     : 1;
  uint8_t am_pm_hour_20 : 1;
  uint8_t mode_12_24h   : 1;
  uint8_t a2m3          : 1;

  uint8_t day_1s                : 4;
  uint8_t day_10s               : 2;
  uint8_t day_of_week_or_month  : 1;
  uint8_t a2m4                  : 1;
} __attribute__((packed)) Internal_DS3231_Alarm2_t;

typedef struct _internal_ds3231_control_s
{
  uint8_t alarm1_intr_en  : 1;
  uint8_t alarm2_intr_en  : 1;
  uint8_t intr_control    : 1;
  uint8_t rs              : 2;
  uint8_t conv            : 1;
  uint8_t bbsqw           : 1;
  uint8_t osc_en_n        : 1;
} __attribute__((packed)) Internal_DS3231_Control_t;

typedef struct _internal_ds3231_ctrlstat_s
{
  uint8_t a1f     : 1;
  uint8_t a2f     : 1;
  uint8_t bsy     : 1;
  uint8_t en32kHz : 1;
  uint8_t         : 3;
  uint8_t osf     : 1;
} __attribute__((__packed__)) Internal_DS3231_CtrlStat_t;

static void ds3231_convert_ext_calendar(DS3231_Calendar_t* in, Internal_DS3231_Calendar_t* out);
static void ds3231_convert_int_calendar(DS3231_Calendar_t* out, Internal_DS3231_Calendar_t* in);
static esp_err_t ds3231_i2c_read(DS3231_Cfg_t cfg, uint8_t reg, uint8_t* data, size_t data_len, TickType_t);
static esp_err_t ds3231_i2c_write(DS3231_Cfg_t cfg, uint8_t reg, uint8_t* data, size_t data_len, TickType_t);
static esp_err_t ds3231_get_alarm1(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout);
static esp_err_t ds3231_get_alarm2(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout);
static esp_err_t ds3231_set_alarm1(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout);
static esp_err_t ds3231_set_alarm2(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout);

static inline esp_err_t ds3231_get_ctrl(DS3231_Cfg_t cfg, Internal_DS3231_Control_t* ctrl, TickType_t timeout)
{
  return ds3231_i2c_read(cfg, DS3231_CTRL_REG, (uint8_t*)ctrl, sizeof(*ctrl), timeout);
}

static inline esp_err_t ds3231_set_ctrl(DS3231_Cfg_t cfg, Internal_DS3231_Control_t* ctrl, TickType_t timeout)
{
  return ds3231_i2c_write(cfg, DS3231_CTRL_REG, (uint8_t*)ctrl, sizeof(*ctrl), timeout);
}

static inline esp_err_t ds3231_get_cs(DS3231_Cfg_t cfg, Internal_DS3231_CtrlStat_t* ctrl_status, TickType_t timeout)
{
  return ds3231_i2c_read(cfg, DS3231_CS_REG, (uint8_t*)ctrl_status, sizeof(*ctrl_status), timeout);
}

static inline esp_err_t ds3231_set_cs(DS3231_Cfg_t cfg, Internal_DS3231_CtrlStat_t* ctrl_status, TickType_t timeout)
{
  return ds3231_i2c_write(cfg, DS3231_CS_REG, (uint8_t*)ctrl_status, sizeof(*ctrl_status), timeout);
}

DS3231_Cfg_t ds3231_create(i2c_port_t i2c_port)
{
  DS3231_Cfg_t cfg = (DS3231_Cfg_t)malloc(sizeof(*cfg));
  if (cfg)
    cfg->i2c_port = i2c_port;

  return cfg;
}

esp_err_t ds3231_get_calendar(DS3231_Cfg_t cfg, DS3231_Calendar_t* calendar, TickType_t timeout)
{
  Internal_DS3231_Calendar_t int_calendar;
  esp_err_t res = ds3231_i2c_read(cfg, DS3231_CAL_REG, (uint8_t*)&int_calendar, sizeof(int_calendar), timeout);
  if (res == ESP_OK)
    ds3231_convert_int_calendar(calendar, &int_calendar);
  return res;
}

esp_err_t ds3231_set_calendar(DS3231_Cfg_t cfg, DS3231_Calendar_t* calendar, TickType_t timeout)
{
  Internal_DS3231_Calendar_t int_calendar;
  ds3231_convert_ext_calendar(calendar, &int_calendar);
  return ds3231_i2c_write(cfg, DS3231_CAL_REG, (uint8_t*)&int_calendar, sizeof(int_calendar), timeout);
}

esp_err_t ds3231_get_temperature(DS3231_Cfg_t cfg, float* temperature, TickType_t timeout)
{
  uint8_t temp_data[2];
  esp_err_t res = ds3231_i2c_read(cfg, DS3231_TEMP_REG, temp_data, sizeof(temp_data), timeout);

  if (res == ESP_OK && temperature)
  {
    uint16_t temp = (temp_data[0] << 2) | (temp_data[1] >> 6);
    if (temp & 0x200)
    {
      // if the 10th bit (sign bit) is asserted then assert bits 11-15
      // this will convert to proper, negative int16_t value.
      temp |= 0xFC00;
    }

    *temperature = (int16_t)temp * 0.25f;
  }

  return res;
}

esp_err_t ds3231_get_alarm(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout)
{
  if (alarm->alarm_type == DS3231_AlarmType_Alarm1)
    return ds3231_get_alarm1(cfg, alarm, timeout);
  else if (alarm->alarm_type == DS3231_AlarmType_Alarm2)
    return ds3231_get_alarm2(cfg, alarm, timeout);
  else
    return ESP_ERR_INVALID_ARG;
}

esp_err_t ds3231_set_alarm(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout)
{
  if (alarm->alarm_type == DS3231_AlarmType_Alarm1)
    return ds3231_set_alarm1(cfg, alarm, timeout);
  else if (alarm->alarm_type == DS3231_AlarmType_Alarm2)
    return ds3231_set_alarm2(cfg, alarm, timeout);
  else
    return ESP_ERR_INVALID_ARG;
}

esp_err_t ds3231_get_intr_en(DS3231_Cfg_t cfg, DS3231_Interrupt_t* intr_flag, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res == ESP_OK)
  {
    *intr_flag = DS3231_Interrupt_None;
    *intr_flag |= ctrl.alarm1_intr_en ? DS3231_Interrupt_Alarm_1 : 0;
    *intr_flag |= ctrl.alarm2_intr_en ? DS3231_Interrupt_Alarm_2 : 0;
  }

  return res;
}

esp_err_t ds3231_set_intr_en(DS3231_Cfg_t cfg, DS3231_Interrupt_t intr_flags, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl,timeout);
  if (res == ESP_OK)
  {
    ctrl.intr_control = intr_flags != 0;
    ctrl.alarm1_intr_en = (intr_flags & DS3231_Interrupt_Alarm_1) == DS3231_Interrupt_Alarm_1;
    ctrl.alarm2_intr_en = (intr_flags & DS3231_Interrupt_Alarm_2) == DS3231_Interrupt_Alarm_2;
  }

  return ds3231_set_ctrl(cfg, &ctrl, timeout);
}

esp_err_t ds3231_set_square_wave(DS3231_Cfg_t cfg, DS3231_SquareWave_t sqw, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res != ESP_OK)
    return res;

  if (sqw == DS3231_SquareWave_Off)
  {
    ctrl.bbsqw = 0;
  }
  else
  {
    ctrl.rs = sqw;
    ctrl.bbsqw = 1;
  }

  return ds3231_set_ctrl(cfg, &ctrl, timeout);
}

esp_err_t ds3231_get_square_wave(DS3231_Cfg_t cfg, DS3231_SquareWave_t* square_wave_setting, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res == ESP_OK)
    *square_wave_setting = ctrl.bbsqw ? ctrl.rs : DS3231_SquareWave_Off;

  return ESP_OK;
}

esp_err_t ds3231_set_convert_temperature(DS3231_Cfg_t cfg, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res != ESP_OK)
    return res;

  ctrl.conv = 1;
  return ds3231_set_ctrl(cfg, &ctrl, timeout);
}

esp_err_t ds3231_get_convert_temperature(DS3231_Cfg_t cfg, uint8_t* conv, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res == ESP_OK)
    *conv = ctrl.conv;
  return res;
}

esp_err_t ds3231_get_osc(DS3231_Cfg_t cfg, DS3231_Oscillator_t* eosc, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res != ESP_OK)
    return res;

  *eosc = ctrl.osc_en_n;
  return ESP_OK;
}

esp_err_t ds3231_set_osc(DS3231_Cfg_t cfg, DS3231_Oscillator_t eosc, TickType_t timeout)
{
  Internal_DS3231_Control_t ctrl;
  esp_err_t res = ds3231_get_ctrl(cfg, &ctrl, timeout);
  if (res != ESP_OK)
    return res;

  ctrl.osc_en_n = eosc;
  return ds3231_set_ctrl(cfg, &ctrl, timeout);
}

esp_err_t ds3231_get_32kHz(DS3231_Cfg_t cfg, DS3231_32kHz_t* en32kHz, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t cs;
  esp_err_t res = ds3231_get_cs(cfg, &cs, timeout);
  if (res == ESP_OK)
    *en32kHz = cs.en32kHz;
  return res; 
}

esp_err_t ds3231_set_32kHz(DS3231_Cfg_t cfg, DS3231_32kHz_t en32kHz, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t cs;
  esp_err_t res = ds3231_get_cs(cfg, &cs, timeout);
  if (res != ESP_OK)
    return res;

  cs.en32kHz = en32kHz;
  return ds3231_set_cs(cfg, &cs, timeout);
}

esp_err_t ds3231_is_busy(DS3231_Cfg_t cfg, uint8_t* busy, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t cs;
  esp_err_t res = ds3231_get_cs(cfg, &cs, timeout);
  if (res == ESP_OK)
    *busy = cs.bsy;
  return res;
}

esp_err_t ds3231_get_osc_stop_flag(DS3231_Cfg_t cfg, uint8_t* osc_stop_flag, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t cs;
  esp_err_t res = ds3231_get_cs(cfg, &cs, timeout);
  if (res != ESP_OK)
    return res;

  *osc_stop_flag = cs.osf;
  return ESP_OK;
}

esp_err_t ds3231_clear_osc_stop_flag(DS3231_Cfg_t cfg, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t cs;
  esp_err_t res = ds3231_get_cs(cfg, &cs, timeout);
  if (res != ESP_OK)
    return res;

  cs.osf = 0;
  return ds3231_set_cs(cfg, &cs, timeout);
}

esp_err_t ds3231_get_intr_flag(DS3231_Cfg_t cfg, DS3231_Interrupt_t* intr_flag, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t ctrl_status;
  int res = ds3231_get_cs(cfg, &ctrl_status, timeout);
  if (res == ESP_OK)
  {
    *intr_flag = DS3231_Interrupt_None;
    *intr_flag |= ctrl_status.a1f ? DS3231_Interrupt_Alarm_1 : 0;
    *intr_flag |= ctrl_status.a2f ? DS3231_Interrupt_Alarm_2 : 0;
  }

  return res;
}

esp_err_t ds3231_clear_intr_flag(DS3231_Cfg_t cfg, DS3231_Interrupt_t intr_flags, TickType_t timeout)
{
  Internal_DS3231_CtrlStat_t ctrl_status;
  esp_err_t res = ds3231_get_cs(cfg, &ctrl_status, timeout);
  if (res != ESP_OK)
    return res;

  ctrl_status.a1f = (intr_flags & DS3231_Interrupt_Alarm_1) == DS3231_Interrupt_Alarm_1 ? 0 : ctrl_status.a1f;
  ctrl_status.a2f = (intr_flags & DS3231_Interrupt_Alarm_2) == DS3231_Interrupt_Alarm_2 ? 0 : ctrl_status.a2f;

  return ds3231_set_cs(cfg, &ctrl_status, timeout);
}

esp_err_t ds3231_get_aging_offset(DS3231_Cfg_t cfg, uint8_t* aging_offset, TickType_t timeout)
{
  return ds3231_i2c_read(cfg, DS3231_AGE_REG, aging_offset, sizeof(*aging_offset), timeout);
}

esp_err_t ds3231_set_aging_offset(DS3231_Cfg_t cfg, uint8_t aging_offset, TickType_t timeout)
{
  return ds3231_i2c_write(cfg, DS3231_AGE_REG, &aging_offset, sizeof(aging_offset), timeout);
}

void ds3231_delete(DS3231_Cfg_t cfg)
{
  if (cfg)
    free(cfg);
}

static esp_err_t ds3231_i2c_read(DS3231_Cfg_t cfg, uint8_t reg, uint8_t* data, size_t data_len, TickType_t timeout)
{
  i2c_cmd_handle_t i2c_cmd_handle = i2c_cmd_link_create();
  i2c_master_start(i2c_cmd_handle);
  i2c_master_write_byte(i2c_cmd_handle, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(i2c_cmd_handle, reg, true);
  i2c_master_start(i2c_cmd_handle);
  i2c_master_write_byte(i2c_cmd_handle, (DS3231_ADDR << 1) | I2C_MASTER_READ, true);
  i2c_master_read(i2c_cmd_handle, data, data_len, I2C_MASTER_LAST_NACK);
  i2c_master_stop(i2c_cmd_handle);
  esp_err_t res = i2c_master_cmd_begin(cfg->i2c_port, i2c_cmd_handle, timeout);
  i2c_cmd_link_delete(i2c_cmd_handle);

  return res;
}

static esp_err_t ds3231_i2c_write(DS3231_Cfg_t cfg, uint8_t reg, uint8_t* data, size_t data_len, TickType_t timeout)
{
  i2c_cmd_handle_t i2c_cmd_handle = i2c_cmd_link_create();
  i2c_master_start(i2c_cmd_handle);
  i2c_master_write_byte(i2c_cmd_handle, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(i2c_cmd_handle, reg, true);
  i2c_master_write(i2c_cmd_handle, data, data_len, true);
  i2c_master_stop(i2c_cmd_handle);
  esp_err_t res = i2c_master_cmd_begin(cfg->i2c_port, i2c_cmd_handle, timeout);
  i2c_cmd_link_delete(i2c_cmd_handle);

  return res;
}

static void ds3231_convert_ext_calendar(DS3231_Calendar_t* in, Internal_DS3231_Calendar_t* out)
{
  out->seconds_1s = in->seconds % 10;
  out->seconds_10s = in->seconds / 10;
  out->minutes_1s = in->minutes % 10;
  out->minutes_10s = in->minutes / 10;
  out->hour_1s = in->hour % 10;
  if (in->clock_type == DS3231_ClockType_12_Hour)
  {
    out->mode_12_24h = 1;
    out->am_pm_hour_20 = in->am_pm == DS3231_PM;
    out->hour_10s = in->hour / 10;
  }
  else
  {
    out->mode_12_24h = 0;
    if (in->hour > 19)
    {
      out->am_pm_hour_20 = 1;
      out->hour_10s = 0;
    }
    else if (in->hour > 9)
    {
      out->am_pm_hour_20 = 0;
      out->hour_10s = in->hour / 10;
    }
  }

  out->day_of_week = in->day_of_week;
  out->day_of_month_1s = in->day_of_month % 10;
  out->day_of_month_10s = in->day_of_month / 10;

  out->month_1s = in->month % 10;
  out->month_10s = in->month / 10;

  uint16_t year = in->year; 
  if (year > 2099)
  {
    out->century = 1;
    year -= 2100;
  }
  else
  {
    out->century = 0;
    year -= 2000;
  }

  out->year_1s = year % 10;
  out->year_10s = year / 10;
}

static void ds3231_convert_int_calendar(DS3231_Calendar_t* out, Internal_DS3231_Calendar_t* in)
{
  out->seconds = in->seconds_10s * 10 + in->seconds_1s;
  out->minutes = in->minutes_10s * 10 + in->minutes_1s;
  if (in->mode_12_24h)
  {
    out->clock_type = DS3231_ClockType_12_Hour;
    out->hour = in->hour_10s * 10 + in->hour_1s;
    out->am_pm = in->am_pm_hour_20 ? DS3231_PM : DS3231_AM;
  }
  else
  {
    out->clock_type = DS3231_ClockType_24_Hour;
    out->hour = in->am_pm_hour_20 * 20 + in->hour_10s * 10 + in->hour_1s;
  }

  out->day_of_week = in->day_of_week;
  out->day_of_month = in->day_of_month_10s * 10 + in->day_of_month_1s;
  out->month = in->month_10s * 10 + in->month_1s;
  out->year = 2000 + in->century * 100 + in->year_10s * 10 + in->year_1s;
}

static esp_err_t ds3231_get_alarm1(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout)
{
  Internal_DS3231_Alarm1_t alarm1;
  esp_err_t res = ds3231_i2c_read(cfg, DS3231_ALM1_REG, (uint8_t*)&alarm1, sizeof(alarm1), timeout);
  if (res == ESP_OK)
  {
    alarm->seconds = alarm1.seconds_10s * 10 + alarm1.seconds_1s;
    alarm->minutes = alarm1.minutes_10s * 10 + alarm1.minutes_1s;

    if (alarm1.mode_12_24h)
    {
      alarm->clock_type = DS3231_ClockType_12_Hour;
      alarm->am_pm = alarm1.am_pm_hour_20 ? DS3231_PM : DS3231_AM;
      alarm->hour = alarm1.hour_10s * 10 + alarm1.hour_1s;
    }
    else
    {
      alarm->clock_type = DS3231_ClockType_24_Hour;
      alarm->hour = alarm1.am_pm_hour_20 * 20 + alarm1.hour_10s * 10 + alarm1.hour_1s;
    }

    alarm->day = alarm1.day_10s * 10 + alarm1.day_1s;
    alarm->day_type = alarm1.day_of_week_or_month ? DS3231_AlarmDayType_DayOfWeek : DS3231_AlarmDayType_DayOfMonth;

    alarm->alarm_rate = alarm1.a1m4 << 3 | alarm1.a1m3 << 2 | alarm1.a1m2 << 1 | alarm1.a1m1;
  }

  return res;
}

static esp_err_t ds3231_get_alarm2(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout)
{
  Internal_DS3231_Alarm2_t alarm2;
  esp_err_t res = ds3231_i2c_read(cfg, DS3231_ALM1_REG, (uint8_t*)&alarm2, sizeof(alarm2), timeout);
  if (res == ESP_OK)
  {
    alarm->seconds = 0;
    alarm->minutes = alarm2.minutes_10s * 10 + alarm2.minutes_1s;

    if (alarm2.mode_12_24h)
    {
      alarm->clock_type = DS3231_ClockType_12_Hour;
      alarm->am_pm = alarm2.am_pm_hour_20 ? DS3231_PM : DS3231_AM;
      alarm->hour = alarm2.hour_10s * 10 + alarm2.hour_1s;
    }
    else
    {
      alarm->clock_type = DS3231_ClockType_24_Hour;
      alarm->hour = alarm2.am_pm_hour_20 * 20 + alarm2.hour_10s * 10 + alarm2.hour_1s;
    }

    alarm->day = alarm2.day_10s * 10 + alarm2.day_1s;
    alarm->day_type = alarm2.day_of_week_or_month ? DS3231_AlarmDayType_DayOfWeek : DS3231_AlarmDayType_DayOfMonth;

    alarm->alarm_rate = alarm2.a2m4 << 2 | alarm2.a2m3 << 1 | alarm2.a2m2;
  }

  return res;
}
static esp_err_t ds3231_set_alarm1(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout)
{
  Internal_DS3231_Alarm1_t alarm1;

  alarm1.seconds_1s = alarm->seconds % 10;
  alarm1.seconds_10s = alarm->seconds / 10;
  alarm1.a1m1 = (alarm->alarm_rate & 0b0001) == 0b0001;

  alarm1.minutes_1s = alarm->minutes % 10;
  alarm1.minutes_10s = alarm->minutes / 10;
  alarm1.a1m2 = (alarm->alarm_rate & 0b0010) == 0b0010;

  alarm1.hour_1s = alarm->hour % 10;
  if (alarm->clock_type == DS3231_ClockType_12_Hour)
  {
    alarm1.hour_10s = alarm->hour / 10;
    alarm1.am_pm_hour_20 = alarm->am_pm == DS3231_PM;
  }
  else
  {
    alarm1.mode_12_24h = 0;
    alarm1.a1m3 = (alarm->alarm_rate & 0b0100) == 0b0100;
    if (alarm->hour > 19)
    {
      alarm1.am_pm_hour_20 = 1;
      alarm1.hour_10s = 0;
    }
    else if (alarm->hour > 9)
    {
      alarm1.am_pm_hour_20 = 0;
      alarm1.hour_10s = alarm->hour / 10;
    }
    else
    {
      alarm1.hour_10s = 0;
    }
  }

  alarm1.day_of_week_or_month = alarm->day_type == DS3231_AlarmDayType_DayOfMonth;
  alarm1.day_1s = alarm->day % 10;
  alarm1.day_1s = alarm->day / 10;
  alarm1.a1m4 = (alarm->alarm_rate & 0b1000) == 0b1000;

  return ds3231_i2c_write(cfg, DS3231_ALM1_REG, (uint8_t*)&alarm1, sizeof(alarm1), timeout);
}

static esp_err_t ds3231_set_alarm2(DS3231_Cfg_t cfg, DS3231_AlarmSetting_t* alarm, TickType_t timeout)
{
  Internal_DS3231_Alarm2_t alarm2;

  alarm2.minutes_1s = alarm->minutes % 10;
  alarm2.minutes_10s = alarm->minutes / 10;
  alarm2.a2m2 = (alarm->alarm_rate & 0b001) == 0b001;

  alarm2.hour_1s = alarm->hour % 10;
  if (alarm->clock_type == DS3231_ClockType_12_Hour)
  {
    alarm2.hour_10s = alarm->hour / 10;
    alarm2.am_pm_hour_20 = alarm->am_pm == DS3231_PM;
  }
  else
  {
    alarm2.mode_12_24h = 0;
    alarm2.a2m3 = (alarm->alarm_rate & 0b010) == 0b010;
    if (alarm->hour > 19) // 20-23
    {
      alarm2.am_pm_hour_20 = 1;
      alarm2.hour_10s = 0;
    }
    else if (alarm->hour > 9) //10-19
    {
      alarm2.am_pm_hour_20 = 0;
      alarm2.hour_10s = alarm->hour / 10;
    }
    else // 1-9
    {
      alarm2.hour_10s = 0;
    }
  }

  alarm2.day_of_week_or_month = alarm->day_type == DS3231_AlarmDayType_DayOfWeek;
  alarm2.day_1s = alarm->day % 10;
  alarm2.day_10s = alarm->day / 10;
  alarm2.a2m4 = (alarm->alarm_rate & 0b100) == 0b100;

  return ds3231_i2c_write(cfg, DS3231_ALM2_REG, (uint8_t*)&alarm2, sizeof(alarm2), timeout);
}