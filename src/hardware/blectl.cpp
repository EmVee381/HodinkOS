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

/*
 *  inspire by https://github.com/bburky/t-watch-2020-project
 *
 */
#include "config.h"
#include "Arduino.h"
#include "../gui/mainbar/setup_tile/bluetooth_settings/bluetooth_message.h"

#include "BLEDevice.h"
#include "blectl.h"
#include "esp32notifications.h"


#include "json_psram_allocator.h"
#include "callback.h"
#include "msg_chain.h"
#include "powermgm.h"
#include "pmu.h"

bool blectl_powermgm_event_cb( EventBits_t event, void *arg );
bool blectl_powermgm_loop_cb( EventBits_t event, void *arg );
bool blectl_pmu_event_cb( EventBits_t event, void *arg );



portMUX_TYPE DRAM_ATTR blectlMux = portMUX_INITIALIZER_UNLOCKED;
EventGroupHandle_t blectl_status = NULL;

// Create an interface to the BLE notification library
BLENotifications notifications;
blectl_msg_t blectl_msg;
msg_chain_t *blectl_msg_chain = NULL;

// Holds the incoming call's ID number, or zero if no notification
uint32_t incomingCallNotificationUUID;

callback_t *blectl_callback = NULL;



blectl_config_t blectl_config;
BLEServer *pServer = NULL;
bool blectl_send_event_cb( EventBits_t event, void *arg ) {
    return( callback_send( blectl_callback, event, arg ) );
}
void blectl_set_enable_on_standby( bool enable_on_standby ) {   
         
     
    blectl_config.enable_on_standby = enable_on_standby;
    blectl_save_config();
}

void blectl_setup( void ) {
    blectl_status = xEventGroupCreate();

    esp_bt_controller_enable( ESP_BT_MODE_BLE );
    esp_bt_controller_mem_release( ESP_BT_MODE_CLASSIC_BT );
    esp_bt_mem_release( ESP_BT_MODE_CLASSIC_BT );
    esp_bt_controller_mem_release( ESP_BT_MODE_IDLE );
    esp_bt_mem_release( ESP_BT_MODE_IDLE );
    pServer=notifications.begin("BLEConnection device name");
    notifications.setConnectionStateChangedCallback(onBLEStateChanged);
    notifications.setNotificationCallback(onNotificationArrived);
    notifications.setRemovedCallback(onNotificationRemoved);
    switch( blectl_config.txpower ) {
        case 0:             BLEDevice::setPower( ESP_PWR_LVL_N12 );
                            break;
        case 1:             BLEDevice::setPower( ESP_PWR_LVL_N9 );
                            break;
        case 2:             BLEDevice::setPower( ESP_PWR_LVL_N6 );
                            break;
        case 3:             BLEDevice::setPower( ESP_PWR_LVL_N3 );
                            break;
        case 4:             BLEDevice::setPower( ESP_PWR_LVL_N0 );
                            break;
        default:            BLEDevice::setPower( ESP_PWR_LVL_N9 );
                            break;
    }
     if ( blectl_get_autoon() ) {
        blectl_on();

    }
    powermgm_register_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, blectl_powermgm_event_cb, "blectl" );
 //   powermgm_register_loop_cb( POWERMGM_SILENCE_WAKEUP | POWERMGM_STANDBY | POWERMGM_WAKEUP, blectl_powermgm_loop_cb, "blectl loop" );
    pmu_register_cb( PMUCTL_BATTERY_PERCENT | PMUCTL_CHARGING | PMUCTL_VBUS_PLUG, blectl_pmu_event_cb, "bluetooth battery");

}
void onBLEStateChanged(BLENotifications::State state) {
  switch(state) {
      case BLENotifications::StateConnected:
        blectl_set_event( BLECTL_CONNECT );
        blectl_clear_event( BLECTL_DISCONNECT );
        blectl_send_event_cb( BLECTL_CONNECT, (void *)"connected" );
       // blectl_msg_chain = msg_chain_delete( blectl_msg_chain );
        log_i("BLE connected");
          break;

      case BLENotifications::StateDisconnected:
         blectl_set_event( BLECTL_DISCONNECT );
        blectl_clear_event( BLECTL_CONNECT );
        blectl_send_event_cb( BLECTL_DISCONNECT, (void *)"disconnected" );
       // blectl_msg_chain = msg_chain_delete( blectl_msg_chain );
        blectl_msg.active = false;
        log_i("BLE disconnected");
        if ( blectl_get_advertising() ) {
        notifications.startAdvertising(); 
        }
          break; 
  }
}


