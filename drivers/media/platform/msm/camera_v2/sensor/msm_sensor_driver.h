/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MSM_SENSOR_DRIVER_H
#define MSM_SENSOR_DRIVER_H

#include "msm_sensor.h"

int32_t msm_sensor_driver_probe(void *setting);

/*< DTS2014111305646 tangying/205982 20141113 begin*/
/*use dtsi get sensor name instead of board id string*/
int32_t msm_get_probe_sensor_codes(void *sensor_codos_info);
/*DTS2014111305646 tangying/205982 20141113 end >*/
#endif
