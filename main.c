/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.65
        Device            :  PIC16F18875
        Driver Version    :  2.00
 */

/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
 */
 
#include "mcc_generated_files/mcc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

// Size of (circular)buffer for acquired EMG samples
#define SB_DATA_WINDOW 50

// Sampling frequency of signal. 50Hz = 20ms (Timer interrupt configured to 20ms)
#define SAMPLE_FREQUENCY 50
#define MIN_FREQUENCY 25
#define MAX_FREQUENCY 150

// The minimum distance in data points that two registered peaks can be
#define MIN_PK_GAP ((int)(SAMPLE_FREQUENCY / MAX_FREQUENCY))
// The maximum distance two consecutive peaks can be without significantly shifting the calculated neutral-point  
#define MAX_PK_GAP ((int)(SAMPLE_FREQUENCY / MIN_FREQUENCY))
// Size of (circular) buffer for Peak Filter i.e., Twice the maximum gap
#define PK_DATA_WINDOW (2*MAX_PK_GAP+1)

// Size of (circular) buffer for Moving Average Filter. Considers last 0.5 seconds of data for Moving Average 
#define MA_DATA_WINDOW ((int)(SAMPLE_FREQUENCY * 0.5)+1)

// Buffer for acquired EMG data and indices for the Circular Buffer
uint16_t sb_data[SB_DATA_WINDOW];
int8_t sb_front = -1;               
int8_t sb_rear = -1;

/*
 Arrays in C start from the Index 0. A 0 index on the circular buffer indicates there is already an element present in Array[0] 
 Hence we begin the front and rear pointers of each circular buffer with '-1', so that every advance in the pointer indicates the buffer has an element
 This implementation is used for all three circular buffers used : sb_data (Signal Buffer), pk_data (Peak to Peak Filter) and, ma_data (Moving Average Filter) 
 */

// Buffer for Peak Filter and indices for the Circular Buffer
uint16_t pk_data[PK_DATA_WINDOW];
int8_t pk_front = -1;
int8_t pk_rear = -1;

// Buffer for Moving Average Filter and indices for the Circular Buffer
uint16_t ma_data[MA_DATA_WINDOW];
int8_t ma_front = -1;
int8_t ma_rear = -1;

// Sum of all elements in a Moving Average window
uint16_t ma_window_sum = 0;

// Flag that starts data acquisition
uint8_t start_flag = 0;
uint8_t i;

// Flags to indicate the character sent over UART to the Slave PIC
uint8_t sent_1 =0;
uint8_t sent_0 =0;

// Returns true if Circular Buffer is full
bool sbuf_isfull() {
    if ((sb_front == sb_rear + 1) || (sb_front == 0 && sb_rear == SB_DATA_WINDOW - 1))
        return true;
    else
        return false;
}

// Returns true if Circular Buffer is empty
bool sbuf_isempty() {
    if (sb_front == -1)
        return true;
    else
        return false;
}

// Inserts element into the Circular Buffer
bool sbuf_insert(uint16_t element) {
    if (sbuf_isfull()) {
        // Can't insert data since buffer is full
        return false;
    } 
	
	else {
		
        if (sb_front == -1)
            sb_front = 0;
		
        sb_rear = (sb_rear + 1) % SB_DATA_WINDOW;
        sb_data[sb_rear] = element;
        return true;
    }
}

// Removes element from the Circular Buffer
bool sbuf_remove() {
    uint16_t element;
	
    if (sbuf_isempty()) {
        return false;
    } 
	
	else {
        element = sb_data[sb_front];
        if (sb_front == sb_rear) {
            sb_front = -1;
            sb_rear = -1;
        } else {
            sb_front = (sb_front + 1) % SB_DATA_WINDOW;
        }
        return true;
    }
}

// Returns the first inserted element (FIFO) from the Circular Buffer
uint16_t sbuf_peek(){
  return sb_data[sb_front];
}

bool pkdata_isfull() {
    if ((pk_front == pk_rear + 1) || (pk_front == 0 && pk_rear == PK_DATA_WINDOW - 1))
        return true;
    else
        return false;
}

