/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "io/frsky_pxx2.h"
#include "pulses/pxx2.h"
#include "mixer_scheduler.h"

uint8_t s_pulses_paused = 0;
ModuleState moduleState[NUM_MODULES];
InternalModulePulsesData intmodulePulsesData __DMA;
ExternalModulePulsesData extmodulePulsesData __DMA;
TrainerPulsesData trainerPulsesData __DMA;

void ModuleState::startBind(BindInformation * destination, ModuleCallback bindCallback)
{
  bindInformation = destination;
  callback = bindCallback;
  mode = MODULE_MODE_BIND;
#if defined(SIMU)
  bindInformation->candidateReceiversCount = 2;
  strcpy(bindInformation->candidateReceiversNames[0], "SimuRX1");
  strcpy(bindInformation->candidateReceiversNames[1], "SimuRX2");
#endif
}

uint8_t getModuleType(uint8_t module)
{
  uint8_t type = g_model.moduleData[module].type;

#if defined(HARDWARE_INTERNAL_MODULE)
  if (module == INTERNAL_MODULE && isInternalModuleAvailable(type)) {
    return type;
  }
#endif

  if (module == EXTERNAL_MODULE && isExternalModuleAvailable(type)) {
    return type;
  }

  return MODULE_TYPE_NONE;
}

uint8_t getRequiredProtocol(uint8_t module)
{
  uint8_t protocol;

  switch (getModuleType(module)) {
    case MODULE_TYPE_PPM:
      protocol = PROTOCOL_CHANNELS_PPM;
      break;

    case MODULE_TYPE_XJT_PXX1:
#if defined(INTMODULE_USART)
      if (module == INTERNAL_MODULE) {
        protocol = PROTOCOL_CHANNELS_PXX1_SERIAL;
        break;
      }
#endif
      protocol = PROTOCOL_CHANNELS_PXX1_PULSES;
      break;

    case MODULE_TYPE_R9M_PXX1:
      protocol = PROTOCOL_CHANNELS_PXX1_PULSES;
      break;

#if defined(HARDWARE_EXTERNAL_MODULE_SIZE_SML)
    case MODULE_TYPE_R9M_LITE_PXX1:
    case MODULE_TYPE_R9M_LITE_PRO_PXX1:
      protocol = PROTOCOL_CHANNELS_PXX1_SERIAL;
      break;

    case MODULE_TYPE_R9M_LITE_PXX2:
      protocol = PROTOCOL_CHANNELS_PXX2_LOWSPEED;
      break;
#endif

    case MODULE_TYPE_ISRM_PXX2:
    case MODULE_TYPE_R9M_PXX2:
#if defined(HARDWARE_EXTERNAL_MODULE_SIZE_SML)
    case MODULE_TYPE_XJT_LITE_PXX2:
    case MODULE_TYPE_R9M_LITE_PRO_PXX2:
#endif
      protocol = PROTOCOL_CHANNELS_PXX2_HIGHSPEED;
      break;

    case MODULE_TYPE_SBUS:
      protocol = PROTOCOL_CHANNELS_SBUS;
      break;

#if defined(MULTIMODULE)
    case MODULE_TYPE_MULTIMODULE:
      protocol = PROTOCOL_CHANNELS_MULTIMODULE;
      break;
#endif

#if defined(DSM2)
    case MODULE_TYPE_DSM2:
      protocol = limit<uint8_t>(PROTOCOL_CHANNELS_DSM2_LP45, PROTOCOL_CHANNELS_DSM2_LP45+g_model.moduleData[module].rfProtocol, PROTOCOL_CHANNELS_DSM2_DSMX);
      // The module is set to OFF during one second before BIND start
      {
        static tmr10ms_t bindStartTime = 0;
        if (moduleState[module].mode == MODULE_MODE_BIND) {
          if (bindStartTime == 0) bindStartTime = get_tmr10ms();
          if ((tmr10ms_t)(get_tmr10ms() - bindStartTime) < 100) {
            protocol = PROTOCOL_CHANNELS_NONE;
            break;
          }
        }
        else {
          bindStartTime = 0;
        }
      }
      break;
#endif

#if defined(CROSSFIRE)
    case MODULE_TYPE_CROSSFIRE:
      protocol = PROTOCOL_CHANNELS_CROSSFIRE;
      break;
#endif

    default:
      protocol = PROTOCOL_CHANNELS_NONE;
      break;
  }

  if (s_pulses_paused) {
    protocol = PROTOCOL_CHANNELS_NONE;
  }

#if 0
  // will need an EEPROM conversion
  if (moduleState[module].mode == MODULE_OFF) {
    protocol = PROTOCOL_CHANNELS_NONE;
  }
#endif

  return protocol;
}

void enablePulsesExternalModule(uint8_t protocol)
{
  // start new protocol hardware here

  switch (protocol) {
#if defined(PXX1)
    case PROTOCOL_CHANNELS_PXX1_PULSES:
      extmodulePxx1PulsesStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, PXX_PULSES_PERIOD);
      break;
#endif

#if defined(PXX1) && defined(HARDWARE_EXTERNAL_MODULE_SIZE_SML)
    case PROTOCOL_CHANNELS_PXX1_SERIAL:
      extmodulePxx1SerialStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, EXTMODULE_PXX1_SERIAL_PERIOD);
      break;
