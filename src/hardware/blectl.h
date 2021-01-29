/****************************************************************************
 *   Aug 11 17:13:51 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef _BLECTL_H
    #define _BLECTL_H
    #include "esp32notifications.h"
    #include "TTGO.h"
    #include "callback.h"
#include "../gui/mainbar/setup_tile/bluetooth_settings/bluetooth_message.h"


    #define BLECTL_CONNECT               _BV(0)         /** @brief event mask for blectl connect to an client */
    #define BLECTL_DISCONNECT            _BV(1)         /** @brief event mask for blectl disconnect */
    #define BLECTL_STANDBY               _BV(2)         /** @brief event mask for blectl standby */
    #define BLECTL_ON                    _BV(3)         /** @brief event mask for blectl on */
    #define BLECTL_OFF                   _BV(4)         /** @brief event mask for blectl off */
    #define BLECTL_ACTIVE                _BV(5)         /** @brief event mask for blectl active */
    #define BLECTL_MSG                   _BV(6)         /** @brief event mask for blectl msg */
    #define BLECTL_PIN_AUTH              _BV(7)         /** @brief event mask for blectl for pin auth, callback arg is (uint32*) */
    #define BLECTL_PAIRING               _BV(8)         /** @brief event mask for blectl pairing requested */
    #define BLECTL_PAIRING_SUCCESS       _BV(9)         /** @brief event mask for blectl pairing success */
    #define BLECTL_PAIRING_ABORT         _BV(10)        /** @brief event mask for blectl pairing abort */
    #define BLECTL_MSG_SEND_SUCCESS      _BV(11)        /** @brief event mask msg send success */
    #define BLECTL_MSG_SEND_ABORT        _BV(12)        /** @brief event mask msg send abort */

    #define BLECTL_JSON_COFIG_FILE         "/blectl.json"   /** @brief defines json config file name */




    /**
     * @brief blectl config structure
     */
    typedef struct {
        bool autoon = true;             /** @brief auto on/off */
        bool advertising = true;        /** @brief advertising on/off */
        bool enable_on_standby = false; /** @brief enable on standby on/off */
        int32_t txpower = 1;            /** @brief tx power, valide values are from 0 to 4 */
    } blectl_config_t;
  /**
     * @brief blectl send msg structure
     */
    typedef struct {
        char *msg;                      /** @brief pointer to an sending msg */
        bool active;                    /** @brief send msg structure active */
        int32_t msglen;                 /** @brief msg lenght */
        int32_t msgpos;                 /** @brief msg postition for next send */
    } blectl_msg_t;


    /**
     * @brief ble setup function
     */
    void blectl_setup( void );
   /**
     * @brief trigger a blectl managemt event
     * 
     * @param   bits    event to trigger
     */
    void blectl_set_event( EventBits_t bits );
    /**
     * @brief clear a blectl managemt event
     * 
     * @param   bits    event to clear
     */
    void blectl_clear_event( EventBits_t bits );
    /**
     * @brief get a blectl managemt event state
     * 
     * @param   bits    event state, example: POWERMGM_STANDBY to evaluate if the system in standby
     */
    bool blectl_get_event( EventBits_t bits );
    /**
     * @brief registers a callback function which is called on a corresponding event
     * 
     * @param   event  possible values:     BLECTL_CONNECT,
     *                                      BLECTL_DISCONNECT,
     *                                      BLECTL_STANDBY,
     *                                      BLECTL_ON,
     *                                      BLECTL_OFF,       
     *                                      BLECTL_ACTIVE,    
     *                                      BLECTL_MSG,
     *                                      BLECTL_PIN_AUTH,
     *                                      BLECTL_PAIRING,
     *                                      BLECTL_PAIRING_SUCCESS,
     *                                      BLECTL_PAIRING_ABORT
     * @param   blectl_event_cb     pointer to the callback function
     * @param   id                  pointer to an string
     */
    bool blectl_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id );
 



    void onBLEStateChanged(BLENotifications::State state);
    void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData);
    void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData);
 
//origosh
 
     /**
     * @brief send an message over bluettoth to gadgetbridge
     * 
     * @param   msg     pointer to a string
     */
    void blectl_send_msg( char *msg );
 
 void blectl_set_txpower( int32_t txpower );
    /**
     * @brief get the current transmission power
     * 
     * @return  power from 0..4, from -12db to 0db in 3db steps
     */
    int32_t blectl_get_txpower( void );
    /**
     * @brief enable the bluettoth stack
     */
    void blectl_on( void );
    /**
     * @brief disable the bluetooth stack
     */
    void blectl_off( void );
    /**
     * @brief get the current enable config
     * 
     * @return true if bl enabled, false if bl disabled
     */
    bool blectl_get_autoon( void );
    /**
     * @brief set the current bl enable config
     * 
     * @param enable    true if enabled, false if disable
     */
    void blectl_set_autoon( bool autoon );

 bool blectl_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id );
    /**
     * @brief enable blueetooth on standby
     * 
     * @param   enable_on_standby   true means enabled, false means disabled 
     */
    void blectl_set_enable_on_standby( bool enable_on_standby );
    /**
     * @brief enable advertising
     * 
     * @param   advertising true means enabled, false means disabled
     */
    void blectl_set_advertising( bool advertising );
    /**
     * @brief get the current enable_on_standby config
     * 
     * @return  true means enabled, false means disabled
     */
    bool blectl_get_enable_on_standby( void );
    /**
     * @brief get the current advertising config
     * 
     * @return  true means enabled, false means disabled
     */
    bool blectl_get_advertising( void );
    /**
     * @brief store the current configuration to SPIFFS
     */
    void blectl_save_config( void );
    /**
     * @brief read the configuration from SPIFFS
     */
    void blectl_read_config( void );


#endif // _BLECTL_H