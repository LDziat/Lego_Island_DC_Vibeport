extern "C" void MW_DrawLoadingBug();
#ifdef __DREAMCAST__
extern "C" void DebugMenu_Render(void);
extern "C" bool DebugMenu_IsOpen(void);
extern "C" void DebugMenu_RenderFpsOverlay(void);
#endif
#include "actual.h"
#include "d3drmrenderer_opengl1.h"
#include "ddraw_impl.h"
#include "ddsurface_impl.h"
#include "mathutils.h"
#include "meshutils.h"
#include "dcprof.h"
#include "compat.h"

#include <SDL/SDL.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <unordered_map>

static_assert(sizeof(Matrix4x4) == sizeof(D3DRMMATRIX4D), "Matrix4x4 is wrong size");
static_assert(sizeof(GL11_BridgeVector) == sizeof(D3DVECTOR), "GL11_BridgeVector is wrong size");
static_assert(sizeof(GL11_BridgeTexCoord) == sizeof(TexCoord), "GL11_BridgeTexCoord is wrong size");
static_assert(sizeof(GL11_BridgeSceneLight) == sizeof(SceneLight), "GL11_BridgeSceneLight is wrong size");
static_assert(sizeof(GL11_BridgeSceneVertex) == sizeof(D3DRMVERTEX), "GL11_BridgeSceneVertex is wrong size");

Direct3DRMRenderer* OpenGL1Renderer::Create(DWORD width, DWORD height, DWORD msaaSamples)
{
	// We have to reset the attributes here after having enumerated the
	// OpenGL ES 2.0 renderer, or else SDL gets very confused by SDL_GL_DEPTH_SIZE
	// call below when on an EGL-based backend, and crashes with EGL_BAD_MATCH.
	SDL_GL_ResetAttributes();
	// But ResetAttributes resets it to 16.
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	if (msaaSamples > 1) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaSamples);
	}

	if (!DDWindow) {
		SDL_Log("No window handler");
		return nullptr;
	}

	SDL_GLContext context = SDL_GL_CreateContext(DDWindow);
	if (!context) {
		SDL_Log("SDL_GL_CreateContext: %s", SDL_GetError());
		return nullptr;
	}

	if (!SDL_GL_MakeCurrent(DDWindow, context)) {
		SDL_GL_DestroyContext(context);
		SDL_Log("SDL_GL_MakeCurrent: %s", SDL_GetError());
		return nullptr;
	}

	GL11_InitState();

	return new OpenGL1Renderer(width, height, context);
}

OpenGL1Renderer::OpenGL1Renderer(DWORD width, DWORD height, SDL_GLContext context) : m_context(context)
{
	m_width = width;
	m_height = height;
	m_virtualWidth = width;
	m_virtualHeight = height;
#ifdef __DREAMCAST__
	m_renderedImage = nullptr;
#else
	m_renderedImage = SDL_CreateSurface(m_width, m_height, SDL_PIXELFORMAT_RGBA32);
#endif
	GL11_LoadExtensions();
	m_useVBOs = false;
	m_useNPOT = SDL_GL_ExtensionSupported("GL_OES_texture_npot");
}

OpenGL1Renderer::~OpenGL1Renderer()
{
	if (m_renderedImage) {
		SDL_DestroySurface(m_renderedImage);
	}
	SDL_GL_DestroyContext(m_context);
}

void OpenGL1Renderer::PushLights(const SceneLight* lightsArray, size_t count)
{
#ifdef __DREAMCAST__
	m_lights.clear();
	const SceneLight* ambientLight = nullptr;
	const SceneLight* dirLight = nullptr;

	for (size_t i = 0; i < count; ++i) {
		if (lightsArray[i].positional == 0.f && lightsArray[i].directional == 0.f) {
			ambientLight = &lightsArray[i];
		}
		else if (!dirLight) {
			dirLight = &lightsArray[i];
		}
	}

	if (ambientLight) {
		m_lights.push_back(*ambientLight);
		m_ambientIntensity = g_debugAmbientLevel;
	} else {
		m_ambientIntensity = g_debugAmbientLevel;
	}

	if (dirLight) {
		m_lights.push_back(*dirLight);
		// In Direct3DRM, direction points along the light ray. Reverse it so L points towards the light source.
		float dx = -dirLight->direction.x;
		float dy = -dirLight->direction.y;
		float dz = -dirLight->direction.z;
		float lenSq = dx * dx + dy * dy + dz * dz;
		if (lenSq > 0.0001f) {
			float inv = 1.0f / sqrtf(lenSq);
			m_sunDirWorld = { dx * inv, dy * inv, dz * inv };
		} else {
			m_sunDirWorld = { g_debugSunDirX, g_debugSunDirY, g_debugSunDirZ };
		}
	} else {
		m_sunDirWorld = { g_debugSunDirX, g_debugSunDirY, g_debugSunDirZ };
		float lenSq = m_sunDirWorld.x * m_sunDirWorld.x + m_sunDirWorld.y * m_sunDirWorld.y + m_sunDirWorld.z * m_sunDirWorld.z;
		if (lenSq > 0.0001f) {
			float inv = 1.0f / sqrtf(lenSq);
			m_sunDirWorld.x *= inv;
			m_sunDirWorld.y *= inv;
			m_sunDirWorld.z *= inv;
		}
	}
#else
	if (count > 8) {
		SDL_Log("Unsupported number of lights (%d)", static_cast<int>(count));
		count = 8;
	}
	m_lights.assign(lightsArray, lightsArray + count);
#endif
}

