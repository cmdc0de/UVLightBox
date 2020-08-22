/*
 * app.cpp
 *
 * Author: cmdc0de
 */

#include "app.h"
#include <esp_log.h>
#include <system.h>
#include <spibus.h>
#include <device/display/frame_buffer.h>
#include <device/display/display_device.h>
#include <device/display/fonts.h>
#include <device/display/gui.h>
#include <device/touch/XPT2046.h>

#include "menus/menu_state.h"
#include "menus/calibration_menu.h"
#include <app/display_message_state.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include "freertos.h"
#include "fatfsvfs.h"
#include "pinconfig.h"

using libesp::ErrorType;
using libesp::DisplayILI9341;
using libesp::XPT2046;
using libesp::GUI;
using libesp::DisplayMessageState;
using libesp::BaseMenu;
using libesp::System;
using libesp::FreeRTOS;

const char *MyApp::LOGTAG = "AppTask";
const char *MyApp::sYES = "Yes";
const char *MyApp::sNO = "No";


#define START_ROT libesp::DisplayILI9341::LANDSCAPE_TOP_LEFT
static const uint16_t PARALLEL_LINES = 1;

libesp::DisplayILI9341 Display(MyApp::DISPLAY_WIDTH,MyApp::DISPLAY_HEIGHT,START_ROT,
	PIN_NUM_DISPLAY_BACKLIGHT, PIN_NUM_DISPLAY_RESET);

static uint16_t BkBuffer[MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT];
static uint16_t *BackBuffer = &BkBuffer[0];
//static uint16_t *BackBuffer = new uint16_t[MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT];

uint16_t ParallelLinesBuffer[MyApp::DISPLAY_WIDTH*PARALLEL_LINES] = {0};

libesp::ScalingBuffer FrameBuf(&Display, MyApp::FRAME_BUFFER_WIDTH, MyApp::FRAME_BUFFER_HEIGHT, uint8_t(16), MyApp::DISPLAY_WIDTH,MyApp::DISPLAY_HEIGHT, PARALLEL_LINES, (uint8_t*)&BackBuffer[0],(uint8_t*)&ParallelLinesBuffer[0]);

static GUI MyGui(&Display);
static XPT2046 TouchTask(PIN_NUM_TOUCH_IRQ,true);
static CalibrationMenu MyCalibrationMenu;

const char *MyErrorMap::toString(int32_t err) {
	return "TODO";
}

MyApp MyApp::mSelf;

MyApp &MyApp::get() {
	return mSelf;
}

uint8_t *MyApp::getBackBuffer() {
	return (uint8_t *)&BackBuffer[0];
}

uint32_t MyApp::getBackBufferSize() {
	return MyApp::FRAME_BUFFER_WIDTH*MyApp::FRAME_BUFFER_HEIGHT*2;
}


MyApp::MyApp() : MyNvsHandle(0), AppErrors()  {
	ErrorType::setAppDetail(&AppErrors);
}

MyApp::~MyApp() {

}

XPT2046 &MyApp::getTouch() {
	return TouchTask;
}

