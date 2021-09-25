/* rtc.c -- This file is part of PML.
   Copyright (C) 2021 XNSC

   PML is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   PML is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PML. If not, see <https://www.gnu.org/licenses/>. */

#include <pml/cmos.h>
#include <string.h>

enum
{
  second,
  minute,
  hour,
  day,
  month,
  year,
  data_len
};

static unsigned char days_per_month[] = {
  31, /* January */
  28, /* February */
  31, /* March */
  30, /* April */
  31, /* May */
  30, /* June */
  31, /* July */
  31, /* August */
  30, /* September */
  31, /* October */
  30, /* November */
  31  /* December */
};

static void
cmos_wait_update (void)
{
  while (cmos_read_register (CMOS_REG_STATUS_A) & 0x80)
    ;
}

time_t
cmos_read_real_time (void)
{
  unsigned char data[data_len];
  unsigned char old_data[data_len];
  unsigned char status_b;
  unsigned char i;
  unsigned int doy;
  unsigned int posix_year;

  cmos_wait_update ();
  data[second] = cmos_read_register (CMOS_REG_SECONDS);
  data[minute] = cmos_read_register (CMOS_REG_MINUTES);
  data[hour] = cmos_read_register (CMOS_REG_HOURS);
  data[day] = cmos_read_register (CMOS_REG_DAY_OF_MONTH);
  data[month] = cmos_read_register (CMOS_REG_MONTH);
  data[year] = cmos_read_register (CMOS_REG_YEAR);

  /* Make sure we get the same values twice in a row */
  do
    {
      memcpy (old_data, data, data_len);
      cmos_wait_update ();
      data[second] = cmos_read_register (CMOS_REG_SECONDS);
      data[minute] = cmos_read_register (CMOS_REG_MINUTES);
      data[hour] = cmos_read_register (CMOS_REG_HOURS);
      data[day] = cmos_read_register (CMOS_REG_DAY_OF_MONTH);
      data[month] = cmos_read_register (CMOS_REG_MONTH);
      data[year] = cmos_read_register (CMOS_REG_YEAR);
    }
  while (data[second] != old_data[second]
	 || data[minute] != old_data[minute]
	 || data[hour] != old_data[hour]
	 || data[day] != old_data[day]
	 || data[month] != old_data[month]
	 || data[year] != old_data[year]);

  /* Adjust read values if necessary */
  status_b = cmos_read_register (CMOS_REG_STATUS_B);
  if (!(status_b & CMOS_STATUS_B_BCD))
    {
      data[second] = (data[second] & 0x0f) + data[second] / 16 * 10;
      data[minute] = (data[minute] & 0x0f) + data[minute] / 16 * 10;
      data[hour] = ((data[hour] & 0x0f) + (data[hour] & 0x70) / 16 * 10)
	| (data[hour] & 0x80);
      data[day] = (data[day] & 0x0f) + data[day] / 16 * 10;
      data[month] = (data[month] & 0x0f) + data[month] / 16 * 10;
      data[year] = (data[year] & 0x0f) + data[year] / 16 * 10;
    }
  if (!(status_b & CMOS_STATUS_B_12H) && (data[hour] & 0x80))
    data[hour] = ((data[hour] & 0x7f) + 12) % 24;

  posix_year = data[year] + 100; /* Assume 21st century */
  doy = data[day] - 1;
  for (i = 0; i < data[month] - 1; i++)
    doy += days_per_month[i];
  return data[second] + data[minute] * 60 + data[hour] * 3600 + doy * 86400 +
    (posix_year - 70) * 31536000 + (posix_year - 69) / 4 * 86000 -
    (posix_year - 1) / 100 * 86400 + (posix_year + 299) / 400 * 86400;
}
