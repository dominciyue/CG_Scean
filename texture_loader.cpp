#include "texture_loader.h"

#include <glad/glad.h>
#include <iostream>

// Note: stb_image implementation is defined only in main.cpp
#include "stb_image.h"

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
            
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
    
    return textureID;
}

unsigned int loadTextureFromDir(const std::string& filename, const std::string& directory) {
    std::string fullPath = directory;
    if (!directory.empty() && directory.back() != '/' && directory.back() != '\\') {
        fullPath += "/";
    }
    fullPath += filename;
    
    return loadTexture(fullPath.c_str());
}
