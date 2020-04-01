#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct Vertex
	{
		alignas(16) glm::vec3 Position;
		alignas(16) glm::vec3 Normal;
		alignas(16) glm::vec3 Tangent;
		alignas(16) glm::vec2 TexCoord;

		bool operator==(const Vertex& other) const
		{
			return Position == other.Position && Normal == other.Normal && Tangent == other.Tangent && TexCoord == other.TexCoord;
		}

		void CalculateTangent(const Vertex& v1, const Vertex& v2)
		{
			glm::vec3 edge1 = v1.Position - this->Position;
			glm::vec3 edge2 = v2.Position - this->Position;
			glm::vec2 deltaUV1 = v1.TexCoord - this->TexCoord;
			glm::vec2 deltaUV2 = v2.TexCoord - this->TexCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = glm::normalize(tangent);

			this->Tangent = tangent;
		}
	};

	struct Mesh
	{
		~Mesh()
		{
			SAFEDELETEARR(pVertices);
			SAFEDELETEARR(pIndices);
		}

		Vertex* pVertices		= nullptr;
		uint32  NumVertices		= 0;
		uint32* pIndices		= nullptr;
		uint32  NumIndices		= 0;
	};
}

namespace std
{
	template<> struct hash<LambdaEngine::Vertex>
	{
		size_t operator()(LambdaEngine::Vertex const& vertex) const
		{
			return
				((hash<glm::vec3>()(vertex.Position) ^
				 (hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^
				 (hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}
