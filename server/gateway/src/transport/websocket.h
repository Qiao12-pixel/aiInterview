#pragma once
#include "transport/http.h"
namespace gateway {
void HandleWebSocket(App &app, Stream stream, Request req);
}
