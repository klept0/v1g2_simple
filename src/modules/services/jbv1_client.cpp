#include "jbv1_client.h"

JBV1Data g_jbv1;

bool JBV1Data::valid() const {
    return lastUpdateMs != 0 && (millis() - lastUpdateMs) < 10000;
}

void jbv1_tick(uint32_t /*nowMs*/) {
    // No-op for now; future: proactively clear on 10s timeout
}
