.. _ug_zigee_adding_clusters:

Adding ZCL clusters to application
##################################

.. contents::
   :local:
   :depth: 2

Once you are familiar with Zigbee in the |NCS| and you have tested some of the available :ref:`zigbee_samples`, you can use the :ref:`Zigbee template <zigbee_template_sample>` sample to create your own application with custom set of ZCL clusters.
For example, you can create a sensor application that uses a temperature sensor with an on/off switch, with the sensor periodically updating its measured value when it is active.

Adding ZCL clusters to application consists of expanding the Zigbee template sample with new application clusters.
By default, the template sample includes only mandatory Zigbee clusters, that is the Basic and Identify clusters, required to identify a device within a Zigbee network.

Cluster is a representation of a single functionality within a Zigbee device.
Each cluster contains attributes that are stored in the device's memory and commands that can be used to modify or read the state of the device, including the cluster attributes.
Clusters appropriate for a single device type such as a sensor or a light bulb are organized into an addressable container that is called an endpoint.

An application can implement appropriate callback functions to be informed about specific cluster state changes.
These functions can be used to alter device's behavior when the state of a cluster is changing as a result of some external event.

Read the following sections for detailed steps about how to expand the Zigbee template sample.

Requirements
************

To take advantage of this guide, you need to be familiar with :ref:`ug_zigbee_architecture` and :ref:`ug_zigbee_configuring`, and have built and tested at least one of the available :ref:`zigbee_samples`.

To verify the working of functionalities implemented in this user guide, following software and hardware are required:

- nRF devkit used to implement functionalities described in this user guide

- nRF devkit used as a ZigBee coordinator for testing the changes

- Sniffer and Wireshark configured for capturing ZigBee packets (see `Configuring Wireshark for Zigbee <https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_sniffer_802154%2FUG%2Fsniffer_802154%2Fconfiguring_sniffer_802154.html>`_).

.. rst-class:: numbered-step

Copy Zigbee template sample
***************************

Use the :ref:`Zigbee Template <zigbee_template_sample>` sample as the base for adding new ZCL clusters:

1. Make sure that you meet the requirements for building the sample.
#. Build and test the sample as described on its documentation page.
#. Copy the contents of the :file:`samples/zigbee/template` directory to a new directory meant for your custom application.
   For example, :file:`samples/zigbee/sensor`.

.. rst-class:: numbered-step

Add On/Off switch and temperature sensor functionalities
*********************************************************

The Zigbee device is defined as a specific set of clusters. These are specified as a unique set of attributes, commands or functions
(`read more <https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_tz_v3.2.0%2Fusing_zigbee__z_c_l.html>`__ and Home Automation Profile specification).
Moreover, clusters can have either server or client role, which defines the way they're used. (more: zigbee cluster library specification: Client/Server Model).

Extending the device's functionalities with an On/Off switch and a temperature sensor requires adding On/Off Switch and Temperature Sensor devices defined in
Home Automation Profile.

Adding On/Off Switch Device
===========================

To create an On/Off Switch, the following clusters needs to be created:

- Identify

- Basic

- On/Off Switch Configuration

- On/Off

- Scenes

- Groups

A cluster list specifying an On/Off Switch device can be created in one step using **ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST** macro,
which declares a static array of clusters. Clusters that have attributes which can be manipulated needs to be created before cluster list
is declared whereas clusters that lacks attributes are declared seamlessly by the macro.

Some of clusters needed to declare On/Off Switch cluster list are already defined in template sample: Identify and Basic. On/Off, Scenes and Groups
are declared by macro. On/Off Switch Configuration cluster needs to be created manually.

1. Start with defining variables for On/Off Switch Configuration cluster's attributes and declare attribute list for them. Embed these
   attributes into **zb_device_ctx** structure which has already been existing in copied template.

.. code-block:: C++

        struct zb_device_ctx {
                zb_zcl_basic_attrs_t     basic_attr;
                zb_zcl_identify_attrs_t  identify_attr;
                zb_uint8_t               on_off_switch_type_attr;
                zb_uint8_t               on_off_switch_actions_attr;
        };

        ZB_ZCL_DECLARE_ON_OFF_SWITCH_CONFIGURATION_ATTRIB_LIST(
                on_off_switch_attr_list,
                &dev_ctx.on_off_switch_type_attr, &dev_ctx.on_off_switch_actions_attr);

