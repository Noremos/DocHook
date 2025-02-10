#include "DrawUtils.h"

#include "glew.h"

TextureId::~TextureId()
{
	glDeleteTextures(1, &textureId);
}

void GuiImage::makeTexture(unsigned char* image_data, int comp, bool smooth)
{
	// Create a OpenGL texture identifier
	GLuint newtext;
	glGenTextures(1, &newtext);
	glBindTexture(GL_TEXTURE_2D, newtext);
	textureId = std::make_shared<TextureId>(newtext);

	if (smooth)
	{
		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	std::vector<unsigned char> rgba_image_data(width * height * 4);

	if (comp == 1)
	{
		size_t length = width * height;
		rgba_image_data.resize(length * 4);
		for (size_t i = 0; i < length; i++){
			const auto& val = image_data[i * 3];
			rgba_image_data[i * 4 + 0] = val;
			rgba_image_data[i * 4 + 1] = val;
			rgba_image_data[i * 4 + 2] = val;
			rgba_image_data[i * 4 + 3] = 255;
		}

		image_data = rgba_image_data.data();
	}
	else if (comp == 3)
	{
		size_t length = width * height;
		rgba_image_data.resize(length * 4);
		for (size_t i = 0; i < length; i++) {
			rgba_image_data[i * 4 + 0] = image_data[i * 3 + 0];
			rgba_image_data[i * 4 + 1] = image_data[i * 3 + 1];
			rgba_image_data[i * 4 + 2] = image_data[i * 3 + 2];
			rgba_image_data[i * 4 + 3] = 255;
		}

		image_data = rgba_image_data.data();
	}

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
}
