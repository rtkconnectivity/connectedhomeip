/* See Project CHIP LICENSE file for licensing information. */
#include <platform/logging/LogV.h>

#include <core/CHIPConfig.h>
#include <support/logging/Constants.h>

#include <cstdio>
#include <ctype.h>
#include <string.h>
#include "chip_porting.h"

#ifdef PW_RPC_ENABLED
#include "PigweedLogger.h"
#endif

#include <openthread/platform/logging.h>
#include <openthread/cli.h>

#define CONFIG_UART_DIRECT_PRINT 0

namespace chip {
namespace DeviceLayer {

/**
 * Called whenever a log message is emitted by chip or LwIP.
 *
 * This function is intended be overridden by the application to, e.g.,
 * schedule output of queued log entries.
 */
void __attribute__((weak)) OnLogOutput(void) {}

} // namespace DeviceLayer
} // namespace chip


namespace chip {
namespace Logging {
namespace Platform {

void LogV(const char * module, uint8_t category, const char * msg, va_list v)
{
#if CONFIG_UART_DIRECT_PRINT
    char logbuf[269];//256+11+2（CHIP_CONFIG_LOG_MESSAGE_MAX_SIZE + taglen + newlinelen）
    int len;
    len = snprintf(logbuf, 269, "chip[%s] ", module);  
    len += vsnprintf(&logbuf[len], 269 - len, msg, v);
    len += snprintf(&logbuf[len], 269 - len, "%s", "\r\n");
    MatterPlatUartSend(logbuf, len);
#else
    char logbuf[116];
    int len;
    len = snprintf(logbuf, 116, "[%s] ", module);
    len += vsnprintf(&logbuf[len], 116-len, msg, v);
    DBG_BUFFER_INTERNAL(LOG_TYPE, SUBTYPE_FORMAT, MODULE_MATTER, LEVEL_INFO, "%s", 1, TRACE_STRING(logbuf));
#endif

}

} // namespace Platform
} // namespace Logging
} // namespace chip