2. At this step all clusters required to declare On/Off Switch cluster list exist.
   Create an On/Off Switch cluster list. Use Basic and Identify clusters which exist in template sample and previously created On/Off Switch Configuration
   (`read more <https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_tz_v4.0.0%2Fusing_zigbee__z_c_l.html&anchor=att_declaration>`__).

.. code-block:: C++

        ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST(on_off_switch_clusters,
                on_off_switch_attr_list, basic_attr_list, identify_attr_list);

3. Choose and declare endpoint for an On/Off Switch device. Every cluster in On/Off cluster list which was declared in previous step will use chosen endpoint.

.. code-block:: C++

        // Exemplary 11. endpoint will be used for On/Off Switch cluster
        #define ON_OFF_SWITCH_ENDPOINT          11

        ZB_HA_DECLARE_ON_OFF_SWITCH_EP(on_off_switch_ep, ON_OFF_SWITCH_ENDPOINT, on_off_switch_clusters);


.. note::
   Every endpoint has associated Simple Descriptor which contains informations such as application profile identifier, number of input/output clusters, device version etc. (read more: Zigbee Pro specification: Simple Descriptor).
   Simple descriptors are used to find and identify specific devices in Zigbee network, for example to bind a light switch with light bulb.
   Declaring an endpoint for a device (in this case On/Off Switch) actually defines a Simple Descriptor for the endpoint.

4. Create an application context with all declared endpoints. Template declares device context for single endpoint. Modify this declaration so device can have another endpoint for On/Off device.

.. code-block:: C++

        ZBOSS_DECLARE_DEVICE_CTX_2_EP(app_template_ctx, on_off_switch_ep, app_template_ep);


5. Register device context in ZBOSS. Look at the **main** function and find invocation of **ZB_AF_REGISTER_DEVICE_CTX** macro. This creates a link between the application device context and internal ZBOSS structures.

.. code-block:: C++

        ZB_AF_REGISTER_DEVICE_CTX(&app_template_ctx);


Adding Temperature Sensor Device
=====================================

The process of adding Temperature Sensor is similar to On/Off Switch described above.

The Temperature Sensor device incorporates following clusters:

- Identify - already existing in template

- Basic - already existing in template

- Temperature Measurement - needs to be created

1. Extend **zb_device_ctx** structure with Temperature Measurement attributes and declare the attributes list.
   In case of Temperature Measurement Cluster, variables needed to hold it's attributes are declared in **zb_zcl_temp_measurement_attrs_t** structure,
   which is defined in **<addons/zcl/zb_zcl_temp_measurement_addons.h>**. Some of clusters have its attributes combined into helper structures in
   <addons/zcl/zb_zcl_*_addons.h>.

   Include the header and add a new field in **zb_device_ctx**. Then declare the attribute list for Tempearture Measurement cluster.

.. code-block:: C++

        #include <addons/zcl/zb_zcl_temp_measurement_addons.h>


        struct zb_device_ctx {
                zb_zcl_basic_attrs_t            basic_attr;
                zb_zcl_identify_attrs_t         identify_attr;
                zb_uint8_t                      on_off_switch_type_attr;
                zb_uint8_t                      on_off_switch_actions_attr;
                zb_zcl_temp_measurement_attrs_t temp_measure_attrs;
        };


        ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_measurement_attr_list,
						    &dev_ctx.temp_measure_attrs.measure_value,
						    &dev_ctx.temp_measure_attrs.min_measure_value,
						    &dev_ctx.temp_measure_attrs.max_measure_value,
						    &dev_ctx.temp_measure_attrs.tolerance);

2. Create a Temperature Sensor device by declaring its cluster list. Use Basic, Identify and newly created Temperature Measurement clusters.

.. code-block:: C++

        ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(temperature_sensor_clusters, basic_attr_list, identify_attr_list, temp_measurement_attr_list);

3. Choose and declare endpoint for Temperature Sensor device. Declare device context for created endpoint- modify the device context declaration again, so device can have another endpoint.

.. code-block:: C++

        #define TEMPERATURE_SENSOR_ENDPOINT  12

        ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP(temperature_sensor_ep, TEMPERATURE_SENSOR_ENDPOINT, temperature_sensor_clusters);

        ZBOSS_DECLARE_DEVICE_CTX_3_EP(app_template_ctx, temperature_sensor_ep, on_off_switch_ep, app_template_ep);

