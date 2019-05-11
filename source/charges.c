/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "TimersManager.h"
#include "FunctionLib.h"
#include "MemManager.h"
#include "Panic.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "pin_mux.h"

#if (cPWR_UsePowerDownMode)
#include "PWR_Interface.h"
#include "PWR_Configuration.h"
#endif

/* BLE Host Stack */
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "gatt_db_handles.h"

/* Profile / Services */
#include "battery_interface.h"
#include "device_info_interface.h"
#include "heart_rate_interface.h"

/* Connection Manager */
#include "ble_conn_manager.h"

#include "board.h"
#include "ApplMain.h"
#include "charges.h"


/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
typedef enum
{
#if gAppUseBonding_d
    fastWhiteListAdvState_c,
#endif
    fastAdvState_c,
    slowAdvState_c
}advType_t;

typedef struct advState_tag{
    bool_t      advOn;
    advType_t   advType;
}advState_t;

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/

/* Adv State */
static advState_t  mAdvState;
static bool_t      mRestartAdv;
static uint32_t    mAdvTimeout;
static deviceId_t  mPeerDeviceId = gInvalidDeviceId_c;

static uint16_t chargesHandles[1] = { charges_enable_value };

/* Application specific data*/
static tmrTimerID_t mAdvTimerId;
static tmrTimerID_t mMeasurementTimerId;
static tmrTimerID_t mBatteryMeasurementTimerId;

/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/

/* Gatt and Att callbacks */
static void BleApp_AdvertisingCallback (gapAdvertisingEvent_t* pAdvertisingEvent);
static void BleApp_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent);
static void BleApp_GattServerCallback (deviceId_t deviceId, gattServerEvent_t* pServerEvent);
static void BleApp_Config();

/* Timer Callbacks */
static void AdvertisingTimerCallback (void *);
//static void TimerMeasurementCallback (void *);
//static void BatteryMeasurementTimerCallback (void *);

static void BleApp_Advertise(void);

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief    Initializes application specific functionality before the BLE stack init.
*
********************************************************************************** */
/*
void BleApp_Init(void)
{
    // Initialize application support for drivers
    BOARD_InitAdc();
}*/

/*! *********************************************************************************
* \brief    Starts the BLE application.
*
********************************************************************************** */
void BleApp_Start(void)
{
    /* Device is not connected and not advertising*/
    if (!mAdvState.advOn)
    {
#if gAppUseBonding_d
        if (gcBondedDevices > 0)
        {
            mAdvState.advType = fastWhiteListAdvState_c;
        }
        else
        {
#endif
            mAdvState.advType = fastAdvState_c;
#if gAppUseBonding_d
        }
#endif
        BleApp_Advertise();
    }
    
#if (cPWR_UsePowerDownMode)    
    PWR_ChangeDeepSleepMode(1); /* MCU=LLS3, LL=DSM, wakeup on GPIO/LL */
    PWR_AllowDeviceToSleep();
#endif       
}


/*! *********************************************************************************
* \brief        Handles BLE generic callback.
*
* \param[in]    pGenericEvent    Pointer to gapGenericEvent_t.
********************************************************************************** */
void BleApp_GenericCallback (gapGenericEvent_t* pGenericEvent)
{
    /* Call BLE Conn Manager */
    BleConnManager_GenericEvent(pGenericEvent);
    
    switch (pGenericEvent->eventType)
    {
        case gInitializationComplete_c:    
        {
            BleApp_Config();
        }
        break;    
        
        case gAdvertisingParametersSetupComplete_c:
        {
            App_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback);
        }
        break;         

        default: 
            break;
    }
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief        Configures BLE Stack after initialization. Usually used for
*               configuring advertising, scanning, white list, services, et al.
*
********************************************************************************** */
static void BleApp_Config()
{
    /* Configure as GAP peripheral */
    BleConnManager_GapPeripheralConfig();

    /* Register for callbacks*/
    //GattServer_RegisterHandlesForWriteNotifications(NumberOfElements(cpHandles), cpHandles);
    App_RegisterGattServerCallback(BleApp_GattServerCallback);

    GattServer_RegisterHandlesForWriteNotifications(NumberOfElements(chargesHandles), chargesHandles);



    mAdvState.advOn = FALSE;

    /* Start services */
    
    /* Allocate application timers */
    /*
    mAdvTimerId = TMR_AllocateTimer();
    mMeasurementTimerId = TMR_AllocateTimer();
    mBatteryMeasurementTimerId = TMR_AllocateTimer();
    */

#if (cPWR_UsePowerDownMode)    
    PWR_ChangeDeepSleepMode(3); /* MCU=LLS3, LL=IDLE, wakeup on GPIO/LL */
    PWR_AllowDeviceToSleep();    
#endif    
}

