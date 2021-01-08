#include "timer_menu.h"
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
static uint8_t InternalQueueBuffer[TimerMenu::QUEUE_SIZE*TimerMenu::MSG_SIZE] = {0};
const char *TimerMenu::LOGTAG = "TimerMenu";

static libesp::AABBox2D UpArrow(Point2Ds(140,50), 20);
static libesp::Button UpArrowButton((const char *)"Up", uint16_t(0), &UpArrow,RGBColor::BLUE, RGBColor::RED);
static libesp::AABBox2D DownArrow(Point2Ds(140,95), 20);
static libesp::Button DownArrowButton((const char *)"Down", uint16_t(1), &DownArrow,RGBColor::BLUE, RGBColor::RED);
static libesp::AABBox2D Clock(Point2Ds(65,72), 50);
static libesp::Button ClockWidget((const char *)"Clock", uint16_t(2), &Clock,RGBColor::BLUE, RGBColor::RED);

static const int8_t NUM_INTERFACE_ITEMS = 3;
static libesp::Widget *InterfaceElements[NUM_INTERFACE_ITEMS] = {&ClockWidget,&UpArrowButton,&DownArrowButton};



TimerMenu::TimerMenu() :
	AppBaseMenu(), MyLayout(&InterfaceElements[0],NUM_INTERFACE_ITEMS, MyApp::get().getLastCanvasWidthPixel(), MyApp::get().getLastCanvasHeightPixel(), false) {
	
	InternalQueueHandler = xQueueCreateStatic(QUEUE_SIZE,MSG_SIZE,&InternalQueueBuffer[0],&InternalQueue);
	MyLayout.init();
}

TimerMenu::~TimerMenu() {

}


ErrorType TimerMenu::onInit() {
	MyApp::get().getDisplay().fillScreen(RGBColor::BLACK);
	//empty queue
	TouchNotification *pe = nullptr;
	for(int i=0;i<2;i++) {
		if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
			delete pe;
		}
	}
	MyApp::get().getTouch().addObserver(InternalQueueHandler);
	//MyLayout.draw(&MyApp::get().getDisplay());

	return ErrorType();
}

libesp::BaseMenu::ReturnStateContext TimerMenu::onRun() {
	BaseMenu *nextState = this;

	TouchNotification *pe = nullptr;
	Point2Ds TouchPosInBuf;
	libesp::Widget *widgetHit = nullptr;
	bool penUp = false;
	if(xQueueReceive(InternalQueueHandler, &pe, 0)) {
		ESP_LOGI(LOGTAG,"que");
		Point2Ds screenPoint(pe->getX(),pe->getY());
		TouchPosInBuf = MyApp::get().getCalibrationMenu()->getPickPoint(screenPoint);
		ESP_LOGI(LOGTAG,"TouchPoint: X:%d Y:%d PD:%d", int32_t(TouchPosInBuf.getX()),
								 int32_t(TouchPosInBuf.getY()), pe->isPenDown()?1:0);
		penUp = !pe->isPenDown();
		delete pe;
		widgetHit = MyLayout.pick(TouchPosInBuf);
	}

	//MyApp::get().getDisplay().drawRec(120,30,30,30, libesp::RGBColor::GREEN);
	MyLayout.draw(&MyApp::get().getDisplay());


	//MyApp::get().getDisplay().drawRec(100,80,20,20, libesp::RGBColor::GREEN);
	//MyApp::get().getDisplay().drawRec(5,47,50,50, libesp::RGBColor::GREEN);

	if(widgetHit) {
		ESP_LOGI(LOGTAG, "Widget %s hit\n", widgetHit->getName());
		//switch(widgetHit->getWidgetID()) {
		//case 0:
			//nextState = MyApp::get().getMenuState();
			//break;
		//}
	}

	return BaseMenu::ReturnStateContext(nextState);
}

ErrorType TimerMenu::onShutdown() {
	MyApp::get().getTouch().removeObserver(InternalQueueHandler);
	return ErrorType();
}

