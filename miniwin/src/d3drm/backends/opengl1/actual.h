#pragma once

#include "structs.h"

#include <SDL/SDL.h>
#include <stdint.h>
#include <vector>

// We don't want to transitively include windows.h, but we need GLuint
typedef unsigned int GLuint;
struct IDirect3DRMTexture;
struct MeshGroup;

typedef float Matrix4x4[4][4];

struct GL11_BridgeVector {
	float x, y, z;
};

struct GL11_BridgeTexCoord {
	float u, v;
};

struct GL11_BridgeSceneLight {
	FColor color;
	GL11_BridgeVector position;
	float positional;
	GL11_BridgeVector direction;
	float directional;
};

struct GL11_BridgeSceneVertex {
	GL11_BridgeVector position;
	GL11_BridgeVector normal;
	float tu, tv;
};

struct GLTextureCacheEntry {
	IDirect3DRMTexture* texture;
	Uint32 version;
	GLuint glTextureId;
	float width;
	float height;
	float uScale;
	float vScale;
};

struct GLMeshCacheEntry {
	const MeshGroup* meshGroup = nullptr;
	int version = 0;
	bool flat = false;
	bool isStrip = false;

	// non-VBO cache: for flat meshes, flatVertices stores deduplicated surface vertices;
	// for non-flat meshes, flatVertices is empty and vertexData points directly to meshGroup->vertices!
	std::vector<GL11_BridgeSceneVertex> flatVertices;
	const GL11_BridgeSceneVertex* vertexData = nullptr;
	size_t vertexCount = 0;
	std::vector<uint16_t> indices;

	// VBO cache
	GLuint vboPositions = 0;
	GLuint vboNormals = 0;
	GLuint vboTexcoords = 0;
	GLuint ibo = 0;
};

void GL11_InitState();
void GL11_LoadExtensions();
void GL11_DestroyTexture(GLuint texId);
int GL11_GetMaxTextureSize();
GLuint GL11_UploadTextureData(void* pixels, int width, int height, bool isUI, float scaleX, float scaleY, GLuint existingTexId = 0);
void GL11_UpdateTextureRow(GLuint texId, int xoffset, int yoffset, int width, void* pixels);
void GL11_DebugFlashTexture(GLuint texId, int width, int height);
void GL11_UploadMesh(GLMeshCacheEntry& cache, bool hasTexture);
void GL11_DestroyMesh(GLMeshCacheEntry& cache);
void GL11_BeginFrame(const Matrix4x4* projection);
void GL11_UploadLight(int lightIdx, GL11_BridgeSceneLight* l);
void GL11_EnableTransparency();
void GL11_SubmitDraw(
	GLMeshCacheEntry& mesh,
	const Matrix4x4& modelViewMatrix,
	const Appearance& appearance,
	GLuint texId
);
void GL11_Resize(int width, int height);
void GL11_Clear(float r, float g, float b);
void GL11_Draw2DImage(
	const GLTextureCacheEntry* cache,
	const SDL_Rect& srcRect,
	const SDL_Rect& dstRect,
	const FColor& color,
	float left,
	float right,
	float bottom,
	float top,
	bool blend);
void GL11_SetDither(bool dither);
void GL11_Download(SDL_Surface* target);




