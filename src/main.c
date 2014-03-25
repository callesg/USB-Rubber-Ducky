//_____  M A I N ___________________________________________________________
//
// Module		: Ducky USB Conf
// Description	: Composite Ducky USB HID Keyboard & Mass Storage USB Configurable
// Date			: March 25, 2014
// Author       : Calle S
// Credit		: ATMEL, Jason Applebaum's Keyscan Method, Snake for the original source
//__________________________________________________________________________

#include <asf.h>
#include "conf_usb.h"
#include "ui.h"
#include "conf_sd_mmc_spi.h"

static bool LedStatus = false;
static bool main_b_keyboard_enable = false;
static bool main_b_msc_enable = false;

#define KEY_SPEED_COUNT 6
static int KeySpeeds[KEY_SPEED_COUNT];

static int printstate = 0;
static int start_state = 0;


static char *KeyBoardMapFile = "A:\\config\\keymap.map";
static char *KeySpeedFile = "A:\\config\\speed.conf";
static char *TextFile = "A:\\texts\\main.txt";
#define MAX_TEXT 16000
static char PrintText[MAX_TEXT];
static int PrintTextLen = 0;
static int PrintTextPos = 0;

static int CurrentCharID = 0;

struct CharacterDefiner {
	char KeyCode;
	char Modifiers;
};
#define CHAR_COUNT 254
struct CharacterDefiner Characters[CHAR_COUNT];


UDC_DESC_STORAGE usb_dev_desc_t udc_device_desc;

/*! \brief Main function. Execution starts here.
*/
int main(void){
	irq_initialize_vectors();
	cpu_irq_enable();

	// Initialize the sleep manager
	sleepmgr_init();

	sysclk_init();
	board_init();
	ui_init();
	ui_powerdown();

	memories_initialization(FOSC0);
	
	//Set default typing speed
	for(int KeyInd=0;KeyInd < KEY_SPEED_COUNT;KeyInd++){
		KeySpeeds[KeyInd] = KeyInd;
	}
	
	//Reset the Character map
	for(int CharInd=0;CharInd < CHAR_COUNT;CharInd++){
		Characters[CharInd].KeyCode = 0;
	}
	
	//Read the typing speed config file
	nav_reset();
	if( nav_setcwd( KeySpeedFile, false, false ) ) {
		file_open(FOPEN_MODE_R);
		file_bof();
		
		
		int CharInd=0;
		int FileCharInd = 0;
		int SpeedConfNum = 0;
		char ConfNumber[16];
		while(!file_eof() && KEY_SPEED_COUNT > SpeedConfNum) {//Read The character definition
			char Character = (char)file_getc();
			ConfNumber[CharInd] = Character;
			if(Character == '\n'){//When we reach the line eding pharse the number 
				ConfNumber[CharInd] = 0;
				CharInd = 0;
				KeySpeeds[SpeedConfNum] = atoi(ConfNumber);
				SpeedConfNum++;
			}else if(Character == '='){//Anything before Equal signs is ignored
				CharInd = 0;
			}else{
				CharInd++;
			}
			FileCharInd++;
		}
		ConfNumber[CharInd] = 0;
		
		file_close();
	}else{
	}
	
	//Save the Character map to memory
	nav_reset();
	if( nav_setcwd( KeyBoardMapFile, false, false ) ) {
		file_open(FOPEN_MODE_R);
		file_bof();
		
		
		int CharInd=0;
		while(!file_eof() && CHAR_COUNT > CharInd) {//Read The character definition
			int Character = (int)file_getc();
			Characters[Character].KeyCode = (char)file_getc();
			Characters[Character].Modifiers = (char)file_getc();
		}
		file_close();
	}else{
	}
	
	//Save the text that we are about to print to memory
	nav_reset();
	if( nav_setcwd( TextFile, false, false ) ) {
		file_open(FOPEN_MODE_R);
		file_bof();
		
		//Read the text
		PrintTextLen = 0;
		while(!file_eof() && MAX_TEXT > PrintTextLen) {
			PrintText[PrintTextLen] = (char)file_getc();
			PrintTextLen++;
		}
		file_close();
	}else{
	}
	nav_reset();
	
	//memories_initialization(FOSC0);
	// Start USB stack to authorize VBus monitoring
	udc_start();
	udc_attach();
	
	// The main loop manages only the power mode
	// because the USB management is done by interrupt
	while (true) {

		if (main_msc_enable) {
			if (!udi_msc_process_trans()) {
				sleepmgr_enter_sleep();
			}
		}else{
			sleepmgr_enter_sleep();
		}
	}
}

//is there more bytes to print
bool ReachedTextEnd(){
	if(PrintTextLen <= PrintTextPos){
		return(true);
	}
	return(false);
}