/*! *********************************************************************************
* \brief        Configures GAP Advertise parameters. Advertise will satrt after
*               the parameters are set.
*
********************************************************************************** */
static void BleApp_Advertise(void)
{
    switch (mAdvState.advType)
    {
#if gAppUseBonding_d
        case fastWhiteListAdvState_c:
        {
            gAdvParams.minInterval = gFastConnMinAdvInterval_c;
            gAdvParams.maxInterval = gFastConnMaxAdvInterval_c;
            gAdvParams.filterPolicy = gProcessWhiteListOnly_c;
            mAdvTimeout = gFastConnWhiteListAdvTime_c;
        }
        break;
#endif
        case fastAdvState_c:
        {
            gAdvParams.minInterval = gFastConnMinAdvInterval_c;
            gAdvParams.maxInterval = gFastConnMaxAdvInterval_c;
            gAdvParams.filterPolicy = gProcessAll_c;
            mAdvTimeout = gFastConnAdvTime_c - gFastConnWhiteListAdvTime_c;
        }
        break;

        case slowAdvState_c:
        {
            gAdvParams.minInterval = gReducedPowerMinAdvInterval_c;
            gAdvParams.maxInterval = gReducedPowerMinAdvInterval_c;
            gAdvParams.filterPolicy = gProcessAll_c;
            mAdvTimeout = gReducedPowerAdvTime_c;
        }
        break;
    }

    /* Set advertising parameters*/
    Gap_SetAdvertisingParameters(&gAdvParams);
}

