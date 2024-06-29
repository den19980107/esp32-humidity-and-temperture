#include "json.h"

const char* jsonToByte(JsonDocument doc) {
	size_t payloadSize = measureJson(doc) + 1;
	char* payload = new char[payloadSize];
	serializeJson(doc, payload, payloadSize);
	return payload;
}
