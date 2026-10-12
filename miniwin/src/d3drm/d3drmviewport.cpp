#include "d3drm_impl.h"
#include "d3drmframe_impl.h"
#include "d3drmlight_impl.h"
#include "d3drmmesh_impl.h"
#include "d3drmrenderer.h"
#include "d3drmviewport_impl.h"
#include "d3drmvisual_impl.h"
#include "ddraw_impl.h"
#include "mathutils.h"
#include "miniwin.h"

#include <SDL/SDL.h>
#include <SDL/SDL_stdinc.h>
#include <cassert>
#include <float.h>
#include <functional>
#include <math.h>

Direct3DRMViewportImpl::Direct3DRMViewportImpl(DWORD width, DWORD height, Direct3DRMRenderer* renderer)
	: m_virtualWidth(width), m_virtualHeight(height), m_renderer(renderer)
{
}

Direct3DRMViewportImpl::~Direct3DRMViewportImpl()
{
	if (m_camera) {
		m_camera->Release();
		m_camera = nullptr;
	}
}

static inline void D3DRMMatrixMultiply(D3DRMMATRIX4D out, const D3DRMMATRIX4D a, const D3DRMMATRIX4D b)
{
	MultiplyMatrix(out, a, b);
}

static void D3DRMMatrixInvertOrthogonal(D3DRMMATRIX4D out, const D3DRMMATRIX4D m)
{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			out[i][j] = m[j][i];
		}
	}

	out[0][3] = out[1][3] = out[2][3] = 0.f;
	out[3][3] = 1.f;

	D3DVECTOR t = {m[3][0], m[3][1], m[3][2]};

	out[3][0] = -(out[0][0] * t.x + out[1][0] * t.y + out[2][0] * t.z);
	out[3][1] = -(out[0][1] * t.x + out[1][1] * t.y + out[2][1] * t.z);
	out[3][2] = -(out[0][2] * t.x + out[1][2] * t.y + out[2][2] * t.z);
}

static void ComputeFrameWorldMatrix(IDirect3DRMFrame* frame, D3DRMMATRIX4D out)
{
	D3DRMMATRIX4D acc = {{1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}};

	IDirect3DRMFrame* cur = frame;
	while (cur) {
		auto* impl = static_cast<Direct3DRMFrameImpl*>(cur);
		D3DRMMATRIX4D tmp;
		D3DRMMatrixMultiply(tmp, impl->m_transform, acc);
		memcpy(acc, tmp, sizeof(acc));

		if (cur == impl->m_parent) {
			break;
		}
		cur = impl->m_parent;
	}
	memcpy(out, acc, sizeof(acc));
}

void Direct3DRMViewportImpl::CollectLightsFromFrame(
	IDirect3DRMFrame* frame,
	D3DRMMATRIX4D parentToWorld,
	std::vector<SceneLight>& lights
)
{
	if (lights.size() >= 3) {
		return;
	}

	auto* frameImpl = static_cast<Direct3DRMFrameImpl*>(frame);
	D3DRMMATRIX4D worldMatrix;
	D3DRMMatrixMultiply(worldMatrix, parentToWorld, frameImpl->m_transform);

	if (frameImpl->m_lights) {
		const auto& lightItems = frameImpl->m_lights->GetItems();
		for (auto* light : lightItems) {
			if (!light) continue;
			D3DCOLOR color = light->GetColor();
			SceneLight extracted;
			extracted.color = {
				((color >> 16) & 0xFF) / 255.0f,
				((color >> 8) & 0xFF) / 255.0f,
				((color >> 0) & 0xFF) / 255.0f,
				((color >> 24) & 0xFF) / 255.0f
			};

			D3DRMLIGHTTYPE type = light->GetType();
#ifdef __DREAMCAST__
			if (type == D3DRMLIGHT_POINT || type == D3DRMLIGHT_PARALLELPOINT) {
				// Convert point lights to directional lights on Dreamcast to eliminate
				// expensive per-vertex distance, square root, and attenuation math in GLdc
				D3DVECTOR dir = Normalize({-worldMatrix[3][0], -worldMatrix[3][1], -worldMatrix[3][2]});
				extracted.direction = dir;
				extracted.directional = 1.f;
				extracted.positional = 0.f;
			}
			else if (type == D3DRMLIGHT_DIRECTIONAL || type == D3DRMLIGHT_SPOT) {
				extracted.direction = {worldMatrix[2][0], worldMatrix[2][1], worldMatrix[2][2]};
				extracted.directional = 1.f;
			}
#else
			if (type == D3DRMLIGHT_POINT || type == D3DRMLIGHT_SPOT || type == D3DRMLIGHT_PARALLELPOINT) {
				extracted.position = {worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2]};
				extracted.positional = 1.f;
			}
			if (type == D3DRMLIGHT_DIRECTIONAL || type == D3DRMLIGHT_SPOT) {
				extracted.direction = {worldMatrix[2][0], worldMatrix[2][1], worldMatrix[2][2]};
				extracted.directional = 1.f;
			}
#endif
			lights.push_back(extracted);
			if (lights.size() >= 3) {
				return;
			}
		}
	}

	if (frameImpl->m_children) {
		const auto& childItems = frameImpl->m_children->GetItems();
		for (auto* childFrame : childItems) {
			if (!childFrame) continue;
			CollectLightsFromFrame(childFrame, worldMatrix, lights);
			if (lights.size() >= 3) {
				return;
			}
		}
	}
}