.. rst-class:: numbered-step

Testing and verification
************************

To verify the existence of clusters in the device, simple descriptors will be read. This can be done by sending ZDO commands.

Preparing network coordinator for testing
=========================================

1. Enable Zigbee shell in Zigbee network_coordinator sample by adding following line to network_coordinator/prj.conf file.

.. code-block:: none

        CONFIG_ZIGBEE_SHELL=y

2. Build the sample and upload it to the board meant for testing.

3. Open the dk's serial port and issue **help** command. The following output should appear.

.. code-block:: console

        help
        Please press the <Tab> button to see all available commands.
        You can also use the <Tab> button to prompt or auto-complete all commands or its subcommands.
        You can try to call commands with <-h> or <--help> parameter for more information.

        Shell supports following meta-keys:
        Ctrl + (a key from: abcdefklnpuw)
        Alt  + (a key from: bf)
        Please refer to shell documentation for more details.

        Available commands:
        bdb                :Base device behaviour manipulation
        clear              :Clear screen.
        device             :Device commands
        devmem             :Read/write physical memory"devmem address [width [value]]"
        flash              :Flash shell commands
        help               :Prints the help message.
        history            :Command history.
        kernel             :Kernel commands
        nrf_clock_control  :Clock control commmands
        resize             :Console gets terminal screen size or assumes default in
                        case the readout fails. It must be executed after each
                        terminal width change to ensure correct text display.
        sensor             :Sensor commands
        shell              :Useful, not Unix-like shell commands.
        version            :Print firmware version
        zcl                :ZCL subsystem commands.
        zdo                :ZDO manipulation

Running the sample
==================

1. Make sure that network coordinator is running.

#. Build the sample and flash it to the board.

#. Connect to serial port of tested board and observe the output. The device is connected when similar log is displayed:

.. code-block:: console

        I: Joined network successfully (Extended PAN ID: f4ce36e005691785, PAN ID: 0xf7a7)

Verifying clusters operation
============================

The existence of clusters can be checked by reading simple descriptors. This can be done by issuing ZDO commands
to either find the specific cluster in Zigbee network, if device's short address is unknown or to read a list
of clusters contained on a device if its short address is known. Both options are described in following sections.

Looking for a specific cluster in Zigbee network
------------------------------------------------

When one wants to find a specific cluster in a network, match descriptor request can be sent. This is a broadcast command
which expects profile and cluster id. Using network coordinator with Zigbee shell the ZDO match descriptor request can be send in a following way:

1. Make sure the connection with serial port of network_coordinator is still alive. Issue **zdo help** command to know the match descriptor
   command. The fragment is provided here for convinience:

.. code-block:: console

        match_desc - Send match descriptor request.
                Usage: match_desc <h:16-bit destination_address> <h:requested
                address/type> <h:profile ID> <d:number of input clusters> [<h:input
                cluster IDs> ...] <d:number of output clusters> [<h:output cluster
                IDs> ...] [-t | --timeout d:number of seconds to wait for answers]

To find a device with On/Off cluster (0x0006) from Home Automation profile (0x0104), following command will be send:

.. code-block:: console

        zdo match_desc 0xfffd 0xfffd 0x0104 0 1 0x06


resulting with exemplary output:

.. code-block:: console

        Sending broadcast request.

        src_addr=8083 ep=11

.. note::
   Issuing match descriptor request can be used to know short address of device. Alternatively, short address can be checked by
   observing the network_coordinator logs showed during device association or by sniffing the communication and reading packets
   in Wireshark.


Reading a clusters list of a specific Zigbee device
---------------------------------------------------

To know a cluster list existing on a device Simple Descriptor Request can be used. This command requires to know short address of a device. Look at :ref:`Looking for a specific cluster in Zigbee network` to see how
short address can be gathered.

Simple descriptor request requires 2 arguments: short address, endpoint.

Send simple descriptor request to a device:

.. code-block:: console

        zdo simple_desc_req 0x8083 11

The above command will request a simple descriptor from 11. endpoint in device of address 0x8083.
Note that endpoint 11. is used for On/Off switch device which clusters should appear as a result of the command.

It should be expected that device responds and similar output will appear.

