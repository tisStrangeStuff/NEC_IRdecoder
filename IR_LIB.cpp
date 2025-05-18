/*Note: Dont press the button to fast after after decoding, the IRRest loop is reather
and tends not to rest the loop after the first go around for some reason.
*/
#include "IR_LIB.h"

volatile unsigned long bitDecoding = 0; //Bit Timersss
volatile unsigned long previousISRTime = 0; // Bit previous timmmer

bool StartBit = 0;
bool EndBit = 1;

//Storage arrays
volatile unsigned int bitLength[70] = {0};
unsigned int IRBits[32] = {0};

byte bitTimeCount = 0; //Counts the number of intterupts recived, places timing data into a array.

//Used to itierate throug hte data arrays
byte decodingCount1 = 0;
byte decodingCount2 = 0;

//Command and adress bytes as pulled from IR singal
byte command = 0x00;
byte address = 0x00;

byte errorCode = 0x00;

byte IPIN = 0;

bool repeat = 0;

//Defines NEC bit standards to make code more readable
#define NEC_HEADER_PULSE_MIN 8700
#define NEC_HEADER_PULSE_MAX 9500
#define NEC_HEADER_SPACE_MIN 4000
#define NEC_HEADER_SPACE_MAX 5000
#define NEC_BIT_SPACE_MIN 400
#define NEC_BIT_SPACE_MAX 700
#define NEC_BIT_0_MIN 200
#define NEC_BIT_0_MAX  860
#define NEC_BIT_1_MIN 1390
#define NEC_BIT_1_MAX 1990

#define NEC_REPEAT_SPACE_MIN 1750
#define NEC_REPEAT_SPACE_MAX 2750
#define NEC_REPEAT_FOOTER_MIN 200
#define NEC_REPEAT_FOOTER_MAX 860

//Defines errors for debugging
#define errorCode_NO_START 0x01
#define errorCODE_IMPROPER_BIT 0x02
#define errorCODE_IMPROPER_SPACE 0x03
//Rest loop, restes all changing valuse after decoding.
void IRReset(){
    EndBit = 1;
    StartBit = 0;

    decodingCount1 = 2;
    decodingCount2 = 0;
    bitTimeCount = 0;
 
    if(errorCode != 0){
        Serial.print("Error: 0x");
        Serial.println(errorCode, HEX);
        errorCode = 0x00; 
    }    
    memset((void*)bitLength, 0, sizeof(bitLength));
    memset(IRBits, 0, sizeof(IRBits));
}
//Pulls address from IR singal, first 8 bits.
byte addr(){
    if(!repeat){
        address = 0;
        for(int i = 0; i < 8; i++){
            address|= (IRBits[i]<<i);
            
        }
    }else{

    }
    return address;
}
//Pulls command from IR singal, bits 15-23
byte cmd(){
    if(!repeat){
        command = 0;
        for(int i = 15; i < 23; i++){
            command |= (IRBits[i]<<(i-15));

        }
    }else{

    }
        return command;
}

//Sets pin change interupt mask
int intalizeIR(byte IRpin){
    if(IRpin == 3){ //For pin 3
        PCMSK0 |= (1<<PCINT1); //Pin change mask

        EICRA |= (1 << ISC00);    // ISC00 = 1
        EICRA &= ~(1 << ISC01);   // ISC01 = 0
        EIMSK |= (1 << INT1);     // Enable INT0

    }
    if(IRpin == 2){ //For pin 2
        PCMSK0 |= (1<<PCINT0); //Pin change mask

        EICRA |= (1 << ISC00);    // ISC00 = 1
        EICRA &= ~(1 << ISC01);   // ISC01 = 0
        EIMSK |= (1 << INT0);     // Enable INT0
  
    }
    IPIN = IRpin;
    return 0;
}

