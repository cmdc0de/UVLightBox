#include "menu_state.h"
#include "../app.h"
#include "gui_list_processor.h"
#include "calibration_menu.h"
#include <app/display_message_state.h>
#include <esp_log.h>

using libesp::ErrorType;
using libesp::BaseMenu;
using libesp::RGBColor;
using libesp::XPT2046;
using libesp::Point2Ds;
using libesp::TouchNotification;

static StaticQueue_t InternalQueue;
static uint8_t InternalQueueBuffer[MenuState::QUEUE_SIZE*MenuState::MSG_SIZE] = {0};
static const char *LOGTAG = "MenuState";

MenuState::MenuState() :
	AppBaseMenu(), MenuList("Main Menu", Items, 0, 0, 
	MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), 0, ItemCount) {

	ESP_LOGI(LOGTAG,"menulist: %d %d",MyApp::get().getLastCanvasWidthPixel(),
						 MyApp::get().getLastCanvasHeightPixel());
	
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
}

MenuState::~MenuState() {

}


ErrorType MenuState::onInit() {
	Items[0].id = 0;
	//if (MyApp::get().getContacts().getSettings().isNameSet()) {
	//	Items[0].text = (const char *) "Settings";
	//} else {
		Items[0].text = (const char *) "Settings *";
	//}
	Items[1].id = 1;
	Items[1].text = (const char *) "Badge Pair";
	Items[2].id = 2;
	Items[2].text = (const char *) "3D";
	Items[3].id = 3;
	Items[3].text = (const char *) "Screen Saver";
	Items[4].id = 4;
	Items[4].text = (const char *) "Draw";
	Items[5].id = 5;
	Items[5].text = (const char *) "Badge Info";
	Items[6].id = 6;
	Items[6].text = (const char *) "Communications Settings";
	//Items[7].id = 7;
	//Items[7].text = (const char *) "Scan for NPCs ";
	Items[7].id = 7;
	Items[7].text = (const char *) "Test Badge";
	Items[8].id = 8;
	Items[8].text = (const char *) "Calibrate Touch";
	Items[9].id = 9;
	Items[9].text = (const char *) "Connected Devices";
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	MyApp::get().getGUI().drawList(&this->MenuList);
	//empty queue
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(InternalQueueHandler);

	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext MenuState::onRun() {
	BaseMenu *nextState = this;
	TouchNotification *pe = nullptr;
	bool penUp = false;
	bool hdrHit = false;
	pe = processTouch(InternalQueueHandler, MenuList, ItemCount, penUp, hdrHit);
	if (pe) {
		if (penUp ) {
			switch (MenuList.getSelectedItemID()) {
				case 0:
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					break;
				case 5:
					break;
				case 6:
					break;
				case 7:
					break;
				case 8:
					nextState = MyApp::get().getCalibrationMenu();
					break;
				case 9:
					break;
			}
		}
	}

	MyApp::get().getGUI().drawList(&this->MenuList);
	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType MenuState::onShutdown() {
	MyApp::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