// Similar Circular Buffer functions for Peak Filter

bool pkdata_isempty() {
    if (pk_front == -1)
        return true;
    else
        return false;
}

bool pkdata_insert(uint16_t element) {
    if (pkdata_isfull()) {
        // Can't insert data because buffer is full
        return false;
    } else {
        if (pk_front == -1)
            pk_front = 0;
        pk_rear = (pk_rear + 1) % PK_DATA_WINDOW;
        pk_data[pk_rear] = element;
        return true;
    }
}

bool pkdata_remove() {
    uint16_t element;
    if (pkdata_isempty()) {
        return false;
    } else {
        element = pk_data[pk_front];
        if (pk_front == pk_rear) {
            pk_front = -1;
            pk_rear = -1;
        } else {
            pk_front = (pk_front + 1) % PK_DATA_WINDOW;
        }
        return true;
    }
}

// Similar circular buffer functions for Moving Average Filter

bool madata_isfull() {
    if ((ma_front == ma_rear + 1) || (ma_front == 0 && ma_rear == MA_DATA_WINDOW - 1))
        return true;
    else
        return false;
}

bool madata_isempty() {
    if (ma_front == -1)
        return true;
    else
        return false;
}

bool madata_insert(uint16_t element) {
    if (madata_isfull()) {
        // Can't insert data because buffer is full
        return false;
    } else {
        if (ma_front == -1)
            ma_front = 0;
        ma_rear = (ma_rear + 1) % MA_DATA_WINDOW;
        ma_data[ma_rear] = element;
        return true;
    }
}

bool madata_remove() {
    uint16_t element;
    if (madata_isempty()) {
        return false;
    } else {
        element = ma_data[ma_front];
        if (ma_front == ma_rear) {
            ma_front = -1;
            ma_rear = -1;
        } else {
            ma_front = (ma_front + 1) % MA_DATA_WINDOW;
        }
        return true;
    }
}


// Peak Filter returns the neutral_datapoint calculated using (highest_peak+lowest_peak)/2. 
// Highest & Lowest peaks are the maximum and minimum among the elements present in the "Peak Filter Buffer" (pk_data[PK_DATA_WINDOW])

uint16_t get_neutral_peaktopeak(uint16_t datapoint) {
    // Insert every new data point into the Peak Filter Buffer
    pkdata_insert(datapoint);

    // If buffer is full, remove the first inserted data (FIFO)
    if (pkdata_isfull()) {
        pkdata_remove();
    }

    // Start off with first data point for highest peak
    uint16_t highest_peak = pk_data[pk_front];
    uint16_t lowest_peak = pk_data[pk_front];
    uint16_t neutral;
    
    uint8_t i;
    
    // Running through only the valid elements of the array
    for (i = pk_front; i != pk_rear; i = (i + 1) % PK_DATA_WINDOW) {
        if (pk_data[i] > highest_peak) {
            // Reassign variable highest_peak if a new highest is found among elements of "Peak Filter buffer"
            highest_peak = pk_data[i];
        }
        if (pk_data[i] < lowest_peak) {
            // Reassign variable lowest_peak if a new lowest is found among elements of "Peak Filter buffer"
            lowest_peak = pk_data[i];
        }
    }

    // Calculate neutral and return
    neutral = (highest_peak + lowest_peak) / 2;
    return neutral;
}

// Moving Average Filter Returns the average of data points present in the "Moving Average Buffer"
// The sum in updated for every new data point added(or/and removed)
float get_moving_average(uint16_t datapoint) {
    madata_insert(datapoint);
    // ma_window_sum starts at 0 and every new data point is added to this sum
    ma_window_sum += datapoint;

    if (madata_isfull()) {
        // When the buffer is full, the last element is subtracted from the sum and then removed from the buffer
        ma_window_sum -= ma_data[ma_front];
        madata_remove();
    }

    // MA_DATA_WINDOW is 1 more than intended Moving Average Window
    return ma_window_sum / (MA_DATA_WINDOW-1);
}

