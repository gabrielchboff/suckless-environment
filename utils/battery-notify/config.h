/* See LICENSE file for copyright and license details. */

/* Battery glob pattern under /sys/class/power_supply/ */
#define BATTERY_GLOB    "BAT*"

/* Alert threshold for low battery notification */
#define LOW_THRESHOLD  20

/* Alert threshold for critical battery notification */
#define CRITICAL_THRESHOLD 5

/* Re-arm threshold after charging (hysteresis) */
#define REARM_THRESHOLD 25

/* State file to track current tier */
#define STATE_FILE "/tmp/battery-tier.state"