void Direct3DRMViewportImpl::BuildViewFrustumPlanes()
{
	float aspect = (float) m_renderer->GetWidth() / (float) m_renderer->GetHeight();
	float tanFovX = (m_front > 0.0f) ? (m_field / m_front) : 1.0f;
	float tanFovY = tanFovX / aspect;

	// View-space frustum planes (inward-pointing normals)
	m_frustumPlanes[0] = {Normalize({1.0f, 0.0f, tanFovX}), 0.0f};  // Left:   x + tanFovX * z >= 0
	m_frustumPlanes[1] = {Normalize({-1.0f, 0.0f, tanFovX}), 0.0f}; // Right: -x + tanFovX * z >= 0
	m_frustumPlanes[2] = {Normalize({0.0f, -1.0f, tanFovY}), 0.0f}; // Top:   -y + tanFovY * z >= 0
	m_frustumPlanes[3] = {Normalize({0.0f, 1.0f, tanFovY}), 0.0f};  // Bottom:  y + tanFovY * z >= 0

	// Near and far planes
	m_frustumPlanes[4] = {{0.0f, 0.0f, 1.0f}, -m_front}; // Near (Z >= m_front)
	m_frustumPlanes[5] = {{0.0f, 0.0f, -1.0f}, m_back};  // Far  (Z <= m_back)
}

bool IsMeshInFrustum(Direct3DRMMeshImpl* mesh, const D3DRMMATRIX4D& worldViewMatrix, const Plane* frustumPlanes)
{
	D3DRMBOX box;
	mesh->GetBox(&box);

	if (box.min.x > box.max.x || box.min.y > box.max.y || box.min.z > box.max.z) {
		return true;
	}

	D3DVECTOR boxCorners[8] = {
		{box.min.x, box.min.y, box.min.z},
		{box.min.x, box.min.y, box.max.z},
		{box.min.x, box.max.y, box.min.z},
		{box.min.x, box.max.y, box.max.z},
		{box.max.x, box.min.y, box.min.z},
		{box.max.x, box.min.y, box.max.z},
		{box.max.x, box.max.y, box.min.z},
		{box.max.x, box.max.y, box.max.z},
	};

	float nearZ = -frustumPlanes[4].d;
	float farZ = frustumPlanes[5].d;
	float minZ = 1e9f, maxZ = -1e9f;
	bool straddlesNear = false;

	for (D3DVECTOR& corner : boxCorners) {
		corner = TransformPoint(corner, worldViewMatrix);
		if (corner.z < minZ) minZ = corner.z;
		if (corner.z > maxZ) maxZ = corner.z;
		if (corner.z <= nearZ) {
			straddlesNear = true;
		}
	}

	// 1. Fast Near and Far plane test
	if (maxZ < nearZ || minZ > farZ) {
		return false;
	}

	// 2. Test side planes (0..3) ONLY if the box does not straddle behind the near plane.
	if (!straddlesNear) {
		for (int i = 0; i < 4; ++i) {
			const Plane& plane = frustumPlanes[i];
			int out = 0;
			for (int j = 0; j < 8; ++j) {
				const D3DVECTOR& corner = boxCorners[j];
				float dist = plane.normal.x * corner.x + plane.normal.y * corner.y + plane.normal.z * corner.z;
				if (dist < 0.0f) {
					++out;
				} else {
					break; // At least one corner inside, cannot be outside this plane
				}
			}
			if (out == 8) {
				return false;
			}
		}
	}

	return true;
}