libesp::ErrorType MyApp::onInit() {
	ErrorType et;

	ESP_LOGI(LOGTAG,"OnInit: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	et = MyCalibrationMenu.initNVS();
	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to init nvs for calibration");
		return et;
	}

	et = XPT2046::initTouch(PIN_NUM_TOUCH_MISO, PIN_NUM_TOUCH_MOSI, PIN_NUM_TOUCH_CLK,VSPI_HOST, 1);
	ESP_LOGI(LOGTAG,"After Touch and Calibration: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch");
		return et;
	}

	libesp::SPIBus* bus = libesp::SPIBus::get(VSPI_HOST);
	et = TouchTask.init(bus,PIN_NUM_TOUCH_CS);

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"failed to touch SPI");
		return et;
	}

	DisplayILI9341::initDisplay(PIN_NUM_DISPLAY_MISO, PIN_NUM_DISPLAY_MOSI,
			PIN_NUM_DISPLAY_CLK, 2, PIN_NUM_DISPLAY_DATA_CMD, PIN_NUM_DISPLAY_RESET,
			PIN_NUM_DISPLAY_BACKLIGHT, HSPI_HOST);

	ESP_LOGI(LOGTAG,"After Display: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	bus = libesp::SPIBus::get(HSPI_HOST);

	FrameBuf.createInitDevice(bus,PIN_NUM_DISPLAY_CS,PIN_NUM_DISPLAY_DATA_CMD);
	
	ESP_LOGI(LOGTAG,"After FrameBuf: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	ESP_LOGI(LOGTAG,"start display init");
	et=Display.init(libesp::DisplayILI9341::FORMAT_16_BIT, &Font_6x10, &FrameBuf);
	if(et.ok()) {
		ESP_LOGI(LOGTAG,"display init OK");
		Display.fillScreen(libesp::RGBColor::BLACK);
		Display.swap();
		ESP_LOGI(LOGTAG,"fill black done");
		Display.fillRec(0,0,FRAME_BUFFER_WIDTH/4,10,libesp::RGBColor::RED);
		Display.swap();
		vTaskDelay(500 / portTICK_RATE_MS);
		Display.fillRec(0,15,FRAME_BUFFER_WIDTH/2,10,libesp::RGBColor::WHITE);
		Display.swap();
		vTaskDelay(1000 / portTICK_RATE_MS);
		Display.fillRec(0,30,FRAME_BUFFER_WIDTH,10,libesp::RGBColor::BLUE);
		Display.swap();
		vTaskDelay(1000 / portTICK_RATE_MS);
		Display.drawRec(0,60,FRAME_BUFFER_WIDTH/2,20, libesp::RGBColor::GREEN);
		Display.drawString(15,110,"Color Validation.",libesp::RGBColor::RED);
		Display.drawString(30,120,"UVLight Box",libesp::RGBColor::BLUE, libesp::RGBColor::WHITE,1,false);
		Display.swap();

		vTaskDelay(1000 / portTICK_RATE_MS);
		ESP_LOGI(LOGTAG,"After Display swap:Free: %u, Min %u",System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());

	} else {
		ESP_LOGE(LOGTAG,"failed display init");
	}
	
	TouchTask.start();
	ESP_LOGI(LOGTAG,"After Task starts: Free: %u, Min %u", System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
#ifdef 0
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = ((1UL<<PIN_BLUE) | (1UL<<PIN_GREEN) | (1UL<<PIN_RED));
	//disable pull-down mode
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	//disable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
	gpio_set_level(PIN_RED, 1);
	gpio_set_level(PIN_GREEN, 0);
	gpio_set_level(PIN_BLUE, 0);
	vTaskDelay(1000 / portTICK_RATE_MS);
	gpio_set_level(PIN_RED, 0);
	gpio_set_level(PIN_GREEN, 1);
	gpio_set_level(PIN_BLUE, 0);
	vTaskDelay(1000 / portTICK_RATE_MS);
	gpio_set_level(PIN_RED, 0);
	gpio_set_level(PIN_GREEN, 0);
	gpio_set_level(PIN_BLUE, 1);
	vTaskDelay(1000 / portTICK_RATE_MS);
	gpio_set_level(PIN_RED, 0);
	gpio_set_level(PIN_GREEN, 0);
	gpio_set_level(PIN_BLUE, 0);
	vTaskDelay(1000 / portTICK_RATE_MS);
	gpio_set_level(PIN_RED, 1);
	gpio_set_level(PIN_GREEN, 1);
	gpio_set_level(PIN_BLUE, 1);
	vTaskDelay(1000 / portTICK_RATE_MS);
#endif
	setCurrentMenu(getMenuState());
	return et;
}

uint16_t MyApp::getCanvasWidth() {
	return FrameBuf.getBufferWidth(); 
}

uint16_t MyApp::getCanvasHeight() {
	return FrameBuf.getBufferHeight();
}
uint16_t MyApp::getLastCanvasWidthPixel() {
	return getCanvasWidth()-1;
}

uint16_t MyApp::getLastCanvasHeightPixel() {
	return getCanvasHeight()-1;
}
	
libesp::DisplayDevice &MyApp::getDisplay() {
	return Display;
}

libesp::GUI &MyApp::getGUI() {
	return MyGui;
}

ErrorType MyApp::onRun() {
#if 0
		  return ErrorType();
#else
	TouchTask.broadcast();
	libesp::BaseMenu::ReturnStateContext rsc = getCurrentMenu()->run();
	Display.swap();

	if (rsc.Err.ok()) {
		if (getCurrentMenu() != rsc.NextMenuToRun) {
			setCurrentMenu(rsc.NextMenuToRun);
			ESP_LOGI(LOGTAG,"on Menu swap: Free: %u, Min %u",
				System::get().getFreeHeapSize(),System::get().getMinimumFreeHeapSize());
		} else {
/*
			uint32_t screenSaveTime = MyApp::get().getContacts().getSettings().getScreenSaverTime()*1000*60;
			uint32_t now = FreeRTOS::getTimeSinceStart();
			if (getCurrentMenu() != MyApp::get().getGameOfLifeMenu()
				&& (((now-MyButtons.lastTickButtonPushed())>screenSaveTime)
				&& ((now-TouchTask.lastTickScreenTouch()>screenSaveTime)))) {
				setCurrentMenu(MyApp::get().getGameOfLifeMenu());
			}
				*/
		}
	} else {
		//setCurrentState(StateCollection::getDisplayMessageState(
		//		StateCollection::getDisplayMenuState(), (const char *)"Run State Error....", uint16_t (2000)));
	}
	return ErrorType();
#endif
}

MenuState MyMenuState;
libesp::DisplayMessageState DMS;
	
MenuState *MyApp::getMenuState() {
	return &MyMenuState;
}

CalibrationMenu *MyApp::getCalibrationMenu() {
	return &MyCalibrationMenu;
}

DisplayMessageState *MyApp::getDisplayMessageState(BaseMenu *bm, const char *msg, uint32_t msDisplay) {
	DMS.setMessage(msg);
	DMS.setNextState(bm);
	DMS.setTimeInState(msDisplay);
	DMS.setDisplay(&Display);
	return &DMS;

}