/*! *********************************************************************************
* \brief        Handles BLE Advertising callback from host stack.
*
* \param[in]    pAdvertisingEvent    Pointer to gapAdvertisingEvent_t.
********************************************************************************** */
static void BleApp_AdvertisingCallback (gapAdvertisingEvent_t* pAdvertisingEvent)
{
    switch (pAdvertisingEvent->eventType)
    {
        case gAdvertisingStateChanged_c:
        {
            mAdvState.advOn = !mAdvState.advOn;
            
            if (!mAdvState.advOn && mRestartAdv)
            {
                BleApp_Advertise();
                break;
            }                

#if (cPWR_UsePowerDownMode)
            if(!mAdvState.advOn)
            {
                Led1Off();
                PWR_ChangeDeepSleepMode(3);
                PWR_SetDeepSleepTimeInMs(cPWR_DeepSleepDurationMs);
                PWR_AllowDeviceToSleep();    
            }  
            else
            {
                PWR_ChangeDeepSleepMode(1);
                /* Start advertising timer */
                TMR_StartLowPowerTimer(mAdvTimerId,gTmrLowPowerSecondTimer_c,
                         TmrSeconds(mAdvTimeout), AdvertisingTimerCallback, NULL);             
                Led1On();
            }
#else


            if(!mAdvState.advOn)
            {
            }
            else
            {
                TMR_StartLowPowerTimer(mAdvTimerId,gTmrLowPowerSecondTimer_c,
                        TmrSeconds(mAdvTimeout), AdvertisingTimerCallback, NULL);  
            }
#endif 
        }
        break;

        case gAdvertisingCommandFailed_c:
        {
            panic(0,0,0,0);
        }
        break;

        default:
            break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Connection callback from host stack.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    pConnectionEvent    Pointer to gapConnectionEvent_t.
********************************************************************************** */
static void BleApp_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
	/* Connection Manager to handle Host Stack interactions */
	BleConnManager_GapPeripheralEvent(peerDeviceId, pConnectionEvent);

    switch (pConnectionEvent->eventType)
    {
        case gConnEvtConnected_c:
        {
            mPeerDeviceId = peerDeviceId;

            /* Advertising stops when connected */
            mAdvState.advOn = FALSE;            
        
            /* Subscribe client*/
            Bas_Subscribe(peerDeviceId);        
            Hrs_Subscribe(peerDeviceId);
                                    
#if (!cPWR_UsePowerDownMode)  
            /* UI */            
            //Led1On();
#endif            
            
            /* Stop Advertising Timer*/
            mAdvState.advOn = FALSE;
            TMR_StopTimer(mAdvTimerId);
            
            /* Start measurements */
            /*
            TMR_StartLowPowerTimer(mMeasurementTimerId, gTmrLowPowerIntervalMillisTimer_c,
                       TmrSeconds(mHeartRateReportInterval_c), TimerMeasurementCallback, NULL);

            /* Start battery measurements */
            /*
            TMR_StartLowPowerTimer(mBatteryMeasurementTimerId, gTmrLowPowerIntervalMillisTimer_c,
                       TmrSeconds(mBatteryLevelReportInterval_c), BatteryMeasurementTimerCallback, NULL);            
            */
#if (cPWR_UsePowerDownMode)
    PWR_SetDeepSleepTimeInMs(900);
    PWR_ChangeDeepSleepMode(1);
    PWR_AllowDeviceToSleep();    
#endif
        }
        break;
        
        case gConnEvtDisconnected_c:
        {
            /* Unsubscribe client */
            Bas_Unsubscribe();
            Hrs_Unsubscribe();

            mPeerDeviceId = gInvalidDeviceId_c;
            
            TMR_StopTimer(mMeasurementTimerId);
            TMR_StopTimer(mBatteryMeasurementTimerId);            

#if (cPWR_UsePowerDownMode)            
            /* UI */
            //Led1Off();
            
            /* Go to sleep */
            PWR_ChangeDeepSleepMode(3); /* MCU=LLS3, LL=IDLE, wakeup on swithes/LL */
            PWR_SetDeepSleepTimeInMs(cPWR_DeepSleepDurationMs);
            PWR_AllowDeviceToSleep();
#else
            if (pConnectionEvent->eventData.disconnectedEvent.reason == gHciConnectionTimeout_c)
            {
                /* Link loss detected*/
                BleApp_Start();
            }
            else
            {
              /* Connection was terminated by peer or application */
                BleApp_Start();
            }
#endif			
        }
        break;
    default:
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles GATT server callback from host stack.
*
* \param[in]    deviceId        Peer device ID.
* \param[in]    pServerEvent    Pointer to gattServerEvent_t.
********************************************************************************** */
static void BleApp_GattServerCallback (deviceId_t deviceId, gattServerEvent_t* pServerEvent)
{
    uint16_t handle;
    uint8_t status;

    static uint8_t vals[] = {0};

    switch (pServerEvent->eventType)
    {
        case gEvtAttributeWritten_c:
        {
            handle = pServerEvent->eventData.attributeWrittenEvent.handle;
            status = gAttErrCodeNoError_c;

        }
        break;
        case gEvtAttributeWrittenWithoutResponse_c:
        {
        	handle = pServerEvent->eventData.attributeWrittenEvent.handle;
			status = gAttErrCodeNoError_c;
			if(handle == charges_enable_value){
				if(pServerEvent->eventData.attributeWrittenEvent.aValue[0] == 0xAA)
				{
					chargesEnable();
					vals[0] = 0x01;
				}
				else
				{
					chargesDisable();
					vals[0] = 0x00;
				}
				status = GattDb_WriteAttribute(charges_state_value, 1, vals);
			}

        }
    default:
        break;
    }
}


/*! *********************************************************************************
* \brief        Handles advertising timer callback.
*
* \param[in]    pParam        Calback parameters.
********************************************************************************** */
static void AdvertisingTimerCallback(void * pParam)
{
    /* Stop and restart advertising with new parameters */
    Gap_StopAdvertising();

    switch (mAdvState.advType)
    {
#if gAppUseBonding_d
        case fastWhiteListAdvState_c:
        {
            mAdvState.advType = fastAdvState_c;
            mRestartAdv = TRUE;
        }
        break;
#endif
        case fastAdvState_c:
        {
            mAdvState.advType = slowAdvState_c;
            mRestartAdv = TRUE;
        }
        break;

        default:
        {
            //mRestartAdv = FALSE;
        }
        break;
    }
    BleApp_Advertise();
    Gap_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback);
}

/*! *********************************************************************************
* @}
********************************************************************************** */

void chargesInit(){

	//init droge_mcu main_mcu and pwen output
	gpio_pin_config_t conf =
	{
			.pinDirection = kGPIO_DigitalOutput,
			.outputLogic = 0
	};

  CLOCK_EnableClock(kCLOCK_PortA);
  PORT_SetPinMux(PORTA, 18, kPORT_MuxAsGpio);
  PORT_SetPinMux(PORTA, 19, kPORT_MuxAsGpio);
  PORT_SetPinMux(PORTA, 17, kPORT_MuxAsGpio);

	// droge mcu
	GPIO_PinInit(GPIOA, 17, &conf);
	//main Mcu
	GPIO_PinInit(GPIOA, 18, &conf);

	//pw en
	conf.outputLogic = 1;
	GPIO_PinInit(GPIOA, 19, &conf);
}

void chargesEnable(){


  //droge mcu
	GPIO_WritePinOutput(GPIOA, 17, 1);
	//main mcu
	GPIO_WritePinOutput(GPIOA, 18, 1);
	//pwEn
	GPIO_WritePinOutput(GPIOA, 19, 0);
}

void chargesDisable(){
	//droge mcu
	GPIO_WritePinOutput(GPIOA, 17, 0);
	//main mcu
	GPIO_WritePinOutput(GPIOA, 18, 0);
	//pwEn
	GPIO_WritePinOutput(GPIOA, 19, 1);
}