void OpenGL1Renderer::SetFrustumPlanes(const Plane* frustumPlanes)
{
}

void OpenGL1Renderer::SetProjection(const D3DRMMATRIX4D& projection, D3DVALUE front, D3DVALUE back)
{
	memcpy(&m_projection, projection, sizeof(D3DRMMATRIX4D));
}

struct TextureDestroyContextGL {
	OpenGL1Renderer* renderer;
	Uint32 textureId;
};

void OpenGL1Renderer::AddTextureDestroyCallback(Uint32 id, IDirect3DRMTexture* texture)
{
	auto* ctx = new TextureDestroyContextGL{this, id};
	texture->AddDestroyCallback(
		[](IDirect3DRMObject* obj, void* arg) {
			auto* ctx = static_cast<TextureDestroyContextGL*>(arg);
			auto& cache = ctx->renderer->m_textures[ctx->textureId];
			if (cache.texture) {
				static_cast<Direct3DRMTextureImpl*>(cache.texture)->rendererTextureId = 0xFFFFFFFF;
				ctx->renderer->m_textureToId.erase(cache.texture);
			}
			if (cache.glTextureId != 0) {
				GL11_DestroyTexture(cache.glTextureId);
				cache.glTextureId = 0;
				cache.texture = nullptr;
			}
			delete ctx;
		},
		ctx
	);
}

static int NextPowerOfTwo(int v)
{
	int power = 1;
	while (power < v) {
		power <<= 1;
	}
	return power;
}

