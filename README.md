# esp32-ds3231

esp32-ds3231 is ESP-IDF compatible component used to configure DS3231 chips. All chip features are available using this component.

## Setup

The esp32-ds3231 component does not configure the i2c peripheral of the ESP32, this step is required by the user as SDA/SCL pins are configurable based on project needs. To use the esp32-ds3231 component, first initialise an `DS3231_Cfg_t` object with `ds3231_create` with either the i2c port, either `I2C_NUM_0` or `I2C_NUM_1`. The configuration object is passed to each `ds3231_` function. When finished interacting with the DS3231, release resources used by the configuration object by calling `ds3231_delete`.

### Example
```c
  i2c_config_t i2c_config = 
  {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = SDA_PIN,
    .scl_io_num = SCL_PIN,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000,
  };
  i2c_param_config(I2C_NUM_0, &i2c_config);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
  DS3231_Cfg_t ds3231_cfg = ds3231_create(I2C_NUM_0);
  // interact with the DS3231
  ds3231_delete(ds3231_cfg);
```

## Setting / Getting the Date/Time
The date and time is configured and retrieved using the `DS3231_Calendar_t` structure.

### Setting Date/Time Example
```c
  DS3231_Calendar_t calendar =
  {
    .seconds = 0,
    .minutes = 0,
    .hour = 17,                             // Using 24h clock
    .clock_type = DS3231_ClockType_24_Hour, // can be DSD3231_ClockType_12_Hour
    // .am_pm = DS3231_PM                   // if clock_type == DS3231_ClockType_12_Hour, am_pm must also be set
    .day_of_week = 2,
    .day_of_month = 22,
    .month = 11,
    .year = 2021
  };

  esp_err_t res = ds3231_set_calendar(ds3231_cfg, &calendar, pdMS_TO_TICKS(10));
  printf("Result of setting calendar: %s\n", esp_err_to_name(res));
```

### Getting Date/Time Example
```c
  DS3231_Calendar_t calendar;
  esp_err_t res = ds3231_get_calendar(ds3231_cfg, &calendar, pdMS_TO_TICKS(10));
  if (res == ESP_OK)
    printf("%d/%02d/%02d %02d:%02d:%02d\n", calendar.year, calendar.month, calendar.day_of_month,
           calendar.hour, calendar.minutes, calendar.seconds);
  else
    printf("Error getting date/time: %s\n", esp_err_to_name(res));
```
## Configuring Alarms

There are two time-of-day alarms available in the DS3231. Alarm 1 supports configuration of seconds whereas alarm 2 does not. Both alarms are configured using the `DS3231_AlarmSetting_t` structure. Alarms are configured with `ds3231_set_alarm` and retrieved using `ds3231_get_alarm`. To retrieve alarm settings, the `alarm_type` member of `DS3231_AlarmSetting_t` must be set to either `DS3231_AlarmType_Alarm1` or `DS3231_AlarmType_Alarm2`.

