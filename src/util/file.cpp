#include "file.h"

std::string readFile(fs::FS& fs, const char* path) {
	Serial.printf("Reading file: %s\n", path);

	File file = fs.open(path, "r");
	if (!file || file.isDirectory()) {
		Serial.println("- failed to open file for reading");
		return "";
	}

	std::string fileContent;
	while (file.available()) {
		fileContent += file.read();
	}
	file.close();
	return fileContent;
}