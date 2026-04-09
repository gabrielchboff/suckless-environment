/* See LICENSE file for copyright and license details. */

/* Battery device name under /sys/class/power_supply/ */
#define BATTERY    "BAT1"

/* Alert threshold percentage */
#define THRESHOLD  20

/* State file to prevent duplicate alerts */
#define STATE_FILE "/tmp/battery-notified"
