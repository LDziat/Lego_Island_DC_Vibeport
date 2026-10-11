#include <math.h>
#include <stddef.h>
// This file cannot include any minwin headers.

#include "actual.h"

#include "structs.h"

#include <SDL/SDL_opengl.h>
#ifdef __DREAMCAST__
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include <algorithm>
#include <cstring>
#include <vector>

#include "compat.h"

// GL extension API functions.
bool g_useVBOs;
PFNGLGENBUFFERSPROC mwglGenBuffers;
PFNGLBINDBUFFERPROC mwglBindBuffer;
PFNGLBUFFERDATAPROC mwglBufferData;
PFNGLDELETEBUFFERSPROC mwglDeleteBuffers;

void GL11_InitState()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
}

void GL11_LoadExtensions()
{
#ifdef __DREAMCAST__
	g_useVBOs = false;
#else
	g_useVBOs = SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object");
#endif

	if (g_useVBOs) {
		// Load the required GL function pointers.
		mwglGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffersARB");
		mwglBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBufferARB");
		mwglBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferDataARB");
		mwglDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffersARB");
	}
}

void GL11_DestroyTexture(GLuint texId)
{
	glDeleteTextures(1, &texId);
}

int GL11_GetMaxTextureSize()
{
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	return maxTextureSize;
}



GLuint GL11_UploadTextureData(void* pixels, int width, int height, bool isUI, float scaleX, float scaleY, GLuint existingTexId)
{
	GLuint texId;
	if (existingTexId) {
		texId = existingTexId;
	} else {
		glGenTextures(1, &texId);
	}
	glBindTexture(GL_TEXTURE_2D, texId);
	if (isUI) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	if (g_debugDisableTextures && !isUI) {
		// Upload a 1x1 white dummy texture to save RAM and avoid crash
		Uint32 dummyPixel = 0xFFFFFFFF;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &dummyPixel);
	} else {
#ifdef __DREAMCAST__
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif
	}

	return texId;
}

void GL11_UpdateTextureRow(GLuint texId, int xoffset, int yoffset, int width, void* pixels)
{
	glBindTexture(GL_TEXTURE_2D, texId);
#ifdef __DREAMCAST__
	glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif
}

void GL11_DebugFlashTexture(GLuint texId, int width, int height)
{
#ifdef __DREAMCAST__
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId);

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS);
	glColor4f(1, 1, 1, 1);
	// We want to map the top-left of the texture (u=0, v=0) to (u=1, v=1) if it's padded.
	// But it's just a debug screen so drawing the whole texture is fine.
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 1.0f, -0.5f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 1.0f, -0.5f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, -1.0f, -0.5f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -0.5f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	extern void* DDWindow;
	glKosSwapBuffers();
#endif
}

void GL11_UploadMesh(GLMeshCacheEntry& cache, bool hasTexture)
{
	if (g_useVBOs && cache.vertexData && cache.vertexCount > 0 && !cache.indices.empty()) {
		mwglGenBuffers(1, &cache.vboPositions);
		mwglBindBuffer(GL_ARRAY_BUFFER_ARB, cache.vboPositions);
		mwglBufferData(
			GL_ARRAY_BUFFER_ARB,
			cache.vertexCount * sizeof(GL11_BridgeSceneVertex),
			cache.vertexData,
			GL_STATIC_DRAW_ARB
		);

		mwglGenBuffers(1, &cache.ibo);
		mwglBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, cache.ibo);
		mwglBufferData(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			cache.indices.size() * sizeof(cache.indices[0]),
			cache.indices.data(),
			GL_STATIC_DRAW_ARB
		);
	}
}

void GL11_DestroyMesh(GLMeshCacheEntry& cache)
{
	if (g_useVBOs) {
		mwglDeleteBuffers(1, &cache.vboPositions);
		mwglDeleteBuffers(1, &cache.vboNormals);
		mwglDeleteBuffers(1, &cache.vboTexcoords);
		mwglDeleteBuffers(1, &cache.ibo);
	}
	cache.flatVertices.clear();
	cache.flatVertices.shrink_to_fit();
	cache.indices.clear();
	cache.indices.shrink_to_fit();
	cache.vertexData = nullptr;
	cache.vertexCount = 0;
	cache.meshGroup = nullptr;
}

static GLenum s_currentShadeModel = 0;
static GLuint s_currentBoundTexId = 0xFFFFFFFF;
static bool s_currentTex2DEnabled = false;