inline D3DRMVECTOR4D TransformPoint4(const D3DRMVECTOR4D& p, const D3DRMMATRIX4D& m)
{
	return {
		p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + p.w * m[3][0],
		p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + p.w * m[3][1],
		p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + p.w * m[3][2],
		p.x * m[0][3] + p.y * m[1][3] + p.z * m[2][3] + p.w * m[3][3]
	};
}

void Direct3DRMViewportImpl::CollectMeshesFromFrame(IDirect3DRMFrame* frame, D3DRMMATRIX4D parentMatrix)
{
	Direct3DRMFrameImpl* frameImpl = static_cast<Direct3DRMFrameImpl*>(frame);
	D3DRMMATRIX4D localMatrix;
	memcpy(localMatrix, frameImpl->m_transform, sizeof(D3DRMMATRIX4D));

	D3DRMMATRIX4D worldMatrix;
	MultiplyMatrix(worldMatrix, parentMatrix, localMatrix);

	if (!frameImpl->m_visuals) {
		return;
	}

	const auto& visualItems = frameImpl->m_visuals->GetItems();
	for (auto* visual : visualItems) {
		if (!visual) {
			continue;
		}

		IDirect3DRMFrame* childFrame = visual->AsFrame();
		if (childFrame) {
			CollectMeshesFromFrame(childFrame, worldMatrix);
			continue;
		}

		IDirect3DRMMesh* meshInterface = visual->AsMesh();
		if (meshInterface) {
			Direct3DRMMeshImpl* mesh = static_cast<Direct3DRMMeshImpl*>(meshInterface);
			D3DRMMATRIX4D modelViewMatrix;
			MultiplyMatrix(modelViewMatrix, worldMatrix, m_viewMatrix);

			// Tiered distance culling
			D3DRMBOX box;
			mesh->GetBox(&box);
			bool isValidBox = (box.min.x <= box.max.x && box.min.y <= box.max.y && box.min.z <= box.max.z);
			if (isValidBox) {
				float extentX = box.max.x - box.min.x;
				float extentY = box.max.y - box.min.y;
				float extentZ = box.max.z - box.min.z;
				float sizeSq = extentX * extentX + extentY * extentY + extentZ * extentZ;
				float vx = modelViewMatrix[3][0];
				float vy = modelViewMatrix[3][1];
				float vz = modelViewMatrix[3][2];
				float distSq = vx * vx + vy * vy + vz * vz;

#ifdef __DREAMCAST__
				// Tier 1: Tiny parts / accessories / small bricks (size < 6): cull beyond 40
				if (sizeSq < (6.0f * 6.0f) && distSq > (40.0f * 40.0f)) {
					continue;
				}
				// Tier 2: Small props, flowers, minifig parts (size < 18): cull beyond 70
				if (sizeSq < (18.0f * 18.0f) && distSq > (70.0f * 70.0f)) {
					continue;
				}
				// Tier 3: Medium scenery, trees, streetlamps, vehicles (size < 40): cull beyond 120
				if (sizeSq < (40.0f * 40.0f) && distSq > (120.0f * 120.0f)) {
					continue;
				}
				// Tier 4: Large scenery / buildings (size < 120): cull beyond 220
				if (sizeSq < (120.0f * 120.0f) && distSq > (220.0f * 220.0f)) {
					continue;
				}
				// Island terrain meshes have size > 150-200, so they are never distance-culled!
#else
				if (sizeSq < (30.0f * 30.0f) && distSq > (100.0f * 100.0f)) {
					continue;
				}
#endif
			}

			if (IsMeshInFrustum(mesh, modelViewMatrix, m_frustumPlanes)) {
				DWORD groupCount = mesh->GetGroupCount();
				for (DWORD gi = 0; gi < groupCount; ++gi) {
					const MeshGroup& meshGroup = mesh->GetGroup(gi);

					Appearance appearance = {
						meshGroup.color,
						meshGroup.material ? meshGroup.material->GetPower() : 0.0f,
						meshGroup.texture ? m_renderer->GetTextureId(meshGroup.texture) : NO_TEXTURE_ID,
						meshGroup.quality == D3DRMRENDER_FLAT || meshGroup.quality == D3DRMRENDER_UNLITFLAT
					};

					static const Matrix3x3 dummyNormalMatrix = {};
					if (appearance.color.unused != 255) {
						m_deferredDraws.push_back(
							{m_renderer->GetMeshId(mesh, &meshGroup),
							 {},
							 {},
							 {},
							 appearance,
							 modelViewMatrix[3][2]}
						);
						memcpy(m_deferredDraws.back().modelViewMatrix, modelViewMatrix, sizeof(D3DRMMATRIX4D));
						memcpy(m_deferredDraws.back().worldMatrix, worldMatrix, sizeof(D3DRMMATRIX4D));
					}
					else {
						m_renderer->SubmitDraw(
							m_renderer->GetMeshId(mesh, &meshGroup),
							modelViewMatrix,
							worldMatrix,
							m_viewMatrix,
							dummyNormalMatrix,
							appearance
						);
					}
				}
			}
		}
	}
}