static Uint32 UploadTextureData(SDL_Surface* src, bool useNPOT, bool isUI, float scaleX, float scaleY, GLuint existingTexId = 0)
{
	DC_ZONE(Z_TEXUPLOAD);
	DC_COUNT(C_TEX_UPLOADS, 1);
#ifdef __DREAMCAST__
	int newW = useNPOT ? src->w : NextPowerOfTwo(src->w);
	int newH = useNPOT ? src->h : NextPowerOfTwo(src->h);

	bool scaleUI = false;
	bool scale3D = false;
	
	if (isUI && (newW > 512 || newH > 512)) { newW = 512; newH = 512; scaleUI = true; }
	if (!isUI && newW > 64) {
		newW = 64;
		newH = 64;
		scale3D = true;
	}

	if (newW < 8) newW = 8;
	if (newH < 8) newH = 8;

	DC_COUNT(C_TEX_PIXELS, newW * newH);
	
	std::vector<Uint16> fullBuffer;
	fullBuffer.resize(newW * newH);

	std::vector<int> xmap;
	std::vector<int> ymap;
	if (scaleUI || scale3D) {
		xmap.resize(newW);
		for (int x = 0; x < newW; ++x) {
			xmap[x] = (x * src->w) / newW;
		}
		ymap.resize(newH);
		for (int y = 0; y < newH; ++y) {
			ymap[y] = (y * src->h) / newH;
		}
	}
	
	SDL_Palette* pal = src->format->palette;
	
	if (src->format->BitsPerPixel == 8 && pal) {
		bool hasColorkey = (src->flags & SDL_SRCCOLORKEY) != 0;
		Uint32 colorkey = hasColorkey ? src->format->colorkey : 0xFFFFFFFF;

		// Precompute 256-entry ARGB4444 palette table
		Uint16 palLut[256];
		int ncolors = pal->ncolors < 256 ? pal->ncolors : 256;
		for (int i = 0; i < ncolors; ++i) {
			if (hasColorkey && (Uint32)i == colorkey) {
				palLut[i] = 0;
			} else {
				SDL_Color c = pal->colors[i];
				Uint16 r4 = c.r >> 4;
				Uint16 g4 = c.g >> 4;
				Uint16 b4 = c.b >> 4;
				palLut[i] = (0xF << 12) | (r4 << 8) | (g4 << 4) | b4;
			}
		}
		for (int i = ncolors; i < 256; ++i) {
			palLut[i] = 0;
		}

		if (scaleUI || scale3D) {
			for (int y = 0; y < newH; ++y) {
				const Uint8* srcRow = (const Uint8*)src->pixels + ymap[y] * src->pitch;
				Uint16* dstRow = &fullBuffer[y * newW];
				for (int x = 0; x < newW; ++x) {
					dstRow[x] = palLut[srcRow[xmap[x]]];
				}
			}
		} else {
			for (int y = 0; y < src->h; ++y) {
				const Uint8* srcRow = (const Uint8*)src->pixels + y * src->pitch;
				Uint16* dstRow = &fullBuffer[y * newW];
				for (int x = 0; x < src->w; ++x) {
					dstRow[x] = palLut[srcRow[x]];
				}
			}
		}
	} else if (src->format->BitsPerPixel == 16) {
		if (scaleUI || scale3D) {
			for (int y = 0; y < newH; ++y) {
				const Uint16* srcRow = (const Uint16*)((const Uint8*)src->pixels + ymap[y] * src->pitch);
				Uint16* dstRow = &fullBuffer[y * newW];
				for (int x = 0; x < newW; ++x) {
					Uint16 px = srcRow[xmap[x]];
					Uint16 r4 = (px >> 12) & 0xF;
					Uint16 g4 = (px >> 7) & 0xF;
					Uint16 b4 = (px >> 1) & 0xF;
					dstRow[x] = (0xF << 12) | (r4 << 8) | (g4 << 4) | b4;
				}
			}
		} else {
			for (int y = 0; y < src->h; ++y) {
				const Uint16* srcRow = (const Uint16*)((const Uint8*)src->pixels + y * src->pitch);
				Uint16* dstRow = &fullBuffer[y * newW];
				for (int x = 0; x < src->w; ++x) {
					Uint16 px = srcRow[x];
					Uint16 r4 = (px >> 12) & 0xF;
					Uint16 g4 = (px >> 7) & 0xF;
					Uint16 b4 = (px >> 1) & 0xF;
					dstRow[x] = (0xF << 12) | (r4 << 8) | (g4 << 4) | b4;
				}
			}
		}
	} else if (src->format->BitsPerPixel == 32) {
		bool hasColorkey = (src->flags & SDL_SRCCOLORKEY) != 0;
		Uint32 colorkey = hasColorkey ? src->format->colorkey : 0;
		if (scaleUI || scale3D) {
			for (int y = 0; y < newH; ++y) {
				const Uint32* srcRow = (const Uint32*)((const Uint8*)src->pixels + ymap[y] * src->pitch);
				Uint16* dstRow = &fullBuffer[y * newW];
				for (int x = 0; x < newW; ++x) {
					Uint32 px = srcRow[xmap[x]];
					if (hasColorkey && px == colorkey) {
						dstRow[x] = 0;
					} else {
						Uint16 a4 = (px >> 28) & 0xF;
						Uint16 r4 = (px >> 20) & 0xF;
						Uint16 g4 = (px >> 12) & 0xF;
						Uint16 b4 = (px >> 4) & 0xF;
						dstRow[x] = (a4 << 12) | (r4 << 8) | (g4 << 4) | b4;
					}
				}
			}
		} else {
			for (int y = 0; y < src->h; ++y) {
				const Uint32* srcRow = (const Uint32*)((const Uint8*)src->pixels + y * src->pitch);
				Uint16* dstRow = &fullBuffer[y * newW];
				for (int x = 0; x < src->w; ++x) {
					Uint32 px = srcRow[x];
					if (hasColorkey && px == colorkey) {
						dstRow[x] = 0;
					} else {
						Uint16 a4 = (px >> 28) & 0xF;
						Uint16 r4 = (px >> 20) & 0xF;
						Uint16 g4 = (px >> 12) & 0xF;
						Uint16 b4 = (px >> 4) & 0xF;
						dstRow[x] = (a4 << 12) | (r4 << 8) | (g4 << 4) | b4;
					}
				}
			}
		}
	}
	
	// Create the texture in GLdc
	Uint32 texId = GL11_UploadTextureData(fullBuffer.data(), newW, newH, isUI, scaleX, scaleY, existingTexId);
	return texId;
#else
	SDL_Surface* working = src;
	if (src->format != SDL_PIXELFORMAT_RGBA32) {
		working = SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA32);
		if (!working) {
			SDL_Log("SDL_ConvertSurface failed: %s", SDL_GetError());
			return NO_TEXTURE_ID;
		}
	}

	SDL_Surface* finalSurface = working;

	int newW = working->w;
	int newH = working->h;
	if (!useNPOT) {
		newW = NextPowerOfTwo(newW);
		newH = NextPowerOfTwo(newH);
	}
	int max = GL11_GetMaxTextureSize();
	if (newW > max) {
		newW = max;
	}
	if (newH > max) {
		newH = max;
	}

	if (newW != working->w || newH != working->h) {
		SDL_Surface* resized = SDL_CreateSurface(newW, newH, working->format);
		if (!resized) {
			SDL_Log("SDL_CreateSurface (resize) failed: %s", SDL_GetError());
			if (working != src) {
				SDL_DestroySurface(working);
			}
			return NO_TEXTURE_ID;
		}

		SDL_Rect srcRect = {0, 0, working->w, working->h};
		SDL_Rect dstRect = {0, 0, newW, newH};
		SDL_BlitSurfaceScaled(working, &srcRect, resized, &dstRect, SDL_SCALEMODE_NEAREST);

		if (working != src) {
			SDL_DestroySurface(working);
		}
		finalSurface = resized;
	}

	Uint32 texId = GL11_UploadTextureData(finalSurface->pixels, finalSurface->w, finalSurface->h, isUI, scaleX, scaleY);
	if (finalSurface != src) {
		SDL_DestroySurface(finalSurface);
	}
	return texId;
