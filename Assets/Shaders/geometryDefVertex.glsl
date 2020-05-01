#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive : enable

struct SVertex
{
    vec4    Position;
    vec4    Normal;
    vec4    Tangent;
    vec4    TexCoord;
};

struct SInstance
{
    mat3x4  Transform;
    uint    Mask_MeshMaterialIndex;
    uint    SBTRecordOffset_Flags;
    uint    AccelerationStructureHandleTop32;
    uint    AccelerationStructureHandleBottom32;
};

struct SMeshIndexDesc
{
    uint	IndexCount;
    uint	InstanceCount;
    uint	FirstIndex;
    int	    VertexOffset;
    uint	FirstInstance;
    
    uint	MaterialIndex;
};

struct SPerFrameBuffer
{
    mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 ViewInv;
	mat4 ProjectionInv;
	vec4 Position;
	vec4 Right;
	vec4 Up;
};


layout (constant_id = 0) const uint OTHER_TEXTURES_IN_PASS                  = 0;
layout (constant_id = 1) const uint ALLOWED_TEXTURES_PER_DESCRIPTOR_SET     = 256;

layout(binding = 0, set = 1) buffer Vertices            { SVertex val[]; }              b_Vertices;
layout(binding = 1, set = 1) buffer Indices             { uint val[]; }                 b_Indices;
layout(binding = 2, set = 1) buffer Instances           { SInstance val[]; }            b_Instances;
layout(binding = 3, set = 1) buffer MeshIndices         { SMeshIndexDesc val[]; }       b_MeshIndices;
layout(binding = 4, set = 1) uniform PerFrameBuffer     { SPerFrameBuffer val; }        u_PerFrameBuffer;

layout(location = 0) out flat uint out_MaterialIndex;
layout(location = 1) out vec3 out_Normal;
layout(location = 2) out vec3 out_Tangent;
layout(location = 3) out vec3 out_Bitangent;
layout(location = 4) out vec2 out_TexCoord;
layout(location = 5) out vec4 out_Position;

void main()
{
    SInstance instance                          = b_Instances.val[gl_InstanceIndex];
    SVertex vertex                              = b_Vertices.val[gl_VertexIndex];
    SPerFrameBuffer perFrameBuffer              = u_PerFrameBuffer.val;

    uint meshIndexID                            = (instance.Mask_MeshMaterialIndex) & 0x00FFFFFF;
    SMeshIndexDesc meshIndexDesc                = b_MeshIndices.val[meshIndexID];

    mat4 transform;
	transform[0] = vec4(instance.Transform[0]);
	transform[1] = vec4(instance.Transform[1]);
	transform[2] = vec4(instance.Transform[2]);
	transform[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    transform = transpose(transform);

    vec4 worldPosition = transform * vec4(vertex.Position.xyz,  1.0f);

    vec3 normal 	= normalize((transform  * vec4(vertex.Normal.xyz, 0.0f)).xyz);
	vec3 tangent    = normalize((transform  * vec4(vertex.Tangent.xyz, 0.0f)).xyz);
	vec3 bitangent 	= normalize(cross(normal, tangent));
    vec2 texCoord   = vertex.TexCoord.xy;

    uint materialsRenderedPerPass = (ALLOWED_TEXTURES_PER_DESCRIPTOR_SET - OTHER_TEXTURES_IN_PASS) / 5;

    out_MaterialIndex   = meshIndexDesc.MaterialIndex % materialsRenderedPerPass;
	out_Normal 			= normal;
	out_Tangent 		= tangent;
	out_Bitangent 		= bitangent;
	out_TexCoord 		= texCoord;
	out_Position		= worldPosition;

    gl_Position = perFrameBuffer.Projection * perFrameBuffer.View * worldPosition;
}