### Example with Interrupts
```c
static void alarm_isr_top_half(void* arg)
{
  TaskHandle_t handle = (TaskHandle_t)arg;
  BaseType_t xHigherPriorityTaskWoken;
  xTaskNotifyFromISR(handle, 0, eNoAction, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void alarm_isr_bottom_half(void* arg)
{
  DS3231_Cfg_t ds3231_cfg = (DS3231_Cfg_t)arg;
  while(1)
  {
    uint32_t notif_value;
    if(xTaskNotifyWait(pdTRUE, pdTRUE, &notif_value, portMAX_DELAY))
    {
      DS3231_Interrupt_t intr_flag;
      ds3231_get_intr_flag(ds3231_cfg, &intr_flag, pdMS_TO_TICKS(10));
      printf("Received alarm - %02x\n", intr_flag);
      esp_err_t res = ds3231_clear_intr_flag(ds3231_cfg, intr_flag, pdMS_TO_TICKS(10));
      printf("Clearing interrupt resulted in %s\n", esp_err_to_name(res));
    }
  }
}

void app_main(void)
{
  // ... i2c and DS3231 configuration
  
  DS3231_AlarmSetting_t alarm =
  {
    .seconds = 0,
    .minutes = 45,
    .hour = 10,
    .day = 1,                                   // Matching on Hours, Minutes, and Seconds, day is irrelevant
    .clock_type = DS3231_ClockType_12_Hour,     // Alarm is in 12-hour clock even though DS3231 could be using 24-hour clock
    .am_pm = DS3231_PM,                         // Alarm is configured for 10:45 pm
    .day_type = DS3231_AlarmDayType_DayOfMonth, // Can also be DS3231_AlarmDayType_DayOfWeek
    .alarm_rate = DS3231_AlarmRate_HMS_Match,   // Supports all rates as described in the data sheet
    .alarm_type = DS3231_AlarmType_Alarm1       // Configuring alarm 1, seconds are relevant. Alarm 2 is DS3231_AlarmType_Alarm2
  };
  
  // create a task to act as the alarm interrupt 'bottom half'
  TaskHandle_t handle;
  xTaskCreate(&alarm_isr_bottom_half, "Alarm Bottom Half", 2048, ds3231_cfg, 5, &handle);

  // configure GPIO and install ISR
  gpio_pad_select_gpio(ALARM_PIN);
  gpio_set_direction(ALARM_PIN, GPIO_MODE_INPUT);
  gpio_set_intr_type(ALARM_PIN, GPIO_INTR_NEGEDGE);   // Interrupt pin of DS3231 is active low
  gpio_install_isr_service(0);
  gpio_isr_handler_add(ALARM_PIN, alarm_isr_top_half, (void*)handle);
  
  ds3231_set_intr_en(ds3231_cfg, DS3231_Interrupt_None, pdMS_TO_TICKS(10));         // disable interrupts
  ds3231_clear_intr_flag(ds3231_cfg, DS3231_Interrupt_Alarm_1, pdMS_TO_TICKS(10));  // clear interrupt flags
  ds3231_set_alarm(ds3231_cfg, &alarm, pdMS_TO_TICKS(10));                          // configure the alarm
  ds3231_set_intr_en(ds3231_cfg, DS3231_Interrupt_Alarm_1, pdMS_TO_TICKS(10));      // enable interrupt for alarm 1
}
```
## Reading Temperature
The esp32-ds3231 component provides ability to read the temperature register and return the value as in floating point representation. Negative values should be correctly converted even though that functionality hasn't been tested.

## Example
```c
  float temperature;
  esp_err_t res = ds3231_get_temperature(ds3231_cfg, &temperature, pdMS_TO_TICKS(10));
  if (res == ESP_OK)
    printf("Temperature: %fdegC\n", temperature);
  else
    printf("Error reading temperature: %s\n", esp_err_to_name(res));
```

## Configuring Waveform Generation
The DS3231 chip provides two waveform generation outputs. The first is a frequency configurable square wave. The second is a fixed 32kHz square wave.

### Square Wave Generation
Square wave generation on pin 3 can be configured to produce a waveform at frequencies of 1Hz, 1024Hz, 2048Hz, 4096Hz, and 8192Hz. Since pin 3 is has a shared purpose of also being the interrupt pin, interrupts must be disabled to enable square wave generation. Note that the DS3231M only supports 1Hz.

#### Example

```c
  // set square wave generation to 1024Hz
  ds3231_set_square_wave(ds3231_cfg, DS3231_SquareWave_1024Hz, pdMS_TO_TICKS(10));
  
  // disable square wave generation
  ds3231_set_square_wave(ds3231_cfg, DS3231_SquareWave_Off, pdMS_TO_TICKS(10));
  
  // get square wave generation frequency
  DS3231_SquareWave_t square_wave;
  ds3231_get_square_wave(ds3231_cfg, &square_wave, pdMS_TO_TICKS(10));
```

### 32kHz Waveform
The 32kHz waveform generator on pin 1 can either be enabled or disabled.

#### Example

```c
  DS3231_32kHz_t en32kHz;
  ds3231_get_32kHz(ds3231_cfg, &en32kHz, pdMS_TO_TICKS(10));
  ds3231_set_32kHz(ds3231_cfg, !en32kHz, pdMS_TO_TICKS(10)); // Toggle waveform generation
```

## Other Functionality

The esp32-ds3231 supports the full control of the DS3231 chip. Following is a list of other functions provided.

- `ds3231_set_convert_temperature` - Start temperature conversion process
- `ds3231_get_convert_temperature` - Check on temperature conversion process
- `ds3231_set_osc` - Enable or disable the oscillator
- `ds3231_get_osc` - Get the oscillator status
- `ds3231_get_osc_stop_flag` - Get flag indicating whether oscillator has been stopped
- `ds3231_clear_osc_stop_flag` - Clear the oscillator stopped flag
- `ds3231_is_busy` - Return the status of the busy bit in the status register
- `ds3231_set_aging_offset` - Set the aging offset trim
- `ds3231_get_aging_offset` - Get the aging offset trim
