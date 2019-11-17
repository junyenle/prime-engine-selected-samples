#pragma once

#define TOTAL_NODES 4 
#define TOTAL_POLYS 1

#include "PrimeEngine/Math/Vector3.h"
#include "PrimeEngine/Math/Vector4.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include <vector>

using namespace PE::Components;
using namespace PE::Events;
using namespace PE;

struct Navigator
{
	struct Node
	{
		Vector3 pos;
		std::vector<Node *> successors;
		bool isClosed;
		float g;
		float f;
		float h;
		Node *parent;
	};

	struct Poly
	{
		Node *nodes[4];
		int sides;
	};

	void initNodes();
	void initStaticPolys();
	bool isInsideAnyPoly(Vector3 pos);
	bool isInsidePoly(Poly poly, Vector3 pos);
	bool lineSegmentIntersection(Vector3 a1, Vector3 a2, Vector3 b1, Vector3 b2, int type = 0);
	std::vector<Vector3> findPath(Vector3 from, Vector3 to);

	Node staticNodes[TOTAL_NODES];
	Poly staticPolys[TOTAL_POLYS];
	bool initialized = 0;
	int navNodes = 0;
	int numPolys = 0;
};



