#include <Arduino.h>
#include <EasyButton.h>
#include "pages.hpp"

int8_t page = 0;
EasyButton tp_button(TP_PIN_PIN, 80, true, false);
uint32_t time_out = millis();
uint16_t max_time_out = 15000;
bool handlingAction = false;
bool initialLoad = true;
bool subMenu = false;
int8_t pagesCount = 0;
bool showCommon = true;

typedef void(*Page)(bool);
Page pages[] = {
    pageCalendar,
    pageClock,
    pageRtc,
    pageBattery,
    pageBearing,
    pageTemperature,
    pageOta,
    handleSleep,
    NULL,
    NULL,
    NULL
};

typedef void(*Action)();
Action actions[] = {
    actionCalendar, // NULL,
    actionClock,
    actionCounter,
    NULL,
    actionBearing,
    NULL,
    waitOta,
    NULL,
    NULL
};

bool submenu(int8_t press);
struct Submenu {
    const char **names;
    Action *actions;
};

const char *menuOptions[] = {
    "Play",
    "Next",
    "Prev",
    "WSTECZ",
    NULL
};

int8_t menu = -1;

void sendPlay() { MQTTpublish("musiccmd", "play-pause"); }
void sendNext() { MQTTpublish("musiccmd", "next"); }
void sendPrev() { MQTTpublish("musiccmd", "previous"); }
void sendStop() { MQTTpublish("musiccmd", "stop"); }

Action menuActions[] = {
    sendPlay,
    sendNext,
    sendPrev,
    NULL,
    NULL,
};

Submenu mainMenu = { menuOptions, menuActions };

Submenu *submenus[] = {
    NULL,
    NULL,
    NULL,
    &mainMenu,
    NULL,
    // submenuTemperature,
    NULL,
    NULL,
    NULL,
    NULL
};

int timeOut[] = { 8, 15, 15, 60, 30, 15, 0 };

void commonLoop(void *param) {
    for (;;) {
        while (handlingAction) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        // drawCommon(page, pagesCount);
        showCommon = true;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void initButton() {
  pinMode(TP_PWR_PIN, PULLUP);
  digitalWrite(TP_PWR_PIN, HIGH);
  tp_button.begin();
  tp_button.onPressedFor(1000, handleAction);
  tp_button.onPressed(handlePress);
  page = 0;
  showPage();
  for (pagesCount = 0; (pages[pagesCount] || pages[pagesCount+1]) && pagesCount < 10; pagesCount++) {}
  Serial.printf("[RTOS] Task start: drawCommon.\n");
  xTaskCreate(commonLoop, "DrawCommonPart", 2048, NULL, 1, NULL);
}

void refreshTimer() {
    time_out = millis();
}

void handleUi() {
    if (!isCharging()) { digitalWrite(LED_PIN, digitalRead(TP_PIN_PIN)); }
    /*
    tp_button.read();
    if (getBusVoltage() > 4.0) {
        if (!handlingAction) { showPage(); }
        return;
    }
    */
    if (millis() - time_out > max_time_out && !handlingAction) {
        handleSleep(false);
    } else {
        tp_button.read();
        if (!handlingAction) { showPage(); }
    }
}

void handlePress() {
    time_out = millis();
    if (submenus[page] && submenu(1)) { return; }
    initialLoad = true;
    increasePage();
}

void increasePage() {
    page++;
    initialLoad = true;
    if (!pages[page] && !pages[page+1]) { page = 0; }
}

void showPage() {
    bool cleared = false;
    if (showCommon) {
        cleared = drawCommon(page, pagesCount);
        showCommon = false;
    }
    if (submenus[page] && submenu(0)) {
            if (cleared) { submenu(-2); }
            drawBottomBar(getTimeout(), TFT_BLUE);
            return;
    }
    if (pages[page]) {
        max_time_out = timeOut[page] * 1000;
        if (max_time_out < 1000) { max_time_out = 10000; }
        pages[page](cleared || initialLoad);
        initialLoad = false;
    }
}

void handleAction() {
    handlingAction = true;
    if (actions[page]) {
        actions[page]();
        initialLoad = true;
    } else if (submenus[page]) {
        submenu(2);
    }
    handlingAction = false;
    time_out = millis();
}

uint8_t getTimeout() {
    uint32_t t =  100 - ((millis() - time_out) * 100 / max_time_out);
    if (t > 100) { return 100; }
    return t;
}

void home() { page = 0; initialLoad = true; showPage(); }

#define DmenuOptions submenus[page]->names
#define DmenuActions submenus[page]->actions

bool submenu(int8_t press) {
    if (press == -2) {
        drawMenuPointer(menu, DmenuOptions[menu] != NULL);
        drawOptions(DmenuOptions);
        return menu >= 0;
    }
    if (press < 0) { menu = -1; initialLoad = true; return false; }
    if (press == 0 && menu < 0) { return false; }
    if (!DmenuOptions[0]) { return false; }
    if (press == 2 && menu < 0) {
        drawOptions(DmenuOptions);
        menu = 0;
        drawMenuPointer(menu, DmenuOptions[menu] != NULL);
        return true;
    }
    if (menu < 0) { return false; }
    if (press == 2 && DmenuActions[menu]) {
        DmenuActions[menu]();
        return true;
    }
    if (press == 2 && !DmenuActions[menu]) {
        clearScreen();
        menu = -1;
        initialLoad = true;
        return false;
    }
    if (press == 1) {
        menu++;
        if (DmenuOptions[menu]) {
            drawMenuPointer(menu, DmenuOptions[menu] != NULL);
        }
    }
    if (!DmenuOptions[menu]) {
        menu = 0;
        drawMenuPointer(menu, DmenuOptions[menu] != NULL);
    }
    // drawMenuPointer(menu, OPTIONS_TEMPERATURE);
    return true;
}
