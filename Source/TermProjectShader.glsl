


-- Vertex

in vec4 Position;
out vec3 vPosition;

void main()
{
    vPosition = Position.xyz;
}

-- TessControl

layout(vertices = 3) out;
in vec3 vPosition[];
out vec3 tcPosition[];
uniform vec3 CameraPosition;
uniform mat4 Projection;
uniform mat4 Modelview;

#define ID gl_InvocationID

float GetTessLevel(float Distance0, float Distance1)                                            
{    
    //Utilizing an exponential function rather than a linear one to more easily demonstrate tessellation effect                                                                                    
    float AvgDistance = clamp((Distance0 + Distance1) / 2.0,0,100);                                          
	float res = sin((AvgDistance/200f)*3.145);
	return clamp((1.0f/(res*res*res)) , 1, 2048);                                                                                                                                                                   
}                                                                                               

void main()
{

    tcPosition[ID] = vPosition[ID];
    if (ID == 0) {
		
		//Get Distance from cameraposition to vertex
		float EyeToVertexDistance0 = distance(CameraPosition,vPosition[0]); 
		float EyeToVertexDistance1 = distance(CameraPosition,vPosition[1]); 
		float EyeToVertexDistance2 = distance(CameraPosition,vPosition[2]); 
		
		//Get the tessellation level for an edge based on the average distance of its vertecies
		float e0 = GetTessLevel(EyeToVertexDistance1,EyeToVertexDistance2);
		float e1 = GetTessLevel(EyeToVertexDistance2,EyeToVertexDistance0);
		float e2 = GetTessLevel(EyeToVertexDistance0,EyeToVertexDistance1);
		
		gl_TessLevelInner[0] = mix(e1, e2, 0.5);
        gl_TessLevelOuter[0] = e0;
        gl_TessLevelOuter[1] = e1;
        gl_TessLevelOuter[2] = e2;
    }
}

-- TessEval

layout(triangles, fractional_odd_spacing , cw) in;
in vec3 tcPosition[];
out vec3 tePosition;
out vec3 tePatchDistance;
uniform mat4 Projection;
uniform mat4 Modelview;
uniform int Normalizetess;

void main()
{
	vec3 p0 = gl_TessCoord.x * tcPosition[0];
	vec3 p1 = gl_TessCoord.y * tcPosition[1];
	vec3 p2 = gl_TessCoord.z * tcPosition[2];

	tePatchDistance = gl_TessCoord;
	
	//Choose between simple subdivide and a normalized position
	if(Normalizetess == 1){
		tePosition = normalize(p0 + p1 + p2)*1.5;}
	if(Normalizetess == 0){
		tePosition = (p0 + p1 + p2)*1.5;}
	
	gl_Position = Projection * Modelview * vec4(tePosition, 1);
	
}

-- Geometry
//Based on code by Philip Rideout

uniform mat4 Modelview;
uniform mat3 NormalMatrix;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 tePosition[3];
in vec3 tePatchDistance[3];
out vec3 gFacetNormal;
out vec3 gPatchDistance;
out vec3 gTriDistance;

void main()
{
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = NormalMatrix * normalize(cross(A, B));
    
    gPatchDistance = tePatchDistance[0];
    gTriDistance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[1];
    gTriDistance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[2];
    gTriDistance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}

-- Fragment
//Based on code by Philip Rideout

out vec4 FragColor;
in vec3 gFacetNormal;
in vec3 gTriDistance;
in vec3 gPatchDistance;
in float gPrimitive;
uniform vec3 LightPosition;
uniform vec3 DiffuseMaterial;
uniform vec3 AmbientMaterial;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}

void main()
{
    vec3 N = normalize(gFacetNormal);
    vec3 L = LightPosition;
    float df = abs(dot(N, L));
    vec3 color = AmbientMaterial + df * DiffuseMaterial;

    float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
    float d2 = min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z);
    color = amplify(d1, 40, -0.5) * amplify(d2, 60, -0.5) * color;

    FragColor = vec4(color, 1.0);
}