//Intterupt service routine, stores all pulse lengths in a array, tends to be far more relaiable than RT decoding.
//Pin 2
ISR (INT0_vect){
if(IPIN == 2){
    if(!StartBit){
        previousISRTime = bitDecoding;
        bitDecoding = micros();

        if(previousISRTime != 0){
            bitLength[bitTimeCount] = bitDecoding-previousISRTime; //Stores the pulse length
            bitTimeCount++; //Moves onto the next part of the array

            }
         }
    }
}
//Pin 3
ISR (INT1_vect){
if(IPIN == 3){
    if(!StartBit){
        previousISRTime = bitDecoding;
        bitDecoding = micros();

        if(previousISRTime != 0){
            bitLength[bitTimeCount] = bitDecoding-previousISRTime; //Stores the pulse length
            bitTimeCount++; //Moves onto the next part of the array

            }
        }
    }
}
//Decodes the signal into bits
void Decode(){
    int startindx = -1;
    if (bitLength[0] > 15000) {
    //Discard and reset
    IRReset();
    return;
}
//Debuggin shanganins
/*if(bitTimeCount != 0){
    Serial.print("bitTimeCount: ");
    Serial.println(bitTimeCount);
    Serial.print("bitLength[0] = ");
    Serial.println(bitLength[0]);
    Serial.print("bitLength[1] = ");
    Serial.println(bitLength[1]);
    Serial.print("bitLength[2] = ");
    Serial.println(bitLength[2]);
    Serial.print("bitLength[3] = ");
    Serial.println(bitLength[3]);
} */
//This routine handles repeat singals.
if(bitTimeCount >= 4){
    //Checks data array for a reapte singal, easier to handle since its just 3 diffrent pulses
    for(int i =0; i < bitTimeCount - 1; i++){
        if ((bitLength[i] >= NEC_HEADER_PULSE_MIN && bitLength[i] <= NEC_HEADER_PULSE_MAX) &&
            (bitLength[i + 1] >= NEC_REPEAT_SPACE_MIN && bitLength[i + 1] <= NEC_REPEAT_SPACE_MAX) &&
            (bitLength[i + 2] >= NEC_REPEAT_FOOTER_MIN && bitLength[i + 2] <= NEC_REPEAT_FOOTER_MAX)) {
                repeat = true;
                //Serial.println("REPEAT");
                /**** REMOVE AFTER DEBUGGIN*** */
                //delay(1000);
                cmd();
                addr();
                IRReset();
                return;
                    }
                }
            }
if(bitTimeCount >= 65){ //Ensures at least 32 pulses have been counted, or 32 bits have been recieved
    decodingCount1 = 0;
    decodingCount2 = 0;
    if(!StartBit) {
        //Looks for the start bit among the pulse length array, if found starts decoding.
        
        for(int i =0; i <bitTimeCount - 1; i++){
                if((bitLength[i] >= NEC_HEADER_PULSE_MIN) && (bitLength[i] <= NEC_HEADER_PULSE_MAX) 
                && (bitLength[i + 1] >= NEC_HEADER_SPACE_MIN) && (bitLength[i + 1] <= NEC_HEADER_SPACE_MAX)) {
                startindx = i;
                //Serial.println("starting1");
                break;
            }
        }
        
    if(startindx != -1){ 
            StartBit = 1;
            EndBit = 0;
            decodingCount1 = startindx + 2;  //Sets which bits to start decdoing on.
            decodingCount2 = 0;
            memset(IRBits, 0, sizeof(IRBits)); //Clears previous data out of array.
            //Serial.println("starting2");
            } 
            
    }else{
        //Womp womp no start bit
        errorCode = errorCode_NO_START;
        IRReset();
        return;

    }
    while (decodingCount2 <= 32 && !EndBit){
    
        //This is where the decoding happens
        //First if statments checks for the start pulse, indicating that a bit is being sent, and it is roughly 

        if((bitLength[decodingCount1] >= NEC_BIT_SPACE_MIN) && (bitLength[decodingCount1] <= NEC_BIT_SPACE_MAX)){
            decodingCount1++; //Increments to next data point, to check for the space

            //Checks the space timing for interpation a 0 or one
            if((bitLength[decodingCount1] >= NEC_BIT_0_MIN) && (bitLength[decodingCount1] <= NEC_BIT_0_MAX)){
                IRBits[decodingCount2] = 0;
                decodingCount2++; //Moves to check for next bit

            }
            else if((bitLength[decodingCount1] >= NEC_BIT_1_MIN) && (bitLength[decodingCount1] <= NEC_BIT_1_MAX)){
                IRBits[decodingCount2] = 1;
                decodingCount2++; // Moves to check for next bit

            }else{
                    decodingCount1 += 2; // Skip the pulse and space
                    decodingCount2++;    // Still move forward in output array
                    errorCode = errorCODE_IMPROPER_BIT;
                    //Serial.println("Error: ");
                    //Serial.print("0x");
                    //Serial.print(errorCode, HEX);
                    continue;
                }
                    decodingCount1++;
        }else{
            //womp womp noise or the signal was incorrectly transmited
            decodingCount1 += 2; // Skip the pulse and space
            decodingCount2++;    // Still move forward in output array
            errorCode = errorCODE_IMPROPER_SPACE;
            continue;
                    
        }
        if(decodingCount2 >= 32){ //Clears all vars and ends the decoding.
            cmd();
            addr();
            //Serial.print("address: 0x");
            //Serial.print(address, HEX);
            //Serial.print("   command: 0x");
            //Serial.println(command, HEX);
            //repeat = false;
            IRReset();
            /* ***REMOVE AFTER DEBUGGIN*** */
            //delay(10000);
            

            }
        }
    } 
    /*This is the main chunk of the noise handling, clearing the arrays and vars after a 12ms pulse time out
    and if the bit timer array overflows, while also checking for valid singals to remove the possblity of valid signals getting
    timed out*/
    if((bitTimeCount == 71 && !StartBit) || (micros() - bitDecoding > 12000)){
        IRReset();
        return;
        //Serial.println("timeout");
    }
}
