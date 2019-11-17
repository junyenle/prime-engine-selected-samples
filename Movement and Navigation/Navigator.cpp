#include "Navigator.h"

bool Navigator::lineSegmentIntersection(Vector3 a1, Vector3 a2, Vector3 b1, Vector3 b2, int type)
{
	if (type == 3)
	{
		if (b1 == a1 || b1 == a2 || b2 == a1 || b2 == a2)
		{
			return false;
		}
	}
	if (type == 2)
	{
		if (b1 == a1 || b1 == a2)
		{
			return false;
		}
	}
	else if (type == 1)
	{
		if (b2 == a1 || b2 == a2)
		{
			return false;
		}
	}
	Vector3 b = a2 - a1;
	Vector3 d = b2 - b1;
	float bDotDPerp = b.m_x * d.m_z - b.m_z * d.m_x;

	// if b dot d == 0, it means the lines are parallel so have infinite intersection points
	if (bDotDPerp == 0)
		return false;

	Vector3 c = b1 - a1;
	float t = (c.m_x * d.m_z - c.m_z * d.m_x) / bDotDPerp;
	if (t < 0 || t > 1)
		return false;

	float u = (c.m_x * b.m_z - c.m_z * b.m_x) / bDotDPerp;
	if (u < 0 || u > 1)
		return false;

	return true;
}

void Navigator::initStaticPolys()
{
	numPolys = 0;

	staticNodes[0].pos = Vector3(18.1f, 0.0f, 11.3f);
	staticNodes[1].pos = Vector3(11.5f, 0.0f, 11.3f);
	staticNodes[2].pos = Vector3(18.1f, 0.0f, 13.4f);
	staticNodes[3].pos = Vector3(11.5f, 0.0f, 13.4f);
	staticPolys[0].nodes[0] = &staticNodes[0];
	staticPolys[0].nodes[1] = &staticNodes[1];
	staticPolys[0].nodes[2] = &staticNodes[3];
	staticPolys[0].nodes[3] = &staticNodes[2];
	staticPolys[0].sides = 4;

	numPolys += 1;
}

void Navigator::initNodes()
{
	for (int i = 0; i < navNodes; i++)
	{
		staticNodes[i].successors.clear();
	}

	numPolys = 0;

	staticNodes[0].pos = Vector3(18.1f, 0.0f, 11.3f);
	staticNodes[1].pos = Vector3(11.5f, 0.0f, 11.3f);
	staticNodes[2].pos = Vector3(18.1f, 0.0f, 13.4f);
	staticNodes[3].pos = Vector3(11.5f, 0.0f, 13.4f);
	staticPolys[0].nodes[0] = &staticNodes[0];
	staticPolys[0].nodes[1] = &staticNodes[1];
	staticPolys[0].nodes[2] = &staticNodes[3];
	staticPolys[0].nodes[3] = &staticNodes[2];
	staticPolys[0].sides = 4;
	numPolys += 1;

	//staticNodes[0].pos = Vector3(-3.2f, 0.0f, 4.2f);
	//staticNodes[1].pos = Vector3(-1.0f, 0.0f, 4.1f);
	//staticNodes[2].pos = Vector3(-3.3f, 0.0f, -1.0f);
	//staticNodes[3].pos = Vector3(-1.3f, 0.0f, -1.2f);
	//staticPolys[0].nodes[0] = &staticNodes[0];
	//staticPolys[0].nodes[1] = &staticNodes[1];
	//staticPolys[0].nodes[2] = &staticNodes[3];
	//staticPolys[0].nodes[3] = &staticNodes[2];
	//staticPolys[0].sides = 4;
	//numPolys += 1;

	//staticNodes[4].pos = Vector3(2.1f, 0.0f, 1.6f);
	//staticNodes[5].pos = Vector3(4.2f, 0.0f, 1.6f);
	//staticNodes[6].pos = Vector3(2.3f, 0.0f, -3.9f);
	//staticNodes[7].pos = Vector3(4.6f, 0.0f, -3.8f);
	//staticPolys[1].nodes[0] = &staticNodes[4];
	//staticPolys[1].nodes[1] = &staticNodes[5];
	//staticPolys[1].nodes[2] = &staticNodes[7];
	//staticPolys[1].nodes[3] = &staticNodes[6];
	//staticPolys[1].sides = 4;
	//numPolys += 1;

	navNodes = 0;
	for (Poly poly : staticPolys)
	{
		navNodes += poly.sides;
	}

	// for neighboring nodes within each poly
	for (int i = 0; i < numPolys; i++)
	{
		Poly* thisPoly = &staticPolys[i];
		for (int j = 0; j < thisPoly->sides; j++)
		{
			thisPoly->nodes[j]->successors.push_back(thisPoly->nodes[(j + 1) % thisPoly->sides]);
			int prev = j - 1;
			prev = prev < 0 ? thisPoly->sides - 1 : prev;
			thisPoly->nodes[j]->successors.push_back(thisPoly->nodes[prev]);
		}
	}

	// for each pair of nodes from different polys
	for (int i = 0; i < numPolys; i++)
	{
		Poly* polyA = &staticPolys[i];
		for (int j = 0; j < numPolys; j++)
		{
			Poly* polyB = &staticPolys[j];
			for (int k = 0; k < polyA->sides; k++)
			{
				Node* nodeA = polyA->nodes[k];
				for (int w = 0; w < polyB->sides; w++)
				{
					Node* nodeB = polyB->nodes[w];
					bool direct = true;
					for (Poly poly : staticPolys)
					{
						for (int i = 0; i < poly.sides; i++)
						{
							int nextIndex = i + 1;
							if (i == poly.sides - 1)
							{
								nextIndex = 0;
							}

							Vector3 a = poly.nodes[i]->pos;
							Vector3 b = poly.nodes[nextIndex]->pos;
							if (lineSegmentIntersection(a, b, nodeA->pos, nodeB->pos, 3))
							{
								direct = false;
								break;
							}
						}
						if (!direct)
						{
							break;
						}
					} // end for poly
					if (direct)
					{
						nodeA->successors.push_back(nodeB);
					}
				}
			}
		}
	}

	initialized = true;
}