//get next byte to print
int getNextChar(){
	int textpos = PrintTextPos;
	PrintTextPos++;
	
	return(PrintText[textpos]);
}

void main_vbus_action(bool b_high)
{
	if (b_high) {
		// Attach USB Device
		udc_attach();
	} else {
		// VBUS not present
		udc_detach();
	}
}

void main_suspend_action(void){
	ui_powerdown();
}

void main_resume_action(void){
	ui_wakeup();
}

void main_sof_action(void){
	if ((!main_b_keyboard_enable)){
		return;
	}
	process_frame(udd_get_frame_number());
}

void main_remotewakeup_enable(void){
	ui_wakeup_enable();
}

void main_remotewakeup_disable(void){
	ui_wakeup_disable();
}

bool main_keyboard_enable(void){
	main_b_keyboard_enable = true;
	return true;
}

void main_keyboard_disable(void){
	main_b_keyboard_enable = false;
}

bool main_msc_enable(void){
	main_b_msc_enable = true;
	return main_b_msc_enable;
}

void main_msc_disable(void){
	main_b_msc_enable = false;
}


void memories_initialization(long pba_hz){
	#if (defined SD_MMC_SPI_MEM) && (SD_MMC_SPI_MEM == ENABLE)
	// SPI options.
	spi_options_t spiOptions = {
		.reg          = SD_MMC_SPI_NPCS,
		.baudrate     = SD_MMC_SPI_MASTER_SPEED,  // Defined in conf_sd_mmc_spi.h.
		.bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc_spi.h.
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	sysclk_enable_peripheral_clock(SD_MMC_SPI);

	// If the SPI used by the SD/MMC is not enabled.
	if (!spi_is_enabled(SD_MMC_SPI)) {
		// Initialize as master.
		spi_initMaster(SD_MMC_SPI, &spiOptions);
		// Set selection mode: variable_ps, pcs_decode, delay.
		spi_selectionMode(SD_MMC_SPI, 0, 0, 0);
		// Enable SPI.
		spi_enable(SD_MMC_SPI);
	}

	// Initialize SD/MMC with SPI PB clock.
	sd_mmc_spi_init(spiOptions,pba_hz);
	#endif  // SD_MMC_SPI_MEM == ENABLE
}


//Trigger stuff on caps or numlock or similar
void main_kbd_change(uint8_t value){
	//IN OSX each keyboard has its own capslock settings
	if(!LedStatus){
		LED_On( LED1 );
		LedStatus = true;
		}else{
		LED_Off( LED1 );
		LedStatus = false;
	}
	
	//this is called when LEDs CAPS LCK, NUM LCK change
	if (value & HID_LED_NUM_LOCK) {
		// Here, turn on Num LED
		LED_On( LED1 );
	} else{
		// Here, turn off Num LED
		LED_Off( LED1 );
	}
	if (value & HID_LED_CAPS_LOCK) {
		// Here, turn on CAPS LED
		LED_On( LED0 );
	} else{
		// Here, turn off CAPS LED
		LED_Off( LED0 );
	}
}

//Is executed on each usb frame
void process_frame(uint16_t framenumber){
	if(start_state > 2000){//Start printing on frame 2000 so that drivers get installed and such
		
		
		if(printstate == KeySpeeds[0]){//Load a new character
			if(!ReachedTextEnd()){
				CurrentCharID = getNextChar();
				if(Characters[CurrentCharID].KeyCode == 0){//If it is a unknown Character ignore it
					return;
				}
			}else{
				//Text has ended set it above 40 so that it wont run again (untill rollover)
				printstate = KeySpeeds[5]+1;
			}
		}
		
		if(printstate == KeySpeeds[1]){//First we send the modifiers
			if(Characters[CurrentCharID].Modifiers != 0){//Only if there is modifiers
				udi_hid_kbd_modifier_down(Characters[CurrentCharID].Modifiers);
			}else{
				printstate = KeySpeeds[2];
			}
		}
		if(printstate == KeySpeeds[2]){//Then we press the button
			udi_hid_kbd_down(Characters[CurrentCharID].KeyCode);
		}
		
		if(printstate == KeySpeeds[3]){//Then reslse the button
			udi_hid_kbd_up(Characters[CurrentCharID].KeyCode);
		}
		if(printstate == KeySpeeds[4]){//And relse the modifiers
			if(Characters[CurrentCharID].Modifiers != 0){//Only if there is modifiers
				udi_hid_kbd_modifier_up(Characters[CurrentCharID].Modifiers);
			}else{
				printstate = KeySpeeds[5];
			}
		}
		
		
		if(printstate == KeySpeeds[5]){//When we get to the last print state reset and go back to the first
			printstate = KeySpeeds[0];
		}else if(printstate < KeySpeeds[5]){ //if we have gone over our last print state
			printstate += 1;
		}
	}else{
		start_state++;
	}
}