void GL11_BeginFrame(const Matrix4x4* projection)
{
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	s_currentShadeModel = 0;
	s_currentBoundTexId = 0xFFFFFFFF;
	s_currentTex2DEnabled = false;
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	// Disable all lights and reset global ambient
	for (int i = 0; i < 8; ++i) {
		glDisable(GL_LIGHT0 + i);
	}

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	// Projection and view
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((const GLfloat*) projection);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GL11_UploadLight(int lightIdx, GL11_BridgeSceneLight* l)
{
	// Setup light
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLenum lightId = GL_LIGHT0 + lightIdx++;
	const FColor& c = l->color;
	GLfloat col[4] = {c.r, c.g, c.b, c.a};
	const GLfloat zeroAmbient[4] = {0.f, 0.f, 0.f, 1.f};

	if (l->positional == 0.f && l->directional == 0.f) {
		// Ambient light only
		glLightfv(lightId, GL_AMBIENT, col);
		const GLfloat black[4] = {0.f, 0.f, 0.f, 1.f};
		glLightfv(lightId, GL_DIFFUSE, black);
		glLightfv(lightId, GL_SPECULAR, black);
		const GLfloat dummyPos[4] = {0.f, 0.f, 1.f, 0.f};
		glLightfv(lightId, GL_POSITION, dummyPos);
	}
	else {
		glLightfv(lightId, GL_AMBIENT, zeroAmbient);
		glLightfv(lightId, GL_DIFFUSE, col);
		if (l->directional == 1.0f) {
			glLightfv(lightId, GL_SPECULAR, col);
		}
		else {
			const GLfloat black[4] = {0.f, 0.f, 0.f, 1.f};
			glLightfv(lightId, GL_SPECULAR, black);
		}

		GLfloat pos[4];
		if (l->directional == 1.f) {
			pos[0] = -l->direction.x;
			pos[1] = -l->direction.y;
			pos[2] = -l->direction.z;
			pos[3] = 0.f;
		}
		else {
			pos[0] = l->position.x;
			pos[1] = l->position.y;
			pos[2] = l->position.z;
			pos[3] = 1.f;
		}
		glLightfv(lightId, GL_POSITION, pos);
	}
	glEnable(lightId);
}

void GL11_EnableTransparency()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
}

#define NO_TEXTURE_ID 0xffffffff

void GL11_SubmitDraw(
	GLMeshCacheEntry& mesh,
	const Matrix4x4& modelViewMatrix,
	const Appearance& appearance,
	GLuint texId
)
{
	glLoadMatrixf(&modelViewMatrix[0][0]);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
#ifdef __DREAMCAST__
	glColor4ub(appearance.color.r, appearance.color.g, appearance.color.b, appearance.color.unused);
#else
	glColor4ub(appearance.color.r, appearance.color.g, appearance.color.b, appearance.color.a);
#endif

	if (appearance.shininess != 0.0f) {
		GLfloat whiteSpec[] = {1.f, 1.f, 1.f, 1.f};
		glMaterialfv(GL_FRONT, GL_SPECULAR, whiteSpec);
		glMaterialf(GL_FRONT, GL_SHININESS, appearance.shininess);
	}
	else {
		GLfloat noSpec[] = {0.0f, 0.0f, 0.0f, 0.0f};
		glMaterialfv(GL_FRONT, GL_SPECULAR, noSpec);
		glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
	}

	GLenum neededShade = mesh.flat ? GL_FLAT : GL_SMOOTH;
	if (s_currentShadeModel != neededShade) {
		glShadeModel(neededShade);
		s_currentShadeModel = neededShade;
	}

	bool needTex = (appearance.textureId != NO_TEXTURE_ID && texId != 0xFFFFFFFF && texId != 0);
	if (needTex) {
		if (!s_currentTex2DEnabled) {
			glEnable(GL_TEXTURE_2D);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			s_currentTex2DEnabled = true;
		}
		if (s_currentBoundTexId != texId) {
			glBindTexture(GL_TEXTURE_2D, texId);
			s_currentBoundTexId = texId;
		}
	}
	else {
		if (s_currentTex2DEnabled) {
			glDisable(GL_TEXTURE_2D);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			s_currentTex2DEnabled = false;
		}
	}

	if (g_useVBOs) {
		mwglBindBuffer(GL_ARRAY_BUFFER_ARB, mesh.vboPositions);
		glVertexPointer(3, GL_FLOAT, 0, nullptr);

		mwglBindBuffer(GL_ARRAY_BUFFER_ARB, mesh.vboNormals);
		glNormalPointer(GL_FLOAT, 0, nullptr);

		if (needTex) {
			mwglBindBuffer(GL_ARRAY_BUFFER_ARB, mesh.vboTexcoords);
			glTexCoordPointer(2, GL_FLOAT, 0, nullptr);
		}

		mwglBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, mesh.ibo);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_SHORT, nullptr);

		mwglBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		mwglBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
	else {
		if (mesh.vertexData && !mesh.indices.empty()) {
			const char* basePtr = reinterpret_cast<const char*>(mesh.vertexData);
			glVertexPointer(3, GL_FLOAT, sizeof(GL11_BridgeSceneVertex), basePtr + offsetof(GL11_BridgeSceneVertex, position));
			glNormalPointer(GL_FLOAT, sizeof(GL11_BridgeSceneVertex), basePtr + offsetof(GL11_BridgeSceneVertex, normal));
			if (needTex) {
				glTexCoordPointer(2, GL_FLOAT, sizeof(GL11_BridgeSceneVertex), basePtr + offsetof(GL11_BridgeSceneVertex, tu));
			}

			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_SHORT, mesh.indices.data());
		}
	}
}