HRESULT Direct3DRMViewportImpl::RenderScene()
{
	m_backgroundColor = static_cast<Direct3DRMFrameImpl*>(m_rootFrame)->m_backgroundColor;

	// Compute view-projection matrix
	D3DRMMATRIX4D cameraWorld;
	ComputeFrameWorldMatrix(m_camera, cameraWorld);
	D3DRMMatrixInvertOrthogonal(m_viewMatrix, cameraWorld);
	D3DRMMatrixMultiply(m_viewProjectionwMatrix, m_viewMatrix, m_projectionMatrix);

	D3DRMMATRIX4D identity = {{1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}};

	std::vector<SceneLight> lights;
	CollectLightsFromFrame(m_rootFrame, identity, lights);
	m_renderer->PushLights(lights.data(), lights.size());
	HRESULT status = m_renderer->BeginFrame();
	if (status != DD_OK) {
		return status;
	}

	BuildViewFrustumPlanes();
	m_renderer->SetFrustumPlanes(m_frustumPlanes);

	CollectMeshesFromFrame(m_rootFrame, identity);

	if (!m_deferredDraws.empty()) {
		std::sort(
			m_deferredDraws.begin(),
			m_deferredDraws.end(),
			[](const DeferredDrawCommand& a, const DeferredDrawCommand& b) { return a.depth > b.depth; }
		);
		m_renderer->EnableTransparency();
		for (const DeferredDrawCommand& cmd : m_deferredDraws) {
			m_renderer->SubmitDraw(
				cmd.meshId,
				cmd.modelViewMatrix,
				cmd.worldMatrix,
				m_viewMatrix,
				cmd.normalMatrix,
				cmd.appearance
			);
		}
		m_deferredDraws.clear();
	}

	return m_renderer->FinalizeFrame();
}

HRESULT Direct3DRMViewportImpl::Render(IDirect3DRMFrame* rootFrame)
{
	if (!m_renderer) {
		return DDERR_GENERIC;
	}
	m_rootFrame = rootFrame;
	return RenderScene();
}