// Interrupt handler is called every 20ms effectively sampling the EMG signal at 50Hz
void TMR6_EMG_InterruptHandler(void)
{
    if (start_flag == 1) {
        //ADCC_StartConversion(POT_RA0);
        ADCC_StartConversion(EMG_RA2);
        
        adc_result_t adval = ADCC_GetConversionResult();
        
        // Every new sample point is added to the "Signal Circular Buffer"
        // Data is "buffered" (until the limit of 50 data points) so the main loop can get the first inserted element (FIFO)
        // whenever it is free and process it.
       
        sbuf_insert(adval/100);
    }
}


void main(void)
{
    // Initialize the Device
    SYSTEM_Initialize();
    // ^-^

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    // Initialize custom interrupt handler which acquires data from ADC
    TMR6_SetInterruptHandler(TMR6_EMG_InterruptHandler);
    TMR6_Start();
    
    int count = 0;
    
    uint16_t datapoint;
    
    uint16_t neutral_datapoint, result;
    
    uint8_t mode = 0;
    
    double time_elapsed;
    
    uint8_t flex_flag, motor_started;
    
    flex_flag = 0;
    motor_started = 0;

    // Mode 0 -- Flex and Release for Motor ON and Motor OFF (LED_RA7 OFF)
    // Mode 1 -- Flex and Flex for Motor ON and Motor OFF (LED_RA7 ON)
    
    LED_RA7_SetLow();
    while (1)
    {
        // Press Switch-1 to change the Mode (Reset Microcontroller after every Mode change)
        if (MODE_RB4_GetValue() == 0)  
        {
            mode ^= 1;
            LED_RA7_Toggle();
            __delay_ms(700);
        }
         
		// When the flag is cleared and Switch is pressed, the process starts
        if (START_RC5_GetValue() == 0 && start_flag == 0) {
            //printf("START\r\n");
            start_flag = 1;
            __delay_ms(700);
            break;
        }
    }
    
    while (1)
    {
        if(start_flag == 1)
        {
            // Count the number of data points present in signal buffer
// ******** // todo... is count required anymore ?
            for (i = sb_front; i != sb_rear; i = (i + 1) % SB_DATA_WINDOW) {
                count++;
            }

            if(count>0)
            {
                // Get the first inserted data point (in FIFO order)
                datapoint = sbuf_peek();
                
                // Calculates the neutral peak by passing the data point to the Peak Filter
                neutral_datapoint = get_neutral_peaktopeak(datapoint);
                
                // Uses the neutral point to subtract from the current data point
                // Signal is rectified by taking absolute difference between the two points
                // Moving Average filter is applied to the obtained peak values 
                result = get_moving_average(abs(datapoint - neutral_datapoint));
                
                if(mode == 0)
                {
                    // Turn motor at muscle flex & Turn back motor at muscle release
                    if(result>= 25 && sent_1 == 0)
                    {
                        printf("1");
                        sent_1 = 1;
                        sent_0 = 0;
                    }
                    else if(result<25 && sent_0 == 0)
                    {
                        printf("0");
                        sent_0 = 1;
                        sent_1 = 0;
                    }
                }
                
                else
                {
                    // Turn motor at Muscle flex & Turn back motor at next muscle flex
                    if(result>= 25 && flex_flag == 0)
                    {
                        flex_flag = 1;
                        
						if(motor_started == 1)
                        {
                            motor_started =0;
                            printf("0");
                        }
                        else
                        {
                            motor_started = 1;
                            printf("1");
                        }
                        
                    }
                    
                    else if(result<25 && flex_flag == 1)
                    {
                        flex_flag = 0;
                    }
                }
                // **** todo...Reject/ donot consider result until atleast 2*MIN_PK_GAP is calculated ????
                
                // Remove the first element (in FIFO order) since it has already been processed
                sbuf_remove();
                
                time_elapsed += 5.0;
            }

            count = 0;
        }
    }
}
/**
 End of File
*/
