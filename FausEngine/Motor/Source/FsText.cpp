#include"../Headers/FsText.h"
#include"../Headers/FsGame.h"

#include<GL/glew.h>

#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp> 

#include <ft2build.h>
#include <sstream>
#include FT_FREETYPE_H  

using namespace FausEngine;
unsigned int VAO, VBO;
float scale;

FsText::FsText()
{
	//shader= &FsGame::GetInstance()->GetShader(1);
	shader = FsGame::GetReference()->GetShader(1);
	text = "FausEngine";
	color = {1,1,1};
	position = {0,0,0};
	//logger.CreateLogger("FsText","log-FsText");
}

void FsText::Load(std::string path, int size, std::string text, FsVector3 pos, FsVector3 color) {
	//logger.CreateLogger("FsText", "log-FsText");
	const void* address = static_cast<const void*>(this);
	std::stringstream ss;
	ss << address;

	//shader = &FsGame::GetInstance()->GetShader(1);
	shader = FsGame::GetReference()->GetShader(1);
	this->text = text;
	this->color = color;
	scale = 1.0;
	this->position = pos;

	//inicializar freetype
	FT_Library ft;

	// Corroboro que freetype este iniciado
	if (FT_Init_FreeType(&ft)) {
		//logger.SetName("Text: " + ss.str());
		//logger.SetMessage("FsText not initialise", 1);
		FausEngine::FsGame::GetReference()->SetLog("FsText not initialise: "+text, 1);
	}
		

	// Cargo la fuente/font
	FT_Face face;
	if (FT_New_Face(ft, path.c_str(), 0, &face)) {
		//logger.SetName("Text: " + ss.str());
		//logger.SetMessage("Failed to load font: " +path, 1);
		FausEngine::FsGame::GetReference()->SetLog("Failed to load font: " + path, 2);
	}
		

	// se establece el tamano en pixels
	FT_Set_Pixel_Sizes(face, 0, size);

	// Deshabilitar byte-alignment ya que la fuente sale desordenada si no se lo hace
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//==============Creo cada letra siguiendo la estructura Character==============
	//Cargo 128 caracteres en codigo ASCII desde la fuente
	for (unsigned char i = 0; i < 128; i++) {

		// cargo el glifo
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			//std::cout << "Failed to load glyph: "<< face << std::endl;
			//logger.SetName("Text: " + ss.str());
			//logger.SetMessage("Failed to load glyph. ", 1);
			FausEngine::FsGame::GetReference()->SetLog("Failed to load glyph. ", 2);
			continue;
		}

		// Generar textura
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Character character = {
			texture,
			FsVector3(face->glyph->bitmap.width, face->glyph->bitmap.rows,0),
			FsVector3(face->glyph->bitmap_left, face->glyph->bitmap_top,0),
			static_cast<unsigned int>(face->glyph->advance.x)
		};


		Characters.insert(std::pair<GLchar, Character>(i, character));
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	// Finalizo Freetype (Se destruye)
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// COnfiguro los buffers para los cuadrantes de textura
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//logger.SetName("Text: " + ss.str());
	//logger.SetMessage("Loaded Text. "+path, 0);
	FausEngine::FsGame::GetReference()->SetLog("Loaded text: " + text, 0);
}



void FsText::Render() {
	FsVector3 textPos = this->position;

	//enable blending
	// Colorfinal = ColorSource * AlphaSource + ColorDestination * 1- AlphaSource
	//Colorfinal = (1.0f, 0.0f, 0.0f, 1.0f) * 1.0 + (1.0f, 1.0f, 0.0f, 1.0f) * (1.0f- 1.0f)	= (1.0f, 0.0f, 0.0f, 1.0f)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Activo Shader con el color y activo la textura
	shader->Use();
	glUniform3f(shader->GetUVariableLocation(uTypeVariables::uTextColor), this->color.x, this->color.y, this->color.z);
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(VAO);

	// iteracion de caracteres
	std::string::const_iterator c;

	for (c = text.begin(); c != text.end(); c++) {

		Character ch = Characters[*c];

		float xpos = textPos.x + ch.offset.x * scale;
		float ypos = textPos.y - (ch.size.y - ch.offset.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;

		float vertices[6][4] = {
			{ xpos, ypos + h, 0.0, 0.0 },
			{ xpos, ypos, 0.0, 1.0 },
			{ xpos + w, ypos, 1.0, 1.0 },

			{ xpos, ypos + h, 0.0, 0.0 },
			{ xpos + w, ypos, 1.0, 1.0 },
			{ xpos + w, ypos + h, 1.0, 0.0 }
		};

		// renderizar el glifo sobre el cuadrante
		glBindTexture(GL_TEXTURE_2D, ch.texture);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		// Usar glBufferSubData en vez de glBufferData
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Renderizo el cuadrante
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Avanze el cursor para la siguiente letra o glifo, el avance es de 1/64 pixeles
		// Cambio de bit �pr 6 para obtener lo pixeles (2^6 = 64, entonces se divide 1/6)
		textPos.x += (ch.advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_BLEND);
}

void FsText::SetPosition(FsVector3 _position) {
	this->position = _position;
}

void FsText::SetText(std::string _text) {
	this->text = _text;
}

void FsText::SetColor(FsVector3 c) {
	this->color = c;
}

FsVector3 FsText::GetColor() {
	return color;
}

FsVector3 FsText::GetPosition() {
	return position;
}

FsText::~FsText()
{
	//auto i = Characters.begin();
	//while (i != Characters.end()) {
	//	glDeleteTextures(1, &i->second.texture);
	//	++i;
	//}
}