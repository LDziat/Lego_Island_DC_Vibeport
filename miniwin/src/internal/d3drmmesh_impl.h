#pragma once

#include "d3drmobject_impl.h"

#include <algorithm>
#include <vector>

struct MeshGroup {
	SDL_Color color = {0xFF, 0xFF, 0xFF, 0xFF};
	IDirect3DRMTexture* texture = nullptr;
	IDirect3DRMMaterial* material = nullptr;
	D3DRMRENDERQUALITY quality = D3DRMRENDER_GOURAUD;
	int vertexPerFace = 3;
	int version = 0;
	mutable Uint32 rendererMeshId = 0xFFFFFFFF;
	mutable int rendererVersion = -1;
	std::vector<D3DRMVERTEX> vertices;
	std::vector<DWORD> indices;

	MeshGroup() = default;

	MeshGroup(const MeshGroup& other)
		: color(other.color), texture(other.texture), material(other.material), quality(other.quality),
		  vertexPerFace(other.vertexPerFace), rendererMeshId(other.rendererMeshId),
		  rendererVersion(other.rendererVersion), vertices(std::move(other.vertices)), indices(std::move(other.indices))
	{
		if (texture) {
			texture->AddRef();
		}
		if (material) {
			material->AddRef();
		}
	}

	// Move constructor
	MeshGroup(MeshGroup&& other) noexcept
		: color(other.color), texture(other.texture), material(other.material), quality(other.quality),
		  vertexPerFace(other.vertexPerFace), rendererMeshId(other.rendererMeshId),
		  rendererVersion(other.rendererVersion), vertices(other.vertices), indices(other.indices)
	{
		other.texture = nullptr;
		other.material = nullptr;
		other.rendererMeshId = 0xFFFFFFFF;
		other.rendererVersion = -1;
	}

	// Move assignment
	MeshGroup& operator=(MeshGroup&& other) noexcept
	{
		color = other.color;
		texture = other.texture;
		material = other.material;
		quality = other.quality;
		vertexPerFace = other.vertexPerFace;
		rendererMeshId = other.rendererMeshId;
		rendererVersion = other.rendererVersion;
		vertices = std::move(other.vertices);
		indices = std::move(other.indices);
		other.texture = nullptr;
		other.material = nullptr;
		other.rendererMeshId = 0xFFFFFFFF;
		other.rendererVersion = -1;
		return *this;
	}

	~MeshGroup()
	{
		if (texture) {
			texture->Release();
		}
		if (material) {
			material->Release();
		}
	}
};

struct Direct3DRMMeshImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMMesh> {
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	HRESULT Clone(int flags, GUID iid, void** object) override;
	HRESULT AddGroup(
		unsigned int vertexCount,
		unsigned int faceCount,
		unsigned int vertexPerFace,
		unsigned int* faceBuffer,
		D3DRMGROUPINDEX* groupIndex
	) override;
	HRESULT GetGroup(
		D3DRMGROUPINDEX groupIndex,
		unsigned int* vertexCount,
		unsigned int* faceCount,
		unsigned int* vertexPerFace,
		DWORD* indexCount,
		unsigned int* indices
	) override;
	const MeshGroup& GetGroup(D3DRMGROUPINDEX groupIndex);
	DWORD GetGroupCount() override;
	HRESULT SetGroupColor(D3DRMGROUPINDEX groupIndex, D3DCOLOR color) override;
	HRESULT SetGroupColorRGB(D3DRMGROUPINDEX groupIndex, float r, float g, float b) override;
	D3DCOLOR GetGroupColor(D3DRMGROUPINDEX index) override;
	HRESULT SetGroupMaterial(D3DRMGROUPINDEX groupIndex, IDirect3DRMMaterial* material) override;
	HRESULT SetGroupTexture(D3DRMGROUPINDEX groupIndex, IDirect3DRMTexture* texture) override;
	HRESULT GetGroupTexture(D3DRMGROUPINDEX groupIndex, LPDIRECT3DRMTEXTURE* texture) override;
	HRESULT GetGroupMaterial(D3DRMGROUPINDEX groupIndex, LPDIRECT3DRMMATERIAL* material) override;
	HRESULT SetGroupMapping(D3DRMGROUPINDEX groupIndex, D3DRMMAPPING mapping) override;
	D3DRMMAPPING GetGroupMapping(D3DRMGROUPINDEX groupIndex) override;
	HRESULT SetGroupQuality(D3DRMGROUPINDEX groupIndex, D3DRMRENDERQUALITY quality) override;
	D3DRMRENDERQUALITY GetGroupQuality(D3DRMGROUPINDEX groupIndex) override;
	HRESULT SetVertices(D3DRMGROUPINDEX groupIndex, int offset, int count, D3DRMVERTEX* vertices) override;
	HRESULT GetVertices(D3DRMGROUPINDEX groupIndex, int startIndex, int count, D3DRMVERTEX* vertices) override;
	HRESULT GetBox(D3DRMBOX* box) override;
	const D3DVECTOR& GetSphereCenter() const { return m_sphereCenter; }
	float GetSphereRadius() const { return m_sphereRadius; }
	float GetSphereSizeSq() const { return m_sphereSizeSq; }
	bool IsSphereAtOrigin() const { return m_sphereIsOrigin; }

private:
	void UpdateBox();

	std::vector<MeshGroup> m_groups;
	D3DRMBOX m_box;
	D3DVECTOR m_sphereCenter = {0.0f, 0.0f, 0.0f};
	float m_sphereRadius = 0.0f;
	float m_sphereSizeSq = 0.0f;
	bool m_sphereIsOrigin = true;
};
