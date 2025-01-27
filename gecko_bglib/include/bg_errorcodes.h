
/*****************************************************************************
 *
 *  Error definitions
 *
 *  Autogenerated file, do not edit
 *
 ****************************************************************************/

/* Build version:  */

#ifndef BG_ERRORCODES
#define BG_ERRORCODES
enum bg_error_spaces
{
	bg_errspc_hardware=1280,
	bg_errspc_bg=256,
	bg_errspc_sdp=1536,
	bg_errspc_smp=768,
	bg_errspc_bt=512,
	bg_errspc_application=2560,
	bg_errspc_att=1024,
	bg_errspc_filesystem=2304,
	bg_errspc_security=2816,
};
typedef enum bg_error
{
	bg_err_hardware_ps_store_full                                                         =bg_errspc_hardware+1,    //Flash reserved for PS store is full
	bg_err_hardware_ps_key_not_found                                                      =bg_errspc_hardware+2,    //PS key not found
	bg_err_hardware_i2c_ack_missing                                                       =bg_errspc_hardware+3,    //Acknowledge for i2c was not received.
	bg_err_hardware_i2c_timeout                                                           =bg_errspc_hardware+4,    //I2C read or write timed out.
	bg_err_hardware_not_configured                                                        =bg_errspc_hardware+5,    //Hardware is not configured for that function.
	bg_err_hardware_ble_not_supported                                                     =bg_errspc_hardware+6,    //Hardware does not support Blootooth Smart.
	bg_err_success                                                                        =0,                       //No error
	bg_err_invalid_conn_handle                                                            =bg_errspc_bg+1,          //Invalid GATT connection handle.
	bg_err_waiting_response                                                               =bg_errspc_bg+2,          //Waiting response from GATT server to previous procedure.
	bg_err_gatt_connection_timeout                                                        =bg_errspc_bg+3,          //GATT connection is closed due procedure timeout.
	bg_err_invalid_param                                                                  =bg_errspc_bg+128,        //Command contained invalid parameter
	bg_err_wrong_state                                                                    =bg_errspc_bg+129,        //Device is in wrong state to receive command
	bg_err_out_of_memory                                                                  =bg_errspc_bg+130,        //Device has run out of memory
	bg_err_not_implemented                                                                =bg_errspc_bg+131,        //Feature is not implemented
	bg_err_invalid_command                                                                =bg_errspc_bg+132,        //Command was not recognized
	bg_err_timeout                                                                        =bg_errspc_bg+133,        //Command or Procedure failed due to timeout
	bg_err_not_connected                                                                  =bg_errspc_bg+134,        //Connection handle passed is to command is not a valid handle
	bg_err_flow                                                                           =bg_errspc_bg+135,        //Command would cause either underflow or overflow error
	bg_err_user_attribute                                                                 =bg_errspc_bg+136,        //User attribute was accessed through API which is not supported
	bg_err_invalid_license_key                                                            =bg_errspc_bg+137,        //No valid license key found
	bg_err_command_too_long                                                               =bg_errspc_bg+138,        //Command maximum length exceeded
	bg_err_out_of_bonds                                                                   =bg_errspc_bg+139,        //Bonding procedure can't be started because device has no space left for bond.
	bg_err_unspecified                                                                    =bg_errspc_bg+140,        //Unspecified error
	bg_err_hardware                                                                       =bg_errspc_bg+141,        //Hardware failure
	bg_err_buffers_full                                                                   =bg_errspc_bg+142,        //Command not accepted, because internal buffers are full
	bg_err_disconnected                                                                   =bg_errspc_bg+143,        //Command or Procedure failed due to disconnection
	bg_err_too_many_requests                                                              =bg_errspc_bg+144,        //Too many Simultaneous Requests
	bg_err_not_supported                                                                  =bg_errspc_bg+145,        //Feature is not supported in this firmware build
	bg_err_no_bonding                                                                     =bg_errspc_bg+146,        //The bonding does not exist.
	bg_err_crypto                                                                         =bg_errspc_bg+147,        //Error using crypto functions
	bg_err_sdp_record_not_found                                                           =bg_errspc_sdp+1,         //Service Record not found
	bg_err_sdp_record_already_exist                                                       =bg_errspc_sdp+2,         //Service Record with this handle already exist.
	bg_err_smp_passkey_entry_failed                                                       =bg_errspc_smp+1,         //The user input of passkey failed, for example, the user cancelled the operation
	bg_err_smp_oob_not_available                                                          =bg_errspc_smp+2,         //Out of Band data is not available for authentication
	bg_err_smp_authentication_requirements                                                =bg_errspc_smp+3,         //The pairing procedure cannot be performed as authentication requirements cannot be met due to IO capabilities of one or both devices
	bg_err_smp_confirm_value_failed                                                       =bg_errspc_smp+4,         //The confirm value does not match the calculated compare value
	bg_err_smp_pairing_not_supported                                                      =bg_errspc_smp+5,         //Pairing is not supported by the device
	bg_err_smp_encryption_key_size                                                        =bg_errspc_smp+6,         //The resultant encryption key size is insufficient for the security requirements of this device
	bg_err_smp_command_not_supported                                                      =bg_errspc_smp+7,         //The SMP command received is not supported on this device
	bg_err_smp_unspecified_reason                                                         =bg_errspc_smp+8,         //Pairing failed due to an unspecified reason
	bg_err_smp_repeated_attempts                                                          =bg_errspc_smp+9,         //Pairing or authentication procedure is disallowed because too little time has elapsed since last pairing request or security request
	bg_err_smp_invalid_parameters                                                         =bg_errspc_smp+10,        //The Invalid Parameters error code indicates: the command length is invalid or a parameter is outside of the specified range.
	bg_err_smp_dhkey_check_failed                                                         =bg_errspc_smp+11,        //Indicates to the remote device that the DHKey Check value received doesn't match the one calculated by the local device.
	bg_err_smp_numeric_comparison_failed                                                  =bg_errspc_smp+12,        //Indicates that the confirm values in the numeric comparison protocol do not match.
	bg_err_smp_bredr_pairing_in_progress                                                  =bg_errspc_smp+13,        //Indicates that the pairing over the LE transport failed due to a Pairing Request sent over the BR/EDR transport in process.
	bg_err_smp_cross_transport_key_derivation_generation_not_allowed                      =bg_errspc_smp+14,        //Indicates that the BR/EDR Link Key generated on the BR/EDR transport cannot be used to derive and distribute keys for the LE transport.
	bg_err_bt_error_success                                                               =0,                       //Command completed succesfully
	bg_err_bt_unknown_connection_identifier                                               =bg_errspc_bt+2,          //A command was sent from the Host that should identify a connection, but that connection does not exist.
	bg_err_bt_page_timeout                                                                =bg_errspc_bt+4,          //The Page Timeout error code indicates that a page timed out because of the Page Timeout configuration parameter.
	bg_err_bt_authentication_failure                                                      =bg_errspc_bt+5,          //Pairing or authentication failed due to incorrect results in the pairing or authentication procedure. This could be due to an incorrect PIN or Link Key
	bg_err_bt_pin_or_key_missing                                                          =bg_errspc_bt+6,          //Pairing failed because of missing PIN, or authentication failed because of missing Key
	bg_err_bt_memory_capacity_exceeded                                                    =bg_errspc_bt+7,          //Controller is out of memory.
	bg_err_bt_connection_timeout                                                          =bg_errspc_bt+8,          //Link supervision timeout has expired.
	bg_err_bt_connection_limit_exceeded                                                   =bg_errspc_bt+9,          //Controller is at limit of connections it can support.
	bg_err_bt_synchronous_connectiontion_limit_exceeded                                   =bg_errspc_bt+10,         //The Synchronous Connection Limit to a Device Exceeded error code indicates that the Controller has reached the limit to the number of synchronous connections that can be achieved to a device.
	bg_err_bt_acl_connection_already_exists                                               =bg_errspc_bt+11,         //The ACL Connection Already Exists error code indicates that an attempt to create a new ACL Connection to a device when there is already a connection to this device.
	bg_err_bt_command_disallowed                                                          =bg_errspc_bt+12,         //Command requested cannot be executed because the Controller is in a state where it cannot process this command at this time.
	bg_err_bt_connection_rejected_due_to_limited_resources                                =bg_errspc_bt+13,         //The Connection Rejected Due To Limited Resources error code indicates that an incoming connection was rejected due to limited resources.
	bg_err_bt_connection_rejected_due_to_security_reasons                                 =bg_errspc_bt+14,         //The Connection Rejected Due To Security Reasons error code indicates that a connection was rejected due to security requirements not being fulfilled, like authentication or pairing.
	bg_err_bt_connection_rejected_due_to_unacceptable_bd_addr                             =bg_errspc_bt+15,         //The Connection was rejected because this device does not accept the BD_ADDR. This may be because the device will only accept connections from specific BD_ADDRs.
	bg_err_bt_connection_accept_timeout_exceeded                                          =bg_errspc_bt+16,         //The Connection Accept Timeout has been exceeded for this connection attempt.
	bg_err_bt_unsupported_feature_or_parameter_value                                      =bg_errspc_bt+17,         //A feature or parameter value in the HCI command is not supported.
	bg_err_bt_invalid_command_parameters                                                  =bg_errspc_bt+18,         //Command contained invalid parameters.
	bg_err_bt_remote_user_terminated                                                      =bg_errspc_bt+19,         //User on the remote device terminated the connection.
	bg_err_bt_remote_device_terminated_connection_due_to_low_resources                    =bg_errspc_bt+20,         //The remote device terminated the connection because of low resources
	bg_err_bt_remote_powering_off                                                         =bg_errspc_bt+21,         //Remote Device Terminated Connection due to Power Off
	bg_err_bt_connection_terminated_by_local_host                                         =bg_errspc_bt+22,         //Local device terminated the connection.
	bg_err_bt_repeated_attempts                                                           =bg_errspc_bt+23,         //The Controller is disallowing an authentication or pairing procedure because too little time has elapsed since the last authentication or pairing attempt failed.
	bg_err_bt_pairing_not_allowed                                                         =bg_errspc_bt+24,         //The device does not allow pairing. This can be for example, when a device only allows pairing during a certain time window after some user input allows pairing
	bg_err_bt_unknown_lmp_pdu                                                             =bg_errspc_bt+25,         //The Controller has received an unknown LMP OpCode.
	bg_err_bt_unsupported_remote_feature                                                  =bg_errspc_bt+26,         //The remote device does not support the feature associated with the issued command or LMP PDU.
	bg_err_bt_sco_offset_rejected                                                         =bg_errspc_bt+27,         //The offset requested in the LMP_SCO_link_req PDU has been rejected.
	bg_err_bt_sco_interval_rejected                                                       =bg_errspc_bt+28,         //The interval requested in the LMP_SCO_link_req PDU has been rejected.
	bg_err_bt_sco_air_mode_rejected                                                       =bg_errspc_bt+29,         //The air mode requested in the LMP_SCO_link_req PDU has been rejected.
	bg_err_bt_invalid_lmp_parameters                                                      =bg_errspc_bt+30,         //Some LMP PDU / LL Control PDU parameters were invalid.
	bg_err_bt_unspecified_error                                                           =bg_errspc_bt+31,         //No other error code specified is appropriate to use.
	bg_err_bt_unsupported_lmp_parameter_value                                             =bg_errspc_bt+32,         //An LMP PDU or an LL Control PDU contains at least one parameter value that is not supported by the Controller at this time.
	bg_err_bt_role_change_not_allowed                                                     =bg_errspc_bt+33,         //Controller will not allow a role change at this time.
	bg_err_bt_ll_response_timeout                                                         =bg_errspc_bt+34,         //Connection terminated due to link-layer procedure timeout.
	bg_err_bt_lmp_error_transaction_collision                                             =bg_errspc_bt+35,         //LMP transaction has collided with the same transaction that is already in progress.
	bg_err_bt_lmp_pdu_not_allowed                                                         =bg_errspc_bt+36,         //Controller sent an LMP PDU with an OpCode that was not allowed.
	bg_err_bt_encryption_mode_not_acceptable                                              =bg_errspc_bt+37,         //The requested encryption mode is not acceptable at this time.
	bg_err_bt_link_key_cannot_be_changed                                                  =bg_errspc_bt+38,         //Link key cannot be changed because a fixed unit key is being used.
	bg_err_bt_requested_qos_not_supported                                                 =bg_errspc_bt+39,         //The requested Quality of Service is not supported.
	bg_err_bt_instant_passed                                                              =bg_errspc_bt+40,         //LMP PDU or LL PDU that includes an instant cannot be performed because the instant when this would have occurred has passed.
	bg_err_bt_pairing_with_unit_key_not_supported                                         =bg_errspc_bt+41,         //It was not possible to pair as a unit key was requested and it is not supported.
	bg_err_bt_different_transaction_collision                                             =bg_errspc_bt+42,         //LMP transaction was started that collides with an ongoing transaction.
	bg_err_bt_qos_unacceptable_parameter                                                  =bg_errspc_bt+44,         //The specified quality of service parameters could not be accepted at this time, but other parameters may be acceptable.
	bg_err_bt_qos_rejected                                                                =bg_errspc_bt+45,         //The specified quality of service parameters cannot be accepted and QoS negotiation should be terminated.
	bg_err_bt_channel_assesment_not_supported                                             =bg_errspc_bt+46,         //The Controller cannot perform channel assessment because it is not supported.
	bg_err_bt_insufficient_security                                                       =bg_errspc_bt+47,         //The HCI command or LMP PDU sent is only possible on an encrypted link.
	bg_err_bt_parameter_out_of_mandatory_range                                            =bg_errspc_bt+48,         //A parameter value requested is outside the mandatory range of parameters for the given HCI command or LMP PDU.
	bg_err_bt_role_switch_pending                                                         =bg_errspc_bt+50,         //Role Switch is pending. This can be used when an HCI command or LMP PDU cannot be accepted because of a pending role switch. This can also be used to notify a peer device about a pending role switch.
	bg_err_bt_reserved_slot_violation                                                     =bg_errspc_bt+52,         //The current Synchronous negotiation was terminated with the negotiation state set to Reserved Slot Violation.
	bg_err_bt_role_switch_failed                                                          =bg_errspc_bt+53,         //role switch was attempted but it failed and the original piconet structure is restored. The switch may have failed because the TDD switch or piconet switch failed.
	bg_err_bt_extended_inquiry_response_too_large                                         =bg_errspc_bt+54,         //The extended inquiry response, with the requested requirements for FEC, is too large to fit in any of the packet types supported by the Controller.
	bg_err_bt_simple_pairing_not_supported_by_host                                        =bg_errspc_bt+55,         //The IO capabilities request or response was rejected because the sending Host does not support Secure Simple Pairing even though the receiving Link Manager does.
	bg_err_bt_host_busy_pairing                                                           =bg_errspc_bt+56,         //The Host is busy with another pairing operation and unable to support the requested pairing. The receiving device should retry pairing again later.
	bg_err_bt_connection_rejected_due_to_no_suitable_channel_found                        =bg_errspc_bt+57,         //The Controller could not calculate an appropriate value for the Channel selection operation.
	bg_err_bt_controller_busy                                                             =bg_errspc_bt+58,         //Operation was rejected because the controller is busy and unable to process the request.
	bg_err_bt_unacceptable_connection_interval                                            =bg_errspc_bt+59,         //Remote evice terminated the connection because of an unacceptable connection interval.
	bg_err_bt_directed_advertising_timeout                                                =bg_errspc_bt+60,         //Directed advertising completed without a connection being created.
	bg_err_bt_connection_terminated_due_to_mic_failure                                    =bg_errspc_bt+61,         //Connection was terminated because the Message Integrity Check (MIC) failed on a received packet.
	bg_err_bt_connection_failed_to_be_established                                         =bg_errspc_bt+62,         //LL initiated a connection but the connection has failed to be established. Controller did not receive any packets from remote end.
	bg_err_bt_mac_connection_failed                                                       =bg_errspc_bt+63,         //The MAC of the 802.11 AMP was requested to connect to a peer, but the connection failed.
	bg_err_bt_coarse_clock_adjustment_rejected_but_will_try_to_adjust_using_clock_dragging=bg_errspc_bt+64,         //The master, at this time, is unable to make a coarse adjustment to the piconet clock, using the supplied parameters. Instead the master will attempt to move the clock using clock dragging.
	bg_err_application_file_open_failed                                                   =bg_errspc_application+1, //File open failed.
	bg_err_application_xml_parse_failed                                                   =bg_errspc_application+2, //XML parsing failed.
	bg_err_application_device_connection_failed                                           =bg_errspc_application+3, //Device connection failed.
	bg_err_application_device_comunication_failed                                         =bg_errspc_application+4, //Device communication failed.
	bg_err_application_authentication_failed                                              =bg_errspc_application+5, //Device authentication failed.
	bg_err_application_incorrect_gatt_database                                            =bg_errspc_application+6, //Device has incorrect GATT database.
	bg_err_application_disconnected_due_to_procedure_collision                            =bg_errspc_application+7, //Device disconnected due to procedure collision.
	bg_err_application_disconnected_due_to_secure_session_failed                          =bg_errspc_application+8, //Device disconnected due to failure to establish or reestablish a secure session.
	bg_err_application_encryption_decryption_error                                        =bg_errspc_application+9, //Encrypion/decryption operation failed.
	bg_err_application_maximum_retries                                                    =bg_errspc_application+10,//Maximum allowed retries exceeded.
	bg_err_application_data_parse_failed                                                  =bg_errspc_application+11,//Data parsing failed.
	bg_err_att_invalid_handle                                                             =bg_errspc_att+1,         //The attribute handle given was not valid on this server
	bg_err_att_read_not_permitted                                                         =bg_errspc_att+2,         //The attribute cannot be read
	bg_err_att_write_not_permitted                                                        =bg_errspc_att+3,         //The attribute cannot be written
	bg_err_att_invalid_pdu                                                                =bg_errspc_att+4,         //The attribute PDU was invalid
	bg_err_att_insufficient_authentication                                                =bg_errspc_att+5,         //The attribute requires authentication before it can be read or written.
	bg_err_att_request_not_supported                                                      =bg_errspc_att+6,         //Attribute Server does not support the request received from the client.
	bg_err_att_invalid_offset                                                             =bg_errspc_att+7,         //Offset specified was past the end of the attribute
	bg_err_att_insufficient_authorization                                                 =bg_errspc_att+8,         //The attribute requires authorization before it can be read or written.
	bg_err_att_prepare_queue_full                                                         =bg_errspc_att+9,         //Too many prepare writes have been queueud
	bg_err_att_att_not_found                                                              =bg_errspc_att+10,        //No attribute found within the given attribute handle range.
	bg_err_att_att_not_long                                                               =bg_errspc_att+11,        //The attribute cannot be read or written using the Read Blob Request
	bg_err_att_insufficient_enc_key_size                                                  =bg_errspc_att+12,        //The Encryption Key Size used for encrypting this link is insufficient.
	bg_err_att_invalid_att_length                                                         =bg_errspc_att+13,        //The attribute value length is invalid for the operation
	bg_err_att_unlikely_error                                                             =bg_errspc_att+14,        //The attribute request that was requested has encountered an error that was unlikely, and therefore could not be completed as requested.
	bg_err_att_insufficient_encryption                                                    =bg_errspc_att+15,        //The attribute requires encryption before it can be read or written.
	bg_err_att_unsupported_group_type                                                     =bg_errspc_att+16,        //The attribute type is not a supported grouping attribute as defined by a higher layer specification.
	bg_err_att_insufficient_resources                                                     =bg_errspc_att+17,        //Insufficient Resources to complete the request
	bg_err_att_application                                                                =bg_errspc_att+128,       //Application error code defined by a higher layer specification.
	bg_err_filesystem_file_not_found                                                      =bg_errspc_filesystem+1,  //File not found
	bg_err_security_image_signature_verification_failed                                   =bg_errspc_security+1,    //Device firmware signature verification failed.
	bg_err_security_file_signature_verification_failed                                    =bg_errspc_security+2,    //File signature verification failed.
	bg_err_security_image_checksum_error                                                  =bg_errspc_security+3,    //Device firmware checksum is not valid.
	bg_err_last
}errorcode_t;


#endif
