//TRANSMITTER
#include <VirtualWire.h>
void setup()
{
    vw_set_ptt_inverted(true);  // Required by the RF module
    vw_setup(2000);            // bps connection speed
    vw_set_tx_pin(7);         // Arduino pin to connect the receiver data pin
}
 
void loop()
{
   //Message to send:
   const char *msg = "12";
   // send data only when you receive data:
        //if (Serial.available() > 0) {
                // read the incoming byte:
                //int msg = Serial.read();
                vw_send((uint8_t *)"2", 1);
                vw_wait_tx();        // We wait to finish sending the message
                 delay(300);         // We wait to send the message again    
        //}            
}
