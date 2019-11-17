#pragma once

#include "PrimeEngine/Math/Vector3.h"
#include "PrimeEngine/Math/Vector4.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Scene/CameraManager.h"
#include "PrimeEngine/MemoryManagement/Handle.h"

using namespace PE::Components;
using namespace PE::Events;
using namespace PE;

struct Ray
{
	Vector3 origin;
	Vector3 dir;
};

struct Sphere
{
	Vector3 origin;
	float radius;
};

struct Plane 
{
	float d = 0;
	Vector3 normal = { 0, 1, 0 };
};

Ray unprojectNDC(float x, float y)
{
	Vector4 rayClipSpace = { x, y, 1.0f, 1.0f };
	CameraManager *cameraManager = CameraManager::Instance();
	Camera *activeCamera = cameraManager->getActiveCamera();
	Handle h_cameraSceneNode = activeCamera->m_hCameraSceneNode;
	CameraSceneNode *cameraSceneNode = h_cameraSceneNode.getObject<CameraSceneNode>();
	Matrix4x4 inverseProject = cameraSceneNode->m_viewToProjectedTransform.inverse();
	Vector4 rayCameraSpace = inverseProject * rayClipSpace;
	rayCameraSpace.m_z = 1.0f;
	rayCameraSpace.m_w = 0.0f;
	Matrix4x4 inverseWorldToView = cameraSceneNode->m_worldToViewTransform.inverse();
	Vector4 rayWorldSpace = inverseWorldToView * rayCameraSpace;
	Vector3 rayWorldSpace3D(rayWorldSpace.m_x, rayWorldSpace.m_y, rayWorldSpace.m_z);
	rayWorldSpace3D.normalize();
	Vector3 rayOrigin = cameraSceneNode->m_base.getPos();
	return { rayOrigin, rayWorldSpace3D };
}

bool rayPlaneIntersection(Ray &ray, Plane &plane, Vector3 &intersectionPoint)
{
	Vector3 a = ray.origin;
	Vector3 b = ray.origin + ray.dir * 10000;

	// Compute the t value for the directed line ab intersecting the plane
	Vector3 ab = b - a;
	float t = ((plane.d - plane.normal.dotProduct(a)) / plane.normal.dotProduct(ab));
	// If t in [0..1] compute and return intersection point
	if (t >= 0.0f && t <= 1.0f)
	{
		intersectionPoint = a + t * ab;
		return true;
	}
	else
	{
		intersectionPoint.m_x = -1;
		intersectionPoint.m_y = -1;
		intersectionPoint.m_z = -1;
		return false;
	}

	return false;
}

bool raySphereIntersection(Ray &ray, Sphere &sphere)
{
	// t^2 + 2(m dot d)t + (m dot m) - r^2 = 0
	// m = rayOrigin - sphereCenter (distanceFromCenter)
	// d = rayDirection
	Vector3 centerToOrigin = ray.origin - sphere.origin;
	float rSquared = sphere.radius * sphere.radius;
	float b = centerToOrigin.dotProduct(ray.dir);
	float c = centerToOrigin.dotProduct(centerToOrigin) - rSquared;

	// if outside sphere, and pointing away from it
	if (b > 0.0f &&  c > 0.0f)
	{
		return false;
	}

	// note b should have had a 2 so b^2 makes this a 4; therefore, discriminant is b^2 - c
	float discriminant = b*b - c;
	if (discriminant < 0.0f)
	{
		return false;
	}

	return true;
}