// A notification arrived from the mobile device, ie a social media notification or incoming call.
// parameters:
//  - notification: an Arduino-friendly structure containing notification information. Do not keep a
//                  pointer to this data - it will be destroyed after this function.
//  - rawNotificationData: a pointer to the underlying data. It contains the same information, but is
//                         not beginner-friendly. For advanced use-cases.
void onNotificationArrived(const ArduinoNotification * notification, const Notification * rawNotificationData) {
    Serial.println("Got notification: ");
    Serial.println(ctime(&notification->time));  // 
    
    Serial.println(notification->uuid); 
    Serial.println(notification->title); // The title, ie name of who sent the message
    Serial.println(notification->message); // The detail, ie "be home for dinner at 7".
    Serial.println(notification->type);  // Which app sent it
    Serial.println(notifications.getNotificationCategoryDescription(notification->category));  // ie "social media"
    Serial.println(notification->categoryCount); // How may other notifications are there from this app (ie badge number)

    DynamicJsonDocument doc(1024);
    doc["t"]="notify";
    doc["id"]=notification->uuid;
    doc["src"]=notification->type;
    doc["title"]=notification->title;
    doc["body"]=notification->message;
    doc["timestamp"]=notification->time;

char json_string[256];
    serializeJson(doc,json_string);
 blectl_send_event_cb( BLECTL_MSG, json_string );
    /*
    if (notification->category == CategoryIDIncomingCall) {
		// If this is an incoming call, store it so that we can later send a user action.
        incomingCallNotificationUUID = notification->uuid;
        Serial.println("--- INCOMING CALL: PRESS A TO ACCEPT, C TO REJECT ---"); 
    }
    else {
        incomingCallNotificationUUID = 0; // Make invalid - no incoming call
    }
    */
}


// A notification was cleared
void onNotificationRemoved(const ArduinoNotification * notification, const Notification * rawNotificationData) {
     Serial.print("Removed notification: ");   
     Serial.println(notification->title);
     Serial.println(notification->message);
     Serial.println(notification->type);  
     bluetooth_delete_msg_from_chain(notification->uuid);
}




// original

void blectl_set_autoon( bool autoon ) {
    blectl_config.autoon = autoon;

    if( autoon ) {
        blectl_on();
    }
    else {
        blectl_off();
    }
    blectl_save_config();
}

int32_t blectl_get_txpower( void ) {
    return( blectl_config.txpower );
}

bool blectl_get_enable_on_standby( void ) {
    return( blectl_config.enable_on_standby );
}

bool blectl_get_autoon( void ) {
    return( blectl_config.autoon );
}

bool blectl_get_advertising( void ) {
    return( blectl_config.advertising );
}

void blectl_save_config( void ) {
    fs::File file = SPIFFS.open( BLECTL_JSON_COFIG_FILE, FILE_WRITE );

    if (!file) {
        log_e("Can't open file: %s!", BLECTL_JSON_COFIG_FILE );
    }
    else {
        SpiRamJsonDocument doc( 1000 );

        doc["autoon"] = blectl_config.autoon;
        doc["advertising"] = blectl_config.advertising;
        doc["enable_on_standby"] = blectl_config.enable_on_standby;
        doc["tx_power"] = blectl_config.txpower;
        log_e("Save to file: %s!", BLECTL_JSON_COFIG_FILE );

        if ( serializeJsonPretty( doc, file ) == 0) {
            log_e("Failed to write config file");
        }
        doc.clear();
    }
    file.close();
}

void blectl_read_config( void ) {
    fs::File file = SPIFFS.open( BLECTL_JSON_COFIG_FILE, FILE_READ );

    if (!file) {
        log_e("Can't open file: %s!", BLECTL_JSON_COFIG_FILE );
    }
    else {
        int filesize = file.size();
        SpiRamJsonDocument doc( filesize * 2 );
        log_i("Opened file: %s!", BLECTL_JSON_COFIG_FILE );

        DeserializationError error = deserializeJson( doc, file );
        if ( error ) {
            log_e("blectl deserializeJson() failed: %s", error.c_str() );
        }
        else {     
            log_i("standby: %d!", doc["enable_on_standby"]?1:0 );
         
            blectl_config.autoon = doc["autoon"] | true;
            blectl_config.advertising = doc["advertising"] | true;
            blectl_config.enable_on_standby = doc["enable_on_standby"] | true;
            blectl_config.txpower = doc["tx_power"] | 1;
        }        
        doc.clear();
    }
    file.close();
}