#endif

#if defined(DSM2)
    case PROTOCOL_CHANNELS_DSM2_LP45:
    case PROTOCOL_CHANNELS_DSM2_DSM2:
    case PROTOCOL_CHANNELS_DSM2_DSMX:
      extmoduleSerialStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, DSM2_PERIOD);
      break;
#endif

#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
      EXTERNAL_MODULE_ON();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, CROSSFIRE_PERIOD);
      break;
#endif

#if defined(PXX2) && defined(EXTMODULE_USART)
    case PROTOCOL_CHANNELS_PXX2_HIGHSPEED:
      extmoduleInvertedSerialStart(PXX2_HIGHSPEED_BAUDRATE);
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, PXX2_PERIOD);
      break;

    case PROTOCOL_CHANNELS_PXX2_LOWSPEED:
      extmoduleInvertedSerialStart(PXX2_LOWSPEED_BAUDRATE);
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, PXX2_PERIOD);
      break;
#endif

#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
      extmoduleSerialStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, MULTIMODULE_PERIOD);
      break;
#endif

#if defined(SBUS)
    case PROTOCOL_CHANNELS_SBUS:
      extmoduleSerialStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, SBUS_PERIOD);
      break;
#endif

#if defined(PPM)
    case PROTOCOL_CHANNELS_PPM:
      extmodulePpmStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, PPM_PERIOD(EXTERNAL_MODULE));
      break;
#endif

    default:
      break;
  }
}

void setupPulsesExternalModule(uint8_t protocol)
{
  switch (protocol) {
#if defined(PXX1)
    case PROTOCOL_CHANNELS_PXX1_PULSES:
      extmodulePulsesData.pxx.setupFrame(EXTERNAL_MODULE);
      break;
#endif

#if defined(PXX1) && defined(HARDWARE_EXTERNAL_MODULE_SIZE_SML)
    case PROTOCOL_CHANNELS_PXX1_SERIAL:
      extmodulePulsesData.pxx_uart.setupFrame(EXTERNAL_MODULE);
      break;
#endif

#if defined(PXX2)
    case PROTOCOL_CHANNELS_PXX2_HIGHSPEED:
    case PROTOCOL_CHANNELS_PXX2_LOWSPEED:
      extmodulePulsesData.pxx2.setupFrame(EXTERNAL_MODULE);
      break;
#endif

#if defined(SBUS)
    case PROTOCOL_CHANNELS_SBUS:
      setupPulsesSbus();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, SBUS_PERIOD);
      break;
#endif

#if defined(DSM2)
    case PROTOCOL_CHANNELS_DSM2_LP45:
    case PROTOCOL_CHANNELS_DSM2_DSM2:
    case PROTOCOL_CHANNELS_DSM2_DSMX:
      setupPulsesDSM2();
      break;
#endif

#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
    {
      ModuleSyncStatus& status = getModuleSyncStatus(EXTERNAL_MODULE);
      if (status.isValid())
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, status.getAdjustedRefreshRate());
      setupPulsesCrossfire();
      break;
    }
#endif

#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
    {
      ModuleSyncStatus& status = getModuleSyncStatus(EXTERNAL_MODULE);
      if (status.isValid())
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, status.getAdjustedRefreshRate());
      else
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, MULTIMODULE_PERIOD);

      setupPulsesMultiExternalModule();
      break;
    }
#endif

#if defined(PPM)
    case PROTOCOL_CHANNELS_PPM:
      setupPulsesPPMExternalModule();
      break;
#endif

    default:
      // external module stopped, set period to 0
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, 0);
      break;
  }
}

#if defined(HARDWARE_INTERNAL_MODULE)
static void enablePulsesInternalModule(uint8_t protocol)
{
  // start new protocol hardware here

  switch (protocol) {
#if defined(PXX1) && !defined(INTMODULE_USART)
    case PROTOCOL_CHANNELS_PXX1_PULSES:
      intmodulePxx1PulsesStart();
#if defined(INTMODULE_HEARTBEAT)
      mixerSchedulerStop();
#else
      mixerSchedulerSetPeriod(INTERNAL_MODULE, INTMODULE_PXX1_SERIAL_PERIOD);
      mixerSchedulerStart();
#endif
      break;
#endif

#if defined(PXX1) && defined(INTMODULE_USART)
    case PROTOCOL_CHANNELS_PXX1_SERIAL:
      intmodulePxx1SerialStart();
#if defined(INTMODULE_HEARTBEAT)
      mixerSchedulerStop();
#else
      mixerSchedulerSetPeriod(INTERNAL_MODULE, INTMODULE_PXX1_SERIAL_PERIOD);
      mixerSchedulerStart();
#endif
      break;
#endif

#if defined(PXX2)
    case PROTOCOL_CHANNELS_PXX2_HIGHSPEED:
      intmoduleSerialStart(PXX2_HIGHSPEED_BAUDRATE, true, USART_Parity_No, USART_StopBits_1, USART_WordLength_8b);
#if defined(INTMODULE_HEARTBEAT)
      mixerSchedulerStop();
#else
      mixerSchedulerSetPeriod(INTERNAL_MODULE, PXX2_PERIOD);
      mixerSchedulerStart();
#endif
      break;
#endif

#if defined(INTERNAL_MODULE_MULTI)
    case PROTOCOL_CHANNELS_MULTIMODULE:
      intmodulePulsesData.multi.initFrame();
      intmoduleSerialStart(MULTIMODULE_BAUDRATE, true, USART_Parity_Even, USART_StopBits_2, USART_WordLength_9b);
      mixerSchedulerSetPeriod(INTERNAL_MODULE, MULTIMODULE_PERIOD);
      break;
#endif
    default:
      // internal module stopped, set internal period to 0 and start the scheduler
      mixerSchedulerSetPeriod(INTERNAL_MODULE, 0);
      mixerSchedulerStart();
      break;
  }
}