#endif
}

Uint32 OpenGL1Renderer::GetTextureId(IDirect3DRMTexture* iTexture, bool isUI, float scaleX, float scaleY)
{
	auto texture = static_cast<Direct3DRMTextureImpl*>(iTexture);

	if (texture->rendererTextureId != 0xFFFFFFFF && texture->rendererVersion == texture->m_version) {
		return texture->rendererTextureId;
	}

	auto surface = static_cast<DirectDrawSurfaceImpl*>(texture->m_surface);

	int newW = m_useNPOT ? surface->m_surface->w : NextPowerOfTwo(surface->m_surface->w);
	int newH = m_useNPOT ? surface->m_surface->h : NextPowerOfTwo(surface->m_surface->h);
	
	bool scaled = false;
#ifdef __DREAMCAST__
	if (isUI && (newW > 512 || newH > 512)) {
		newW = 512;
		newH = 512;
		scaled = true;
	}
	if (!isUI && newW > 64) {
		newW = 64;
		newH = 64;
		scaled = true;
	}
	if (newW < 8) newW = 8;
	if (newH < 8) newH = 8;
#endif

	float finalUScale = scaled ? surface->m_surface->w : newW;
	float finalVScale = scaled ? surface->m_surface->h : newH;

	auto it = m_textureToId.find(iTexture);
	if (it != m_textureToId.end()) {
		Uint32 id = it->second;
		auto& tex = m_textures[id];
		if (tex.version != texture->m_version) {
#ifdef __DREAMCAST__
			if (tex.width == newW && tex.height == newH && tex.glTextureId != 0) {
				tex.glTextureId = UploadTextureData(surface->m_surface, m_useNPOT, isUI, scaleX, scaleY, tex.glTextureId);
			} else {
				GL11_DestroyTexture(tex.glTextureId);
				tex.glTextureId = UploadTextureData(surface->m_surface, m_useNPOT, isUI, scaleX, scaleY);
			}
#else
			GL11_DestroyTexture(tex.glTextureId);
			tex.glTextureId = UploadTextureData(surface->m_surface, m_useNPOT, isUI, scaleX, scaleY);
#endif
			tex.version = texture->m_version;
			tex.width = newW;
			tex.height = newH;
			tex.uScale = finalUScale;
			tex.vScale = finalVScale;
		}
		texture->rendererTextureId = id;
		texture->rendererVersion = texture->m_version;
		return id;
	}

	GLuint texId = UploadTextureData(surface->m_surface, m_useNPOT, isUI, scaleX, scaleY);

	for (Uint32 i = 0; i < m_textures.size(); ++i) {
		auto& tex = m_textures[i];
		if (!tex.texture) {
			tex.texture = texture;
			tex.version = texture->m_version;
			tex.glTextureId = texId;
			tex.width = newW;
			tex.height = newH;
			tex.uScale = finalUScale;
			tex.vScale = finalVScale;
			m_textureToId[iTexture] = i;
			texture->rendererTextureId = i;
			texture->rendererVersion = texture->m_version;
			AddTextureDestroyCallback(i, texture);
			return i;
		}
	}

	Uint32 newId = (Uint32) m_textures.size();
	m_textures.push_back(
		{texture,
		 texture->m_version,
		 texId,
		 static_cast<float>(newW),
		 static_cast<float>(newH),
		 static_cast<float>(finalUScale),
		 static_cast<float>(finalVScale)}
	);
	m_textureToId[iTexture] = newId;
	texture->rendererTextureId = newId;
	texture->rendererVersion = texture->m_version;
	AddTextureDestroyCallback(newId, texture);
	return newId;
}

#ifdef __DREAMCAST__
static bool StripifyTriangles(
	const uint16_t* inIndices,
	size_t numIndices,
	std::vector<uint16_t>& outStrip
)
{
	size_t numTris = numIndices / 3;
	if (numTris < 2) return false;

	struct Tri {
		uint16_t v[3];
	};
	const Tri* tris = reinterpret_cast<const Tri*>(inIndices);

	// Edge hash table: maps directed edge (u, v) -> (tri_idx, third_vertex)
	std::unordered_map<uint32_t, std::pair<size_t, uint16_t>> edgeMap;
	edgeMap.reserve(numTris * 3);
	for (size_t i = 0; i < numTris; ++i) {
		uint16_t a = tris[i].v[0], b = tris[i].v[1], c = tris[i].v[2];
		edgeMap[(static_cast<uint32_t>(a) << 16) | b] = {i, c};
		edgeMap[(static_cast<uint32_t>(b) << 16) | c] = {i, a};
		edgeMap[(static_cast<uint32_t>(c) << 16) | a] = {i, b};
	}

	std::vector<bool> visited(numTris, false);
	outStrip.clear();
	outStrip.reserve(numIndices);

	auto connectTri = [&](uint16_t u0, uint16_t u1, uint16_t u2) {
		if (outStrip.empty()) {
			outStrip.push_back(u0);
			outStrip.push_back(u1);
			outStrip.push_back(u2);
			return;
		}
		uint16_t last = outStrip.back();
		size_t S = outStrip.size();
		if (S % 2 != 0) {
			outStrip.push_back(last);
			outStrip.push_back(last);
			outStrip.push_back(u0);
		} else {
			outStrip.push_back(last);
			outStrip.push_back(u0);
		}
		outStrip.push_back(u0);
		outStrip.push_back(u1);
		outStrip.push_back(u2);
	};

	for (size_t startIdx = 0; startIdx < numTris; ++startIdx) {
		if (visited[startIdx]) continue;
		visited[startIdx] = true;
		connectTri(tris[startIdx].v[0], tris[startIdx].v[1], tris[startIdx].v[2]);

		while (true) {
			size_t L = outStrip.size();
			uint16_t e0, e1;
			if ((L - 2) % 2 == 0) {
				e0 = outStrip[L - 2];
				e1 = outStrip[L - 1];
			} else {
				e0 = outStrip[L - 1];
				e1 = outStrip[L - 2];
			}

			uint32_t key = (static_cast<uint32_t>(e0) << 16) | e1;
			auto it = edgeMap.find(key);
			if (it != edgeMap.end() && !visited[it->second.first]) {
				visited[it->second.first] = true;
				outStrip.push_back(it->second.second);
			} else {
				break;
			}
		}
	}

	// Safety fallback: only accept if strip saves at least 15% indices
	if (outStrip.size() > numIndices * 85 / 100) {
		return false;
	}
	return true;
}
#endif

GLMeshCacheEntry GLUploadMesh(const MeshGroup& meshGroup, bool useVBOs)
{
	DC_ZONE(Z_MESHUPLOAD);
	DC_COUNT(C_MESH_UPLOADS, 1);
	GLMeshCacheEntry cache;
	cache.meshGroup = &meshGroup;
	cache.version = meshGroup.version;
	cache.flat = meshGroup.quality == D3DRMRENDER_FLAT || meshGroup.quality == D3DRMRENDER_UNLITFLAT;
	cache.isUnlit = (meshGroup.quality == D3DRMRENDER_UNLITFLAT);

	if (cache.flat) {
		std::vector<D3DRMVERTEX> flattenedVertices;
		FlattenSurfaces(
			meshGroup.vertices.data(),
			meshGroup.vertices.size(),
			meshGroup.indices.data(),
			meshGroup.indices.size(),
			meshGroup.texture != nullptr,
			flattenedVertices,
			cache.indices
		);
		cache.flatVertices.resize(flattenedVertices.size());
		memcpy(cache.flatVertices.data(), flattenedVertices.data(), flattenedVertices.size() * sizeof(GL11_BridgeSceneVertex));
		cache.vertexData = cache.flatVertices.data();
		cache.vertexCount = cache.flatVertices.size();
	}
	else {
		cache.indices.resize(meshGroup.indices.size());
		std::transform(meshGroup.indices.begin(), meshGroup.indices.end(), cache.indices.begin(), [](DWORD index) {
			return static_cast<uint16_t>(index);
		});
#ifdef __DREAMCAST__
		if (g_debugStrips) {
			std::vector<uint16_t> strip;
			if (StripifyTriangles(cache.indices.data(), cache.indices.size(), strip)) {
				cache.indices = std::move(strip);
				cache.isStrip = true;
			}
		}
#endif
		cache.vertexData = reinterpret_cast<const GL11_BridgeSceneVertex*>(meshGroup.vertices.data());
		cache.vertexCount = meshGroup.vertices.size();
	}

	GL11_UploadMesh(cache, meshGroup.texture != nullptr);

	return cache;
}