void blectl_on( void ) {
    blectl_config.autoon = true;
    if ( blectl_config.advertising ) {
        pServer->getAdvertising()->start();
    }
    else {
        pServer->getAdvertising()->stop();
    }
    blectl_set_event( BLECTL_ON );
    blectl_clear_event( BLECTL_OFF );
    blectl_send_event_cb( BLECTL_ON, (void *)NULL );
}

void blectl_off( void ) {
    blectl_config.autoon = false;
    pServer->getAdvertising()->stop();
    blectl_set_event( BLECTL_OFF );
    blectl_clear_event( BLECTL_ON );
    blectl_send_event_cb( BLECTL_OFF, (void *)NULL );
}
void blectl_set_event( EventBits_t bits ) {
    portENTER_CRITICAL(&blectlMux);
    xEventGroupSetBits( blectl_status, bits );
    portEXIT_CRITICAL(&blectlMux);
}

void blectl_clear_event( EventBits_t bits ) {
    portENTER_CRITICAL(&blectlMux);
    xEventGroupClearBits( blectl_status, bits );
    portEXIT_CRITICAL(&blectlMux);
}

bool blectl_get_event( EventBits_t bits ) {
    portENTER_CRITICAL(&blectlMux);
    EventBits_t temp = xEventGroupGetBits( blectl_status ) & bits;
    portEXIT_CRITICAL(&blectlMux);
    if ( temp )
        return( true );

    return( false );
}
bool blectl_register_cb( EventBits_t event, CALLBACK_FUNC callback_func, const char *id ) {
    if ( blectl_callback == NULL ) {
        blectl_callback = callback_init( "blectl" );
        if ( blectl_callback == NULL ) {
            log_e("blectl callback alloc failed");
            while(true);
        }
    }    
    return( callback_register( blectl_callback, event, callback_func, id ) );
}
void blectl_set_txpower( int32_t txpower ) {
    if ( txpower >= 0 && txpower <= 4 ) {
        blectl_config.txpower = txpower;
    }
    switch( blectl_config.txpower ) {
        case 0:             BLEDevice::setPower( ESP_PWR_LVL_N12 );
                            break;
        case 1:             BLEDevice::setPower( ESP_PWR_LVL_N9 );
                            break;
        case 2:             BLEDevice::setPower( ESP_PWR_LVL_N6 );
                            break;
        case 3:             BLEDevice::setPower( ESP_PWR_LVL_N3 );
                            break;
        case 4:             BLEDevice::setPower( ESP_PWR_LVL_N0 );
                            break;
        default:            BLEDevice::setPower( ESP_PWR_LVL_N9 );
                            break;
    }
    blectl_save_config();
}
void blectl_set_advertising( bool advertising ) {  
    blectl_config.advertising = advertising;
    blectl_save_config();
    if ( blectl_get_event( BLECTL_CONNECT ) )
        return;

    if ( advertising ) {
        pServer->getAdvertising()->start();
    }
    else {
        pServer->getAdvertising()->stop();
    }
}
void blectl_send_msg( char *msg ) {
    if ( blectl_get_event( BLECTL_CONNECT ) ) {
        blectl_msg_chain = msg_chain_add_msg( blectl_msg_chain, msg );
    }
    else {
        log_e("msg can't send while bluetooth is not connected");
    }
}

bool blectl_pmu_event_cb( EventBits_t event, void *arg ) {
    static int32_t percent = 0;
    static bool charging = false;
    static bool plug = false;

    switch( event ) {
        case PMUCTL_BATTERY_PERCENT:
            percent = *(int32_t*)arg;
            if ( blectl_get_event( BLECTL_CONNECT ) ) {
          //      blectl_update_battery( percent, charging, plug );
            }
            break;
        case PMUCTL_CHARGING:
            charging = *(bool*)arg;
            break;
        case PMUCTL_VBUS_PLUG:
            plug = *(bool*)arg;
            break;
    }
    return( true );
}
bool blectl_powermgm_event_cb( EventBits_t event, void *arg ) {
    bool retval = true;

    switch( event ) {
        case POWERMGM_STANDBY:          
            if ( blectl_get_enable_on_standby() && blectl_get_event( BLECTL_ON ) ) {
                retval = false;
                log_w("standby blocked by \"enable_on_standby\" option");
            }
            else {
                log_i("go standby");
            }
            break;
        case POWERMGM_WAKEUP: 
                        pServer->getAdvertising()->start();
          
            log_i("go wakeup");
            break;
        case POWERMGM_SILENCE_WAKEUP:   
            log_i("go silence wakeup");
            break;
    }
    return( retval );
}

bool blectl_powermgm_loop_cb( EventBits_t event, void *arg ) {
 //   blectl_loop();
    return( true );
}