bool setupPulsesInternalModule(uint8_t protocol)
{
  switch (protocol) {
#if defined(HARDWARE_INTERNAL_MODULE) && defined(PXX1) && !defined(INTMODULE_USART)
    case PROTOCOL_CHANNELS_PXX1_PULSES:
      intmodulePulsesData.pxx.setupFrame(INTERNAL_MODULE);
      return true;
#endif

#if defined(PXX1) && defined(INTMODULE_USART)
    case PROTOCOL_CHANNELS_PXX1_SERIAL:
      intmodulePulsesData.pxx_uart.setupFrame(INTERNAL_MODULE);
      return true;
#endif

#if defined(PXX2)
    case PROTOCOL_CHANNELS_PXX2_HIGHSPEED:
    {
      bool result = intmodulePulsesData.pxx2.setupFrame(INTERNAL_MODULE);
      if (moduleState[INTERNAL_MODULE].mode == MODULE_MODE_SPECTRUM_ANALYSER || moduleState[INTERNAL_MODULE].mode == MODULE_MODE_POWER_METER) {
        mixerSchedulerSetPeriod(INTERNAL_MODULE, PXX2_TOOLS_PERIOD);
      }
      else {
#if !defined(INTMODULE_HEARTBEAT)
        mixerSchedulerSetPeriod(INTERNAL_MODULE, PXX2_PERIOD);
#else
        mixerSchedulerSetPeriod(INTERNAL_MODULE, 0);
#endif
      }      
      return result;
    }
#endif

#if defined(PCBTARANIS) && defined(INTERNAL_MODULE_PPM)
    case PROTOCOL_CHANNELS_PPM:
      setupPulsesPPMInternalModule();
      mixerSchedulerSetPeriod(INTERNAL_MODULE, PPM_PERIOD(INTERNAL_MODULE));
      return true;
#endif

#if defined(INTERNAL_MODULE_MULTI)
    case PROTOCOL_CHANNELS_MULTIMODULE:
      setupPulsesMultiInternalModule();
      mixerSchedulerSetPeriod(INTERNAL_MODULE, MULTIMODULE_PERIOD);
      return true;
#endif

    default:
      return true;
  }
}

bool setupPulsesInternalModule()
{
  uint8_t protocol = getRequiredProtocol(INTERNAL_MODULE);

  heartbeat |= (HEART_TIMER_PULSES << INTERNAL_MODULE);

  if (moduleState[INTERNAL_MODULE].protocol != protocol) {
    intmoduleStop();
    moduleState[INTERNAL_MODULE].protocol = protocol;
    enablePulsesInternalModule(protocol);
    return false;
  }
  else {
    return setupPulsesInternalModule(protocol);
  }
}
#endif

bool setupPulsesExternalModule()
{
  uint8_t protocol = getRequiredProtocol(EXTERNAL_MODULE);

  heartbeat |= (HEART_TIMER_PULSES << EXTERNAL_MODULE);

  if (moduleState[EXTERNAL_MODULE].protocol != protocol) {
    extmoduleStop();
    moduleState[EXTERNAL_MODULE].protocol = protocol;
    enablePulsesExternalModule(protocol);
    return false;
  }
  else {
    setupPulsesExternalModule(protocol);
    return true;
  }
}

void setCustomFailsafe(uint8_t moduleIndex)
{
  if (moduleIndex < NUM_MODULES) {
    for (int ch=0; ch<MAX_OUTPUT_CHANNELS; ch++) {
      if (ch < g_model.moduleData[moduleIndex].channelsStart || ch >= sentModuleChannels(moduleIndex) + g_model.moduleData[moduleIndex].channelsStart) {
        g_model.failsafeChannels[ch] = 0;
      }
      else if (g_model.failsafeChannels[ch] < FAILSAFE_CHANNEL_HOLD) {
        g_model.failsafeChannels[ch] = channelOutputs[ch];
      }
    }
  }
}