void GL11_Resize(int width, int height)
{
	glViewport(0, 0, width, height);
}

void GL11_Clear(float r, float g, float b)
{
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GL11_Draw2DImage(
	const GLTextureCacheEntry* cache,
	const SDL_Rect& srcRect,
	const SDL_Rect& dstRect,
	const FColor& color,
	float left,
	float right,
	float bottom,
	float top,
	bool blend
)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glShadeModel(GL_FLAT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(left, right, bottom, top, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glColor4f(color.r, color.g, color.b, color.a);

	float u1 = 0;
	float v1 = 0;
	float u2 = 0;
	float v2 = 0;

	if (cache && cache->glTextureId != 0xFFFFFFFF) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, cache->glTextureId);
		if (blend) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); } else { glDisable(GL_BLEND); }

		u1 = srcRect.x / cache->uScale;
		v1 = srcRect.y / cache->vScale;
		u2 = (srcRect.x + srcRect.w) / cache->uScale;
		v2 = (srcRect.y + srcRect.h) / cache->vScale;
	}
	else {
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	float x1 = (float) dstRect.x;
	float y1 = (float) dstRect.y;
	float x2 = x1 + dstRect.w;
	float y2 = y1 + dstRect.h;
	float z = blend ? -0.999f : 0.999f;

	glBegin(GL_QUADS);
	glTexCoord2f(u1, v1);
	glVertex3f(x1, y1, z);
	glTexCoord2f(u2, v1);
	glVertex3f(x2, y1, z);
	glTexCoord2f(u2, v2);
	glVertex3f(x2, y2, z);
	glTexCoord2f(u1, v2);
	glVertex3f(x1, y2, z);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	s_currentBoundTexId = 0xFFFFFFFF;
	s_currentTex2DEnabled = false;
	s_currentShadeModel = 0;
	// Restore state
	glMatrixMode(GL_MODELVIEW);
}

void GL11_Download(SDL_Surface* target)
{
	glFinish();
	#ifndef __DREAMCAST__
	#ifndef __DREAMCAST__
	glReadPixels(0, 0, target->w, target->h, GL_RGBA, GL_UNSIGNED_BYTE, target->pixels);
#endif
#endif
}

void GL11_SetDither(bool dither)
{
#ifndef __DREAMCAST__
	if (dither) {
		glEnable(GL_DITHER);
	}
	else {
		glDisable(GL_DITHER);
	}
#endif
}

#ifdef __DREAMCAST__
#include <GL/gl.h>
#include <SDL/SDL.h>

extern "C" void MW_DrawLoadingBar(float progress) {
	static float t = 0; // Temp to avoid unused variable warning if any
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	// Draw background (black)
	glColor4f(0, 0, 0, 1);
	glBegin(GL_QUADS);
	glVertex2f(0, 0); glVertex2f(640, 0);
	glVertex2f(640, 480); glVertex2f(0, 480);
	glEnd();

	// Draw bar outline/background (white)
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glVertex2f(98, 398); glVertex2f(542, 398);
	glVertex2f(542, 422); glVertex2f(98, 422);
	glEnd();

	// Draw bar empty background (gray)
	glColor4f(0.2f, 0.2f, 0.2f, 1);
	glBegin(GL_QUADS);
	glVertex2f(100, 400); glVertex2f(540, 400);
	glVertex2f(540, 420); glVertex2f(100, 420);
	glEnd();

	// Draw bar fill (orange)
	glColor4f(1.0f, 0.5f, 0.0f, 1.0f);
	glBegin(GL_QUADS);
	glVertex2f(100, 400); glVertex2f(100 + 440 * progress, 400);
	glVertex2f(100 + 440 * progress, 420); glVertex2f(100, 420);
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glKosSwapBuffers();
}

extern "C" void MW_DrawLoadingBug() {
	static float t = 0;
	t += 0.05f;
	float progress = (sinf(t) + 1.0f) / 2.0f;
	MW_DrawLoadingBar(progress);
}
#endif



















