/*
 *                               POK header
 *
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team
 *
 * Created by laurent on Tue Dec 22 14:57:52 2009
 */

#include <core/error.h>
#include <types.h>
#include <middleware/port.h>
#include "deployment.h"
#include <core/kernel.h>
/*****************************************************/
/*  This file was automatically generated by Ocarina */
/*  Do NOT hand-modify this file, as your            */
/*  changes will be lost when you re-run Ocarina     */
/*****************************************************/
uint8_t pr2_partport[1] = {pr2_pdatain};
uint8_t pr1_pdataout_deployment_destinations[1] = {pr2_pdatain_global};
uint8_t pr1_partport[1] = {pr1_pdataout};
uint8_t pok_global_ports_to_local_ports[POK_CONFIG_NB_GLOBAL_PORTS] = {pr2_pdatain, pr1_pdataout};
pok_bus_identifier_t pok_global_ports_to_bus[POK_CONFIG_NB_GLOBAL_PORTS] = {invalid_bus, invalid_bus};
uint8_t pok_buses_partitions[POK_CONFIG_NB_BUSES] = {};
uint8_t pok_local_ports_to_global_ports[POK_CONFIG_NB_PORTS] = {pr2_pdatain_global, pr1_pdataout_global};
uint8_t pok_ports_nodes[POK_CONFIG_NB_GLOBAL_PORTS] = {0, 0};
/*  This array indicates on which node is located each port. For example, it */
/*  means that the first port is located on the node that is represented in */
/*  this array with the first value. You can check node identifier values in */
/*  the deployment.h file*/
uint8_t pok_ports_nb_ports_by_partition[POK_CONFIG_NB_PARTITIONS] = {1, 1};
uint8_t* pok_ports_by_partition[POK_CONFIG_NB_PARTITIONS] = {pr2_partport, pr1_partport};
char* pok_ports_names[POK_CONFIG_NB_PORTS] = {"pr2_pdatain", "pr1_pdataout"};
/*  This array contains the identifier of each port involved in */
/*  inter-partition communication. These names are used in */
/*  pok_port_sampling_create() and pok_port_ queueing_create*/
uint8_t pok_ports_identifiers[POK_CONFIG_NB_PORTS] = {pr2_pdatain, pr1_pdataout};
uint8_t pok_ports_nb_destinations[POK_CONFIG_NB_PORTS] = {0, 1};
uint8_t* pok_ports_destinations[POK_CONFIG_NB_PORTS] = {NULL, pr1_pdataout_deployment_destinations};
pok_port_kind_t pok_ports_kind[POK_CONFIG_NB_PORTS] = {POK_PORT_KIND_SAMPLING, POK_PORT_KIND_SAMPLING};

/**************************/
/* pok_partition_error   */
/*************************/

void pok_partition_error(uint8_t partition, uint32_t error) {
    switch (partition) {
    case 0: {
        switch (error) {
        case POK_ERROR_KIND_PARTITION_CONFIGURATION: {
            pok_error_partition_callback(0);

            break;
        }
        }

        break;
    }
    case 1: {
        switch (error) {
        case POK_ERROR_KIND_PARTITION_CONFIGURATION: {
            pok_error_partition_callback(1);

            break;
        }
        }

        break;
    }
    }
}

/***********************/
/* pok_kernel_error   */
/**********************/

void pok_kernel_error(uint32_t error) {
    switch (error) {
    case POK_ERROR_KIND_KERNEL_INIT: {
        pok_kernel_restart();

        break;
    }
    }
}