struct GLMeshDestroyContext {
	OpenGL1Renderer* renderer;
	Uint32 id;
};

void OpenGL1Renderer::AddMeshDestroyCallback(Uint32 id, IDirect3DRMMesh* mesh)
{
	auto* ctx = new GLMeshDestroyContext{this, id};
	mesh->AddDestroyCallback(
		[](IDirect3DRMObject*, void* arg) {
			auto* ctx = static_cast<GLMeshDestroyContext*>(arg);
			auto& cache = ctx->renderer->m_meshs[ctx->id];
			if (cache.meshGroup) {
				cache.meshGroup->rendererMeshId = 0xFFFFFFFF;
				ctx->renderer->m_meshGroupToId.erase(cache.meshGroup);
			}
			cache.meshGroup = nullptr;
			GL11_DestroyMesh(cache);
			delete ctx;
		},
		ctx
	);
}

Uint32 OpenGL1Renderer::GetMeshId(IDirect3DRMMesh* mesh, const MeshGroup* meshGroup)
{
	if (meshGroup->rendererMeshId != 0xFFFFFFFF && meshGroup->rendererVersion == meshGroup->version) {
		return meshGroup->rendererMeshId;
	}

	auto it = m_meshGroupToId.find(meshGroup);
	if (it != m_meshGroupToId.end()) {
		Uint32 id = it->second;
		auto& cache = m_meshs[id];
		if (cache.version != meshGroup->version) {
			cache = std::move(GLUploadMesh(*meshGroup, m_useVBOs));
		}
		meshGroup->rendererMeshId = id;
		meshGroup->rendererVersion = meshGroup->version;
		return id;
	}

	auto newCache = GLUploadMesh(*meshGroup, m_useVBOs);

	for (Uint32 i = 0; i < m_meshs.size(); ++i) {
		auto& cache = m_meshs[i];
		if (!cache.meshGroup) {
			cache = std::move(newCache);
			m_meshGroupToId[meshGroup] = i;
			meshGroup->rendererMeshId = i;
			meshGroup->rendererVersion = meshGroup->version;
			AddMeshDestroyCallback(i, mesh);
			return i;
		}
	}

	Uint32 newId = (Uint32) m_meshs.size();
	m_meshs.push_back(std::move(newCache));
	m_meshGroupToId[meshGroup] = newId;
	meshGroup->rendererMeshId = newId;
	meshGroup->rendererVersion = meshGroup->version;
	AddMeshDestroyCallback(newId, mesh);
	return newId;
}

HRESULT OpenGL1Renderer::BeginFrame()
{
	SDL_GL_MakeCurrent(DDWindow, m_context);
#ifdef __DREAMCAST__
	m_frameVerticesSubmitted = 0;
#endif
	GL11_BeginFrame((Matrix4x4*) &m_projection[0][0]);

	int lightIdx = 0;
	for (const auto& l : m_lights) {
		if (lightIdx > 7) {
			break;
		}
		GL11_UploadLight(lightIdx, (GL11_BridgeSceneLight*) &l);

		lightIdx++;
	}
	return DD_OK;
}

void OpenGL1Renderer::EnableTransparency()
{
	GL11_EnableTransparency();
}