HRESULT Direct3DRMViewportImpl::ForceUpdate(int x, int y, int w, int h)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::Clear()
{
	if (!m_renderer) {
		return DDERR_GENERIC;
	}

	uint8_t r = (m_backgroundColor >> 16) & 0xFF;
	uint8_t g = (m_backgroundColor >> 8) & 0xFF;
	uint8_t b = m_backgroundColor & 0xFF;
	m_renderer->Clear(r / 255.0f, g / 255.0f, b / 255.0f);

	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::SetCamera(IDirect3DRMFrame* camera)
{
	if (camera) {
		camera->AddRef();
	}
	if (m_camera) {
		m_camera->Release();
	}
	m_camera = camera;
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::GetCamera(IDirect3DRMFrame** camera)
{
	if (m_camera) {
		m_camera->AddRef();
	}
	*camera = m_camera;
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::SetProjection(D3DRMPROJECTIONTYPE type)
{
	return DD_OK;
}

D3DRMPROJECTIONTYPE Direct3DRMViewportImpl::GetProjection()
{
	return D3DRMPROJECTIONTYPE::PERSPECTIVE;
}

HRESULT Direct3DRMViewportImpl::SetFront(D3DVALUE z)
{
	m_front = z;
	UpdateProjectionMatrix();
	return DD_OK;
}

D3DVALUE Direct3DRMViewportImpl::GetFront()
{
	return m_front;
}

HRESULT Direct3DRMViewportImpl::SetBack(D3DVALUE z)
{
	m_back = z;
	UpdateProjectionMatrix();
	return DD_OK;
}

D3DVALUE Direct3DRMViewportImpl::GetBack()
{
	return m_back;
}

HRESULT Direct3DRMViewportImpl::SetField(D3DVALUE field)
{
	m_field = field;
	UpdateProjectionMatrix();
	return DD_OK;
}

void Direct3DRMViewportImpl::UpdateProjectionMatrix()
{
	float virtualAspect = (float) m_virtualWidth / (float) m_virtualHeight;
	float windowAspect = (float) m_renderer->GetWidth() / (float) m_renderer->GetHeight();

	float base_f = m_front / m_field;
	float f_v = base_f * virtualAspect;
	float f_h = base_f;
	if (windowAspect >= virtualAspect) {
		f_h *= virtualAspect / windowAspect;
	}
	else {
		f_v *= windowAspect / virtualAspect;
	}

	float depth = m_back - m_front;

	D3DRMMATRIX4D projection = {
		{f_h, 0, 0, 0},
		{0, f_v, 0, 0},
		{0, 0, m_back / depth, 1},
		{0, 0, (-m_front * m_back) / depth, 0},
	};
	memcpy(m_projectionMatrix, projection, sizeof(D3DRMMATRIX4D));

	m_renderer->SetProjection(projection, m_front, m_back);

	D3DRMMATRIX4D inverseProjectionMatrix = {
		{1.0f / f_h, 0, 0, 0},
		{0, 1.0f / f_v, 0, 0},
		{0, 0, 0, depth / (-m_front * m_back)},
		{0, 0, 1, -(m_back / depth) * depth / (-m_front * m_back)},
	};
	memcpy(m_inverseProjectionMatrix, inverseProjectionMatrix, sizeof(D3DRMMATRIX4D));
}

D3DVALUE Direct3DRMViewportImpl::GetField()
{
	return m_field;
}

inline float FromNDC(float ndcCoord, float dim)
{
	return (ndcCoord * 0.5f + 0.5f) * dim;
}

inline void MultiplyMatrixVec4(D3DRMVECTOR4D& out, const D3DRMMATRIX4D& mat, const D3DRMVECTOR4D& vec)
{
	out.x = mat[0][0] * vec.x + mat[1][0] * vec.y + mat[2][0] * vec.z + mat[3][0] * vec.w;
	out.y = mat[0][1] * vec.x + mat[1][1] * vec.y + mat[2][1] * vec.z + mat[3][1] * vec.w;
	out.z = mat[0][2] * vec.x + mat[1][2] * vec.y + mat[2][2] * vec.z + mat[3][2] * vec.w;
	out.w = mat[0][3] * vec.x + mat[1][3] * vec.y + mat[2][3] * vec.z + mat[3][3] * vec.w;
}

HRESULT Direct3DRMViewportImpl::Transform(D3DRMVECTOR4D* screen, D3DVECTOR* world)
{
	D3DRMVECTOR4D worldVec = {world->x, world->y, world->z, 1.0f};
	D3DRMVECTOR4D viewVec, projVec;

	MultiplyMatrixVec4(viewVec, m_viewMatrix, worldVec);
	MultiplyMatrixVec4(projVec, m_projectionMatrix, viewVec);

	*screen = projVec;

	float invW = 1.0f / projVec.w;
	float ndcX = projVec.x * invW;
	float ndcY = projVec.y * invW;

	screen->x = FromNDC(ndcX, m_virtualWidth);
	screen->y = FromNDC(-ndcY, m_virtualHeight); // Y-flip

	// Undo perspective divide for screen-space coords
	screen->x *= projVec.z;
	screen->y *= projVec.w;

	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::InverseTransform(D3DVECTOR* world, D3DRMVECTOR4D* screen)
{
	// Convert to screen coordinates
	float screenX = screen->x / screen->z;
	float screenY = screen->y / screen->w;

	// Convert screen coordinates to NDC
	float ndcX = screenX / m_virtualWidth * 2.0f - 1.0f;
	float ndcY = 1.0f - (screenY / m_virtualHeight) * 2.0f;

	D3DRMVECTOR4D clipVec = {ndcX * screen->w, ndcY * screen->w, screen->z, screen->w};

	D3DRMVECTOR4D viewVec;
	MultiplyMatrixVec4(viewVec, m_inverseProjectionMatrix, clipVec);

	D3DRMMATRIX4D inverseViewMatrix;
	D3DRMMatrixInvertOrthogonal(inverseViewMatrix, m_viewMatrix);

	D3DRMVECTOR4D worldVec;
	MultiplyMatrixVec4(worldVec, inverseViewMatrix, viewVec);

	// Perspective divide
	if (worldVec.w != 0.0f) {
		world->x = worldVec.x / worldVec.w;
		world->y = worldVec.y / worldVec.w;
		world->z = worldVec.z / worldVec.w;
	}
	else {
		world->x = worldVec.x;
		world->y = worldVec.y;
		world->z = worldVec.z;
	}

	return DD_OK;
}

struct Ray {
	D3DVECTOR origin;
	D3DVECTOR direction;
};

// Ray-box intersection: slab method
bool RayIntersectsBox(const Ray& ray, const D3DRMBOX& box, float& outT)
{
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; ++i) {
		float origin = (&ray.origin.x)[i];
		float dir = (&ray.direction.x)[i];
		float minB = (&box.min.x)[i];
		float maxB = (&box.max.x)[i];

		if (fabs(dir) < 1e-6f) {
			if (origin < minB || origin > maxB) {
				return false;
			}
		}
		else {
			float invD = 1.0f / dir;
			float t1 = (minB - origin) * invD;
			float t2 = (maxB - origin) * invD;
			if (t1 > t2) {
				std::swap(t1, t2);
			}
			if (t1 > tmin) {
				tmin = t1;
			}
			if (t2 < tmax) {
				tmax = t2;
			}
			if (tmin > tmax) {
				return false;
			}
			if (tmax < 0) {
				return false;
			}
		}
	}

	outT = tmin >= 0 ? tmin : tmax; // closest positive hit
	return true;
}

// Convert screen (x,y) in viewport to picking ray in world space
Ray BuildPickingRay(
	float x,
	float y,
	int width,
	int height,
	IDirect3DRMFrame* camera,
	float front,
	float back,
	float field,
	float aspect
)
{
	float nx = (2.0f * x) / width - 1.0f;
	float ny = 1.0f - (2.0f * y) / height;

	float f = front / field;

	// Ray in view space at near plane:
	D3DVECTOR rayDirView = {nx / f, ny / (f * aspect), 1.0f};

	// Normalize ray direction
	float len = sqrt(DotProduct(rayDirView, rayDirView));
	rayDirView = Normalize(rayDirView);

	// Compute camera world matrix and invert it to get view->world
	D3DRMMATRIX4D cameraWorld;
	ComputeFrameWorldMatrix(camera, cameraWorld);

	// Transform ray origin (camera position) and direction to world space
	D3DVECTOR rayOriginWorld = {cameraWorld[3][0], cameraWorld[3][1], cameraWorld[3][2]};
	// Multiply direction by rotation part of matrix only (3x3 upper-left)
	D3DVECTOR rayDirWorld = {
		rayDirView.x * cameraWorld[0][0] + rayDirView.y * cameraWorld[1][0] + rayDirView.z * cameraWorld[2][0],
		rayDirView.x * cameraWorld[0][1] + rayDirView.y * cameraWorld[1][1] + rayDirView.z * cameraWorld[2][1],
		rayDirView.x * cameraWorld[0][2] + rayDirView.y * cameraWorld[1][2] + rayDirView.z * cameraWorld[2][2]
	};

	len = sqrt(rayDirWorld.x * rayDirWorld.x + rayDirWorld.y * rayDirWorld.y + rayDirWorld.z * rayDirWorld.z);
	rayDirWorld = Normalize(rayDirWorld);

	return Ray{rayOriginWorld, rayDirWorld};
}

bool RayIntersectsTriangle(
	const Ray& ray,
	const D3DVECTOR& v0,
	const D3DVECTOR& v1,
	const D3DVECTOR& v2,
	float& outDist
)
{
	const float EPSILON = 1e-6f;
	D3DVECTOR edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
	D3DVECTOR edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

	D3DVECTOR h = CrossProduct(ray.direction, edge2);
	float a = DotProduct(edge1, h);
	if (fabs(a) < EPSILON) {
		return false;
	}

	float f = 1.0f / a;
	D3DVECTOR s = {ray.origin.x - v0.x, ray.origin.y - v0.y, ray.origin.z - v0.z};
	float u = f * DotProduct(s, h);
	if (u < 0.0f || u > 1.0f) {
		return false;
	}

	D3DVECTOR q = CrossProduct(s, edge1);
	float v = f * DotProduct(ray.direction, q);
	if (v < 0.0f || u + v > 1.0f) {
		return false;
	}

	float t = f * DotProduct(edge2, q);
	if (t > EPSILON) {
		outDist = t;
		return true;
	}
	return false;
}

bool RayIntersectsMeshTriangles(
	const Ray& ray,
	Direct3DRMMeshImpl& mesh,
	const D3DRMMATRIX4D& worldMatrix,
	float& outDistance
)
{
	DWORD groupCount = mesh.GetGroupCount();
	for (DWORD gi = 0; gi < groupCount; ++gi) {
		const MeshGroup& meshGroup = mesh.GetGroup(gi);

		// Iterate over each face and do ray-triangle tests
		for (DWORD fi = 0; fi < meshGroup.indices.size(); fi += 3) {
			DWORD i0 = meshGroup.indices[fi + 0];
			DWORD i1 = meshGroup.indices[fi + 1];
			DWORD i2 = meshGroup.indices[fi + 2];

			// Transform vertices to world space
			D3DVECTOR tri[3];
			for (int j = 0; j < 3; ++j) {
				const D3DVECTOR& v = meshGroup.vertices[(j == 0 ? i0 : (j == 1 ? i1 : i2))].position;
				tri[j] = TransformPoint(v, worldMatrix);
			}

			float dist;
			if (RayIntersectsTriangle(ray, tri[0], tri[1], tri[2], dist)) {
				if (dist < outDistance) {
					outDistance = dist;
				}
				return true;
			}
		}
	}
	return false;
}

inline D3DVECTOR TransformVector(const D3DRMMATRIX4D& mat, const D3DVECTOR& vec)
{
	return {
		vec.x * mat[0][0] + vec.y * mat[1][0] + vec.z * mat[2][0] + mat[3][0],
		vec.x * mat[0][1] + vec.y * mat[1][1] + vec.z * mat[2][1] + mat[3][1],
		vec.x * mat[0][2] + vec.y * mat[1][2] + vec.z * mat[2][2] + mat[3][2]
	};
}

D3DRMBOX ComputeTransformedAABB(const D3DRMBOX& box, const D3DRMMATRIX4D& mat)
{
	D3DVECTOR corners[8] = {
		{box.min.x, box.min.y, box.min.z},
		{box.min.x, box.min.y, box.max.z},
		{box.min.x, box.max.y, box.min.z},
		{box.min.x, box.max.y, box.max.z},
		{box.max.x, box.min.y, box.min.z},
		{box.max.x, box.min.y, box.max.z},
		{box.max.x, box.max.y, box.min.z},
		{box.max.x, box.max.y, box.max.z}
	};

	D3DVECTOR transformed = TransformVector(mat, corners[0]);
	D3DRMBOX worldBox = {transformed, transformed};

	for (int i = 1; i < 8; ++i) {
		D3DVECTOR v = TransformVector(mat, corners[i]);
		worldBox.min.x = std::min(worldBox.min.x, v.x);
		worldBox.min.y = std::min(worldBox.min.y, v.y);
		worldBox.min.z = std::min(worldBox.min.z, v.z);
		worldBox.max.x = std::max(worldBox.max.x, v.x);
		worldBox.max.y = std::max(worldBox.max.y, v.y);
		worldBox.max.z = std::max(worldBox.max.z, v.z);
	}
	return worldBox;
}

HRESULT Direct3DRMViewportImpl::Pick(float x, float y, LPDIRECT3DRMPICKEDARRAY* pickedArray)
{
	if (!m_rootFrame) {
		return DDERR_GENERIC;
	}

	std::vector<PickRecord> hits;

	Ray pickRay = BuildPickingRay(
		x,
		y,
		m_virtualWidth,
		m_virtualHeight,
		m_camera,
		m_front,
		m_back,
		m_field,
		(float) m_virtualWidth / (float) m_virtualHeight
	);

	std::function<void(IDirect3DRMFrame*, std::vector<IDirect3DRMFrame*>&)> recurse;
	recurse = [&](IDirect3DRMFrame* frame, std::vector<IDirect3DRMFrame*>& path) {
		path.push_back(frame);

		IDirect3DRMVisualArray* visuals = nullptr;
		frame->GetVisuals(&visuals);
		DWORD count = visuals->GetSize();
		for (DWORD i = 0; i < count; ++i) {
			IDirect3DRMVisual* visual = nullptr;
			visuals->GetElement(i, &visual);

			IDirect3DRMFrame* subFrame = nullptr;
			visual->QueryInterface(IID_IDirect3DRMFrame, (void**) &subFrame);
			if (subFrame) {
				recurse(subFrame, path);
				subFrame->Release();
				visual->Release();
				continue;
			}

			Direct3DRMMeshImpl* mesh = nullptr;
			visual->QueryInterface(IID_IDirect3DRMMesh, (void**) &mesh);
			if (mesh) {
				D3DRMBOX box;
				mesh->GetBox(&box);
				// Transform box corners to world space
				D3DRMMATRIX4D worldMatrix;
				ComputeFrameWorldMatrix(frame, worldMatrix);
				D3DRMBOX worldBox = ComputeTransformedAABB(box, worldMatrix);

				float boxDist = FLT_MAX;

				// Check box first as an optimization
				if (RayIntersectsBox(pickRay, worldBox, boxDist)) {
					float meshDist = FLT_MAX;

					if (RayIntersectsMeshTriangles(pickRay, *mesh, worldMatrix, meshDist)) {
						auto* arr = new Direct3DRMFrameArrayImpl();
						for (IDirect3DRMFrame* f : path) {
							arr->AddElement(f);
						}

						PickRecord rec = {visual, arr, {meshDist}};
						hits.push_back(rec);
					}
				}
				mesh->Release();
			}
			visual->Release();
		}
		visuals->Release();
		path.pop_back(); // Pop after recursion
	};

	std::vector<IDirect3DRMFrame*> framePath;
	recurse(m_rootFrame, framePath);

	std::sort(hits.begin(), hits.end(), [](const PickRecord& a, const PickRecord& b) {
		return a.desc.dist < b.desc.dist;
	});

	*pickedArray = new Direct3DRMPickedArrayImpl(hits.data(), hits.size());
	for (PickRecord& hit : hits) {
		hit.frameArray->Release();
	}

	return D3DRM_OK;
}

void Direct3DRMViewportImpl::CloseDevice()
{
	m_renderer = nullptr;
}