bool Navigator::isInsideAnyPoly(Vector3 pos)
{
	for (Poly poly : staticPolys)
	{
		if (isInsidePoly(poly, pos))
		{
			return true;
		}
	}
	return false;
}

bool Navigator::isInsidePoly(Poly poly, Vector3 pos)
{
	for (int i = 0; i < poly.sides; i++)
	{
		Vector3 a = poly.nodes[(i + 1) % poly.sides]->pos - poly.nodes[i]->pos;
		Vector3 b;
		if (i == 0)
		{
			b = poly.nodes[poly.sides - 1]->pos - poly.nodes[i]->pos;
		}
		else
		{
			b = poly.nodes[(i - 1) % poly.sides]->pos - poly.nodes[i]->pos;
		}
		Vector3 c = pos - poly.nodes[i]->pos;
		a.normalize(); b.normalize(); c.normalize();
		float ab = a.dotProduct(b);
		float ac = a.dotProduct(c);
		float bc = b.dotProduct(c);
		if (ac < ab || bc < ab)
		{
			return false;
		}
	}
	return true;
}

std::vector<Vector3> Navigator::findPath(Vector3 from, Vector3 to)
{
	initNodes();
	bool direct = true;
	std::vector<Vector3> path;

	/*float minDistToStart = (from - staticNodes[0].pos).lengthSqr();
		float minDistToEnd = (to - staticNodes[0].pos).lengthSqr();
		Node *minNodeToStart = &staticNodes[0];
		Node *minNodeToEnd = &staticNodes[0];

		for (int i = 1; i < 8; i++)
		{
			Node *n = &staticNodes[i];

			float distanceToStart = (from - n->pos).lengthSqr();
			float distanceToEnd = (to - n->pos).lengthSqr();

			if (distanceToStart < minDistToStart)
			{
				minNodeToStart = n;
				minDistToStart = distanceToStart;
			}

			if (distanceToEnd < minDistToEnd)
			{
				minNodeToEnd = n;
				minDistToEnd = distanceToEnd;
			}
		}*/

	Node startNode;
	startNode.pos = from;
	/*minNodeToStart->successors.push_back(&startNode);
	startNode.successors.push_back(minNodeToStart);*/

	Node endNode;
	endNode.pos = to;
	/*endNode.successors.push_back(minNodeToEnd);
	minNodeToEnd->successors.push_back(&endNode);*/
	for (Poly poly : staticPolys)
	{
		if (isInsidePoly(poly, to))
		{
			return path;
		}
	}

	for (Poly poly : staticPolys)
	{
		for (int i = 0; i < poly.sides; i++)
		{
			int nextIndex = i + 1;
			if (i == poly.sides - 1)
			{
				nextIndex = 0;
			}

			Vector3 a = poly.nodes[i]->pos;
			Vector3 b = poly.nodes[nextIndex]->pos;
			if (lineSegmentIntersection(a, b, from, to))
			{
				direct = false;
				break;
			}
		}

		if (!direct)
		{
			break;
		}
	}

	if (direct)
	{
		path.push_back(from);
		path.push_back(to);
		return path;
	}


	for (int j = 0; j < navNodes; j++)
	{
		direct = true;
		Node *currNode = &staticNodes[j];
		for (Poly poly : staticPolys)
		{
			for (int i = 0; i < poly.sides; i++)
			{
				int nextIndex = i + 1;
				if (i == poly.sides - 1)
				{
					nextIndex = 0;
				}

				Vector3 a = poly.nodes[i]->pos;
				Vector3 b = poly.nodes[nextIndex]->pos;
				Vector3 diagDir = poly.nodes[(i + 2) % poly.sides]->pos - poly.nodes[i]->pos;
				diagDir.normalize();
				//Vector3 correctedPos = currNode->pos  - diagDir * 0.5f;
				if (lineSegmentIntersection(a, b, from, currNode->pos, 1))
				{
					direct = false;
					break;
				}
			}

			if (!direct)
			{
				break;
			}
		}

		if (direct)
		{
			currNode->successors.push_back(&startNode);
			startNode.successors.push_back(currNode);
		}
	}

	for (int j = 0; j < navNodes; j++)
	{
		direct = true;
		Node *currNode = &staticNodes[j];
		for (Poly poly : staticPolys)
		{
			for (int i = 0; i < poly.sides; i++)
			{
				int nextIndex = i + 1;
				if (i == poly.sides - 1)
				{
					nextIndex = 0;
				}

				Vector3 a = poly.nodes[i]->pos;
				Vector3 b = poly.nodes[nextIndex]->pos;
				Vector3 diagDir = poly.nodes[(i + 2) % poly.sides]->pos - poly.nodes[i]->pos;
				diagDir.normalize();
				//Vector3 correctedPos = currNode->pos - diagDir * 0.5f;
				if (lineSegmentIntersection(a, b, currNode->pos, to, 2))
				{
					direct = false;
					break;
				}
			}

			if (!direct)
			{
				break;
			}
		}

		if (direct)
		{
			currNode->successors.push_back(&endNode);
			endNode.successors.push_back(currNode);
		}
	}

	// A*
	Node* currentNode = &endNode;
	
	for (int i = 0; i < navNodes; i++)
	{
		staticNodes[i].isClosed = false;
		staticNodes[i].g = 0.0f;
	}

	startNode.isClosed = false;
	startNode.g = 0.0f;

	endNode.isClosed = true;
	endNode.g = 0.0f;

	std::vector<Node *> openSet;

	do
	{
		for (int i = 0; i < currentNode->successors.size(); i++)
		{
			Node* tempTile = currentNode->successors[i];

			if (tempTile->isClosed)
			{
				continue;
			}
			else if (std::find(openSet.begin(), openSet.end(), tempTile) != openSet.end()) // if in open set
			{
				float newG = currentNode->g + (tempTile->pos - currentNode->pos).length();

				if (newG < tempTile->g)
				{
					tempTile->parent = currentNode;
					tempTile->g = newG;
					tempTile->f = tempTile->g + tempTile->h;
				}
			}
			else
			{
				tempTile->parent = currentNode;

				float hValue = (tempTile->pos - startNode.pos).length();
				tempTile->h = hValue;

				float gValue = currentNode->g + (tempTile->pos - currentNode->pos).length();
				tempTile->g = gValue;

				tempTile->f = tempTile->g + tempTile->h;
				openSet.push_back(tempTile);
			}
		}

		if (openSet.empty())
		{
			return path;
		}

		currentNode = openSet[0];
		for (int i = 1; i < openSet.size(); i++)
		{
			if (currentNode->f > openSet[i]->f)
			{
				currentNode = openSet[i];
			}
		}

		openSet.erase(std::find(openSet.begin(), openSet.end(), currentNode));
		currentNode->isClosed = true;
	} while (currentNode != &startNode);

	while (currentNode != &endNode)
	{
		path.push_back(currentNode->pos);
		currentNode = currentNode->parent;
	}

	path.push_back(endNode.pos);
	return path;
}