.. code-block:: console

        src_addr=0x8083 ep=11 profile_id=0x0104 app_dev_id=0x0 app_dev_ver=0x0
        in_clusters=0x0000,0x0003,0x0007 out_clusters=0x0006,0x0005,0x0004,0x0003

Simple descriptor contains Basic Cluster, Identify Cluster and On/Off Switch Configuration with server roles and
Identify, Groups, Scenes, On/Off configures as clients.

Adding OTA
**********

Normally, extending the sample with OTA would require similar steps as :ref:`Adding On/Off Switch Device` and :ref:`Adding Temperature Sensor Device`. Then zcl device callback would be implemented (:ref:`Passing ZCL events to the application`) to control
the process of collecting chunks of new firmware. Fortunately, zigbee fota library handles the majority of these impediments. The procedure of using the library is described in :ref:`ug_zigbee_configuring_libraries.rst`.


Passing ZCL events to the application
*************************************

Declaring and registering a set of clusters that defines a Zigbee device make them discoverable across the network and ready to communicate with another nodes.
Exemplary communication between a light switch and a light bulb uses ZCL commands to change attributes of the On/Off device embedded in the light bulb.
Altering the attribute does nothing more than changing a specific variable values. The user application is responsible for reacting to these changes and producing appropriate behavior.

To inform the application about attributes changes, ZCL events can be passed to it by a callback which follows generic callback definition and will be called *zcl callback*.

.. code-block:: C++

        typedef void (zb_callback_t)(zb_uint8_t param);

The **param** argument passed to the callback contains informations about changed attributes. It is actually a Zboss buffer containing **zb_zcl_device_callback_param_t** structure whose definition fragment is presented below.

.. code-block:: C++

        /* For the full definition please refer to zboss_api_zcl.h */
        typedef struct zb_zcl_device_callback_param_s
        {
                /** Type of device callback */
                zb_zcl_device_callback_id_t device_cb_id;
                zb_uint8_t endpoint;
                zb_zcl_attr_access_t attr_type;

                /** Return status (see zb_ret_t) */
                zb_ret_t status;

                /** Callback custom data */
                union
                {
                        zb_zcl_set_attr_value_param_t  set_attr_value_param;
                        #if defined (ZB_ZCL_SUPPORT_CLUSTER_ON_OFF)
                        /* Off with effect command, On/Off cluster */
                        zb_zcl_on_off_set_effect_value_param_t  on_off_set_effect_value_param;
                        /* */
                        #endif
                        #if defined(ZB_ZCL_SUPPORT_CLUSTER_IDENTIFY)
                        zb_zcl_identify_effect_value_param_t  identify_effect_value_param;
                        #endif

                        /* .
                           .
                           .
                        */

                        zb_zcl_device_cmd_generic_param_t gnr;
                } cb_param;
        } zb_zcl_device_callback_param_t;

Both Temperature Sensor and On/Off Switch devices have client roles thus their attributes are not supposed to be changed.
To show how the zcl event can be used, zcl callback implemented in zigbee fota library will be discussed.

Reading device callback parameters from a Zboss buffer
======================================================

As mentioned before, zcl callback parameters are contained in zb_zcl_device_callback_param_t structure. Parameters are read in a following way:

.. code-block:: C++

        zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);


Once the parameters are obtained, application can use them to perform some action based on a new attribute values. Firstly, it must check the callback id.
Refer to :ref:`zb_zcl_device_callback_id_t` to check available callback ids. Exemplary zigbee fota's zcl callback uses **ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID**.

.. code-block:: C++

        if (device_cb_param->device_cb_id != ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID) {
		return;
	}

Depending on a device callback id, different data are passed to the callback and held by **cb_param** field of zb_zcl_device_callback_param_t. In general the data associated with a callback id
are contained in **set_attr_value_param** field of **cb_param**, but some clusters have their data structure already defined (for example OTA uses **ota_value_param** fields). To see usage of fields associated with other clusters, please
refer to :ref:`light_bulb` sample.

.. code-block:: C++

        zb_zcl_ota_upgrade_value_param_t *ota_upgrade_value = &(device_cb_param->cb_param.ota_value_param);

A zcl callback needs to pass the status of its execution to the caller. It is done by setting appropriate return status to **status** field of zb_zcl_device_callback_param_t structure passed to the callback.

.. code-block:: C++

        device_cb_param->status = RET_OK;