void OpenGL1Renderer::SubmitDraw(
	DWORD meshId,
	const D3DRMMATRIX4D& modelViewMatrix,
	const D3DRMMATRIX4D& worldMatrix,
	const D3DRMMATRIX4D& viewMatrix,
	const Matrix3x3& normalMatrix,
	const Appearance& appearance
)
{
	DC_ZONE(Z_GLSUBMIT);
	auto& mesh = m_meshs[meshId];

	if (mesh.flat) {
		mesh.vertexData = mesh.flatVertices.data();
		mesh.vertexCount = mesh.flatVertices.size();
	}
	else if (mesh.meshGroup) {
		mesh.vertexData = reinterpret_cast<const GL11_BridgeSceneVertex*>(mesh.meshGroup->vertices.data());
		mesh.vertexCount = mesh.meshGroup->vertices.size();
	}

	if (!mesh.vertexData || mesh.indices.empty()) {
		return;
	}

#ifdef __DREAMCAST__
	if (m_frameVerticesSubmitted + mesh.indices.size() > 24000) {
		DC_COUNT(C_INDICES_DROPPED, mesh.indices.size());
		return;
	}
	m_frameVerticesSubmitted += mesh.indices.size();
#endif
	DC_COUNT(C_DRAWS, 1);
	DC_COUNT(C_INDICES, mesh.indices.size());

#ifdef __DREAMCAST__
	Appearance litAppearance = appearance;
	if (!g_debugDisableLighting && (g_debugShadingMode == 1 || g_debugShadingMode == 2) && !mesh.isUnlit) {
		// Transform sun direction into model space
		float lmx, lmy, lmz;
		bool hasWorld = (worldMatrix[0][0] != 0.0f || worldMatrix[1][1] != 0.0f || worldMatrix[2][2] != 0.0f);
		if (hasWorld) {
			lmx = worldMatrix[0][0] * m_sunDirWorld.x + worldMatrix[0][1] * m_sunDirWorld.y + worldMatrix[0][2] * m_sunDirWorld.z;
			lmy = worldMatrix[1][0] * m_sunDirWorld.x + worldMatrix[1][1] * m_sunDirWorld.y + worldMatrix[1][2] * m_sunDirWorld.z;
			lmz = worldMatrix[2][0] * m_sunDirWorld.x + worldMatrix[2][1] * m_sunDirWorld.y + worldMatrix[2][2] * m_sunDirWorld.z;
		} else {
			float lvx = viewMatrix[0][0] * m_sunDirWorld.x + viewMatrix[0][1] * m_sunDirWorld.y + viewMatrix[0][2] * m_sunDirWorld.z;
			float lvy = viewMatrix[0][1] * m_sunDirWorld.x + viewMatrix[0][1] * m_sunDirWorld.y + viewMatrix[0][2] * m_sunDirWorld.z;
			float lvz = viewMatrix[0][2] * m_sunDirWorld.x + viewMatrix[0][2] * m_sunDirWorld.y + viewMatrix[0][2] * m_sunDirWorld.z;
			lmx = modelViewMatrix[0][0] * lvx + modelViewMatrix[0][1] * lvy + modelViewMatrix[0][2] * lvz;
			lmy = modelViewMatrix[1][0] * lvx + modelViewMatrix[1][1] * lvy + modelViewMatrix[1][2] * lvz;
			lmz = modelViewMatrix[2][0] * lvx + modelViewMatrix[2][1] * lvy + modelViewMatrix[2][2] * lvz;
		}
		float lmLenSq = lmx * lmx + lmy * lmy + lmz * lmz;
		if (lmLenSq > 0.0001f) {
			float inv = 1.0f / sqrtf(lmLenSq);
			lmx *= inv;
			lmy *= inv;
			lmz *= inv;
		}

		if (g_debugShadingMode == 1) {
			// Mode 1: Fast Group/Object Directional Shading
			float normSq = mesh.avgNormal.x * mesh.avgNormal.x + mesh.avgNormal.y * mesh.avgNormal.y + mesh.avgNormal.z * mesh.avgNormal.z;
			if (normSq >= 0.01f) {
				float dot = mesh.avgNormal.x * lmx + mesh.avgNormal.y * lmy + mesh.avgNormal.z * lmz;
				float halfLambert = 0.5f * dot + 0.5f;
				if (halfLambert < 0.0f) halfLambert = 0.0f;
				if (halfLambert > 1.0f) halfLambert = 1.0f;
				float intensity = m_ambientIntensity + (1.0f - m_ambientIntensity) * halfLambert;
				if (intensity > 1.0f) intensity = 1.0f;
				if (intensity < m_ambientIntensity) intensity = m_ambientIntensity;

				litAppearance.color.r = static_cast<Uint8>(std::min(255.0f, appearance.color.r * intensity));
				litAppearance.color.g = static_cast<Uint8>(std::min(255.0f, appearance.color.g * intensity));
				litAppearance.color.b = static_cast<Uint8>(std::min(255.0f, appearance.color.b * intensity));
			}
			mesh.litColors.clear();
		} else if (g_debugShadingMode == 2 && mesh.vertexData && mesh.vertexCount > 0) {
			// Mode 2: Per-Vertex Half-Lambert Shading
			mesh.litColors.resize(mesh.vertexCount);
			float baseA = appearance.color.unused / 255.0f;
			float baseR = appearance.color.r / 255.0f;
			float baseG = appearance.color.g / 255.0f;
			float baseB = appearance.color.b / 255.0f;
			for (size_t i = 0; i < mesh.vertexCount; ++i) {
				const auto& n = mesh.vertexData[i].normal;
				float nSq = n.x * n.x + n.y * n.y + n.z * n.z;
				float intensity = 1.0f;
				if (nSq >= 0.01f) {
					float dot = n.x * lmx + n.y * lmy + n.z * lmz;
					float hl = 0.5f * dot + 0.5f;
					if (hl < 0.0f) hl = 0.0f;
					if (hl > 1.0f) hl = 1.0f;
					intensity = m_ambientIntensity + (1.0f - m_ambientIntensity) * hl;
					if (intensity > 1.0f) intensity = 1.0f;
					if (intensity < m_ambientIntensity) intensity = m_ambientIntensity;
				}
				mesh.litColors[i] = { baseA, baseR * intensity, baseG * intensity, baseB * intensity };
			}
		}
	}
#else
	const Appearance& litAppearance = appearance;
#endif

	// Bind texture if present
	if (appearance.textureId != NO_TEXTURE_ID) {
		auto& tex = m_textures[appearance.textureId];
		GL11_SubmitDraw(mesh, modelViewMatrix, litAppearance, tex.glTextureId);
	}
	else {
		GL11_SubmitDraw(mesh, modelViewMatrix, litAppearance, 0);
	}
}

