// rtc.c
// -----------------------------
// Manage Real-Time Clock (x86)

// This driver will only be used
// once on boot in order to determine
// the time of cold start.

#include "../../kernel_ports.h"
#include "../../kernel_types.h"
#include "../../messaging/messaging.h"
#include "../../helpers/string.h"

// Struct to make life easier
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_Time_t;

// Macro definitions for the registers utilised
// by this driver.
#define RTC_SEC      0x00
#define RTC_MIN      0x02
#define RTC_HOUR     0x04
#define RTC_DAY      0x07
#define RTC_MONTH    0x08
#define RTC_YEAR     0x09
// Although some BIOSes include the century at
// 0x10, it is **NOT** standard.
#define STATUS_REG_A 0x0A
#define STATUS_REG_B 0x0B

// @brief helper: read the CMOS registry of number reg
//
// @param reg number of register to read
// @return the value of said register
static uint8_t rtc_read_reg(uint8_t reg) {
  outb(0x70, reg);  // address port
  return inb(0x71); // data port
}

// @brief helper: check if there is an RTC Update In Progress
//
// @return boolean if in progress
static int rtc_update_in_progress() {
    // Check if bit 7 (Update In Progress) is set
    return (rtc_read_reg(STATUS_REG_A) & 0x80);
}

// Macro to convert BCD (Binary Coded Decimal) (8-bit) to decimal.
// Optimised implementation from https://wiki.osdev.org/CMOS#Format_of_Bytes
#define BCD_TO_BIN(bcd) ( (bcd & 0xF0) >> 1) + ( (bcd & 0xF0) >> 3) + (bcd & 0xf)

// @brief helper: convert CMOS time to 64-bit UNIX time
//
// @param y year
// @param m month
// @param d day
// @param h hours
// @param min minutes
// @param s seconds
//
// @return the 64-bit UNIX time
static inline int64_t rtc_conv_bios_to_unix64(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t s) {
    // Days in months for a non-leap year
    static const uint16_t accum_days[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    int64_t year = y;
    
    // Leap year count up to (year - 1)
    // Standard Gregorian leap year rules: divisible by 4, not 100 unless divisible by 400
    int64_t leaps = (year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400;
    
    // Days elapsed from year 0 to (year - 1)
    int64_t days_from_y0 = (year - 1) * 365 + leaps;

    // Unix epoch (1970-01-01) day count from year 0
    // 1970 * 365 + leap days up to 1969 = 719162 days
    static const int64_t unix_epoch_days = 719162;

    int64_t days_since_epoch = days_from_y0 - unix_epoch_days;

    // Add days for months elapsed in the current year
    int64_t month_days = accum_days[m - 1];
    
    // Add a leap day if current year is a leap year and we are past February
    int codon_is_leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
    if (codon_is_leap && m > 2) {
        month_days += 1;
    }

    days_since_epoch += month_days;

    // Add days for elapsed dates in the current month (d is 1-indexed)
    days_since_epoch += (d - 1);

    // Convert total days to seconds and add hours, minutes, seconds
    int64_t total_seconds = days_since_epoch * 86400LL;
    total_seconds += (int64_t)h * 3600LL;
    total_seconds += (int64_t)min * 60LL;
    total_seconds += s;

    return total_seconds;
}

// @brief Reads the time in 64-bit UNIX to *to. Requires timezone offset to be added later. Assume UTC.
//
// @param to pointer to the number to store the time
//
// @return 0 if success, 1 if failure.
int rtc_read_clock(int64_t *to) {
  // disable interrupts
  __asm__ __volatile__ ("cli" : : : "memory");

  message_send_message(" rtc: read\n");
  rtc_Time_t ret;
  bool_t is_pm = false;
  char reportbuf[256] = " rtc: The current time is ";

  while (rtc_update_in_progress())
	  ;
  uint8_t srb = rtc_read_reg(STATUS_REG_B);
  bool_t is_24h = (srb >> 1) & 1;
  bool_t is_bin = (srb >> 2) & 1;

  // First read
  ret.year = rtc_read_reg(RTC_YEAR);
  ret.month = rtc_read_reg(RTC_MONTH);
  ret.day = rtc_read_reg(RTC_DAY);
  ret.hour = rtc_read_reg(RTC_HOUR);
  ret.minute = rtc_read_reg(RTC_MIN);
  ret.second = rtc_read_reg(RTC_SEC);

  if (ret.hour & 0x80)
	  is_pm = true;
  if (!is_bin) {
    // convert BCD to binary
    ret.second = BCD_TO_BIN(ret.second);
    ret.minute = BCD_TO_BIN(ret.minute);
    ret.hour   = BCD_TO_BIN(ret.hour & 0x7F);
    ret.day    = BCD_TO_BIN(ret.day);
    ret.month  = BCD_TO_BIN(ret.month);
    ret.year   = BCD_TO_BIN(ret.year);
  } else {
    ret.hour = ret.hour & 0x7F; // clear is_pm bit
  }
  if (is_pm) {
    if (ret.hour != 12) {
      ret.hour += 12;
    } else {
      if (ret.hour == 0)
	      ret.hour = 0;
    }
  }
  ret.year += ret.year < 26 ? 2100 : 2000;
  append_uint(reportbuf, ret.year, sizeof(reportbuf));
  append_string(reportbuf, "-", sizeof(reportbuf));
  append_uint(reportbuf, ret.month, sizeof(reportbuf));
  append_string(reportbuf, "-", sizeof(reportbuf));
  append_uint(reportbuf, ret.day, sizeof(reportbuf));
  append_string(reportbuf, " ", sizeof(reportbuf));
  append_uint(reportbuf, ret.hour, sizeof(reportbuf));
  append_string(reportbuf, ":", sizeof(reportbuf));
  append_uint(reportbuf, ret.minute, sizeof(reportbuf));
  append_string(reportbuf, ":", sizeof(reportbuf));
  append_uint(reportbuf, ret.second, sizeof(reportbuf));
  append_string(reportbuf, "\n", sizeof(reportbuf));
  message_send_message(reportbuf);
  *to = rtc_conv_bios_to_unix64(ret.year, ret.month, ret.day, ret.hour, ret.minute, ret.second);
  // re-enable interrupts
  __asm__ __volatile__ ("sti" : : : "memory");
  return 0;
}
