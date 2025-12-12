#pragma once
#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <string>

// Load texture file, returns OpenGL texture ID
// Returns 0 on failure
unsigned int loadTexture(const char* path);

// Load texture file (with directory parameter)
unsigned int loadTextureFromDir(const std::string& filename, const std::string& directory);

#endif // TEXTURE_LOADER_H