HRESULT OpenGL1Renderer::FinalizeFrame()
{
	return DD_OK;
}

void OpenGL1Renderer::Resize(int width, int height, const ViewportTransform& viewportTransform)
{
	SDL_GL_MakeCurrent(DDWindow, m_context);
	m_width = width;
	m_height = height;
	m_viewportTransform = viewportTransform;
#ifndef __DREAMCAST__
	if (m_renderedImage) {
		SDL_DestroySurface(m_renderedImage);
	}
	m_renderedImage = SDL_CreateSurface(m_width, m_height, SDL_PIXELFORMAT_RGBA32);
#endif
	GL11_Resize(width, height);
}

void OpenGL1Renderer::Clear(float r, float g, float b)
{
	SDL_GL_MakeCurrent(DDWindow, m_context);
	m_dirty = true;
	GL11_Clear(r, g, b);
}

void OpenGL1Renderer::Flip()
{
	SDL_GL_MakeCurrent(DDWindow, m_context);
#ifdef __DREAMCAST__
	if (DebugMenu_IsOpen() || g_debugShowFps) {
		m_dirty = true;
	}
#endif
	if (m_dirty) {
		{
#ifdef __DREAMCAST__
			if (g_debugShowFps) {
				DebugMenu_RenderFpsOverlay();
			}
			DebugMenu_Render();
#endif
			DC_ZONE(Z_SWAP);
			SDL_GL_SwapWindow(DDWindow);
		}
		m_dirty = false;
	}
}

void OpenGL1Renderer::Draw2DImage(Uint32 textureId, const SDL_Rect& srcRect, const SDL_Rect& dstRect, FColor color, bool blend)
{
	SDL_GL_MakeCurrent(DDWindow, m_context);
	m_dirty = true;

	float left = -m_viewportTransform.offsetX / m_viewportTransform.scale;
	float right = (m_width - m_viewportTransform.offsetX) / m_viewportTransform.scale;
	float top = -m_viewportTransform.offsetY / m_viewportTransform.scale;
	float bottom = (m_height - m_viewportTransform.offsetY) / m_viewportTransform.scale;

	const GLTextureCacheEntry* texture = nullptr;
	if (textureId != NO_TEXTURE_ID) {
		texture = &m_textures[textureId];
	}

	GL11_Draw2DImage(texture, srcRect, dstRect, color, left, right, bottom, top, blend);
}

void OpenGL1Renderer::SetDither(bool dither)
{
	GL11_SetDither(dither);
}

void OpenGL1Renderer::Download(SDL_Surface* target)
{
#ifdef __DREAMCAST__
	SDL_FillSurfaceRect(target, nullptr, SDL_MapRGB(target->format, 0, 0, 0));
	return;
#endif

	GL11_Download(m_renderedImage);

	SDL_Rect srcRect = {
		static_cast<int>(m_viewportTransform.offsetX),
		static_cast<int>(m_viewportTransform.offsetY),
		static_cast<int>(target->w * m_viewportTransform.scale),
		static_cast<int>(target->h * m_viewportTransform.scale),
	};

	SDL_Surface* bufferClone = SDL_CreateSurface(target->w, target->h, SDL_PIXELFORMAT_RGBA32);
	if (!bufferClone) {
		SDL_Log("SDL_CreateSurface: %s", SDL_GetError());
		return;
	}

	SDL_BlitSurfaceScaled(m_renderedImage, &srcRect, bufferClone, nullptr, SDL_SCALEMODE_NEAREST);

	// Flip image vertically into target
	SDL_Rect rowSrc = {0, 0, bufferClone->w, 1};
	SDL_Rect rowDst = {0, 0, bufferClone->w, 1};
	for (int y = 0; y < bufferClone->h; ++y) {
		rowSrc.y = y;
		rowDst.y = bufferClone->h - 1 - y;
		SDL_BlitSurface(bufferClone, &rowSrc, target, &rowDst);
	}

	SDL_DestroySurface(bufferClone);
}

void OpenGL1Renderer::SetPalette(SDL_Color* palette) {}


























