#include "Arduino.h"
#include "Tools.h"
#include "DriverPSShiftReg.h"
#include "DriverMatrix.h"

// Definicion de Metodos ======================================================

static void unmappedKeyAction();
static void setDefaultButtonActions();

// ============================================================================

enum buttonState {
   released,
   pressed
};

static int loadPin;              // LD
static int clockPin;             // CLK
static int dataPin;              // Qh (serial out)
static char port;
static const int regCount = 2;   // numero de reg. de desplazamiento que usa

// [!] buttonsCount: CONTEMPLAR QUE PASA CUANDO NO ES MULTIPLO DE 8
static const int matrixButtons  = 8; // numero de botones dedicados a la matriz
static const int controlButtons = 8; // numero de botones dedicados a control
static const int buttonsCount = matrixButtons + controlButtons;   // total de botones
static void (*callbacks[2][buttonsCount])(int n);     //[0][...]: tecla presionada | [1][...]: tecla levantada

static long latestState = 0;
static long lastValidState = 0;

static const int debounceTime = 50; 
static bool waitingDebounce = false;
static long changeDetected;
static long bitsChanged;

// ============================================================================

void buttonsInit(char _port, int _loadPin, int _clockPin, int _dataPin) {

   loadPin = _loadPin;
   clockPin = _clockPin;
   dataPin = _dataPin;
   port = _port;

   psShiftRegInit(port, loadPin, clockPin, dataPin);  
   setDefaultButtonActions();
}

void buttonsScan() {
   // leo y guardo estado actual de los botones
   latestState = readShiftReg(port, loadPin, clockPin, dataPin, regCount);
   // detecto cambios y aplico debounce
   if (latestState != lastValidState) {
      if (waitingDebounce) {
         if (millis() - changeDetected > debounceTime) {
            // nuevo estado valido detectado
            bitsChanged = lastValidState ^ latestState;
            for (int i = 0; i < buttonsCount; i++) {
               // miro solo los bits que cambiaron
               if (bitsChanged & (1 << i)) {
                  if (latestState & (1 << i)) {
                     // si lo ultimo leido es un 1: se presiono
                     callbacks[pressed][i](i);
                  } else {
                     // si lo ultimo leido es un 0: se solto
                     callbacks[released][i](i);
                  }
               }
            }
            lastValidState = latestState;
            // p/debugging
            // printByte(lastValidState);
         }
      } else {
         // guardo instante en el que note el primer cambio
         changeDetected = millis();
         waitingDebounce = true;
      }
   } else {
      waitingDebounce = false;
   }
}

// retorna el estado actual de los botones de la matriz
long getMatrixButtonsState() {
   // enmascaro estado de todos los botones para quedarme solo con los de la matriz
   return lastValidState & ((1 << matrixButtons) - 1);
}

// configura la funcion a ejecutar al presionar un boton
void buttonPressCallback(void (*handler)(int n), int button) {
   callbacks[pressed][button] = handler;
}

// configura la funcion a ejecutar al presionar un boton
void buttonReleaseCallback(void (*handler)(int n), int button) {
   callbacks[released][button] = handler;
}

// ============================================================================

// accion por default de botones sin configuracion
static void unmappedButtonAction() {
   // p/debugging
   // Serial.println("[!] Action not asigned");
   // Serial.flush();
}

// asigno accion por default a cada boton
static void setDefaultButtonActions() {
   for (int i = 0; i < 2; i ++) {
      for (int j = 0; j < buttonsCount; j++) {
         callbacks[i][j] = unmappedButtonAction;
      }
   }
}


