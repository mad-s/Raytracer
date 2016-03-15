

enum Material {
	DIFFUSE,
	REFLECTIVE,
	REFRACTIVE
};

struct Sphere {
	float3 color;
	float3 emission;
	float3 pos;
	float radius;
	enum Material material;
};

struct Ray {
	float3 origin;
	float3 dir;
};


#define delta 1e-4f

#define half_cos cos
#define half_sin sin
#define half_sqrt sqrt
#define fast_normalize normalize

float rand(__private uint2* state){
	const float invMaxInt = 1.0f/4294967296.0f;
	uint x = (*state).x * 17 + (*state).y * 13123;
	(*state).x = (x<<13)^x;
	(*state).y ^= (x<<7);

	return convert_float(x*(x*x*15731+74323) + 871483)*invMaxInt;
}


float intersectSphere(struct Ray* ray, __constant struct Sphere* sphere){
	float3 v = ray->origin-sphere->pos;
	float vd = dot(v,ray->dir);
	float det = vd*vd-dot(v,v)+sphere->radius*sphere->radius;
	if(det<0)return 0;
	else det=sqrt(det);
	float t = -vd-det;
	if(t>0){
		return t;
	}else{
		t=-vd+det;
		if(t>0){
			return t;
		}else{
			return 0;
		}
	}
}

float3 randHemisphere(float3 normal, __private uint2* state){
	float3 u=fast_normalize(cross(fabs(normal.x)>0.1f?(float3)(0,1,0):(float3)(1,0,0),normal));
	float3 v=cross(normal,u);
	float r1=rand(state)*2*M_PI_F;
	float r2=0.1 + 0.9 * rand(state);
	float r2s=half_sqrt(r2);
	return u*half_cos(r1)*r2s+v*half_sin(r1)*r2s+normal*half_sqrt(1-r2);
	
}

float intersect(struct Ray* ray, __constant struct Sphere* spheres, uint sphereCount, __constant struct Sphere** hitObject){
	float t=INFINITY;

	__constant struct Sphere* hit;
	for(int i=0;i<sphereCount;i++){
		float d = intersectSphere(ray,spheres+i);
		if(d!=0&&d<t){
			t=d;
			hit=spheres+i;
		}
	}
	(*hitObject)=hit;
	return t;
}


float3 raycast(struct Ray* r, __constant struct Sphere* spheres, uint sphereCount, __private uint2* randState){
	unsigned depth = 0;
	float3 radiance=(float3)(0,0,0);
	float3 throughput=(float3)(1,1,1); //how the color has been affected so far
	struct Ray curr = *r;
	__constant struct Sphere* hit;

	for(;depth<4;depth++){
		if(dot(throughput,throughput)<0.001f)return radiance;
		__constant struct Sphere* hit;
		float t = intersect(&curr, spheres, sphereCount, &hit);
		if(t==INFINITY){
			return radiance;
		}
		float3 x = curr.origin+t*curr.dir;
		float3 n = normalize(x-hit->pos);
		radiance+=throughput*hit->emission;
		throughput*=hit->color;
		if(hit->material==DIFFUSE){
			float3 d = randHemisphere(n,randState);
			curr.origin = x+delta*d;
			curr.dir = d;
		} else if(hit->material==REFLECTIVE){
			float3 d = curr.dir-2.0f*n*dot(n,curr.dir);
			curr.origin=x+delta*d;
			curr.dir = d;
		} else if(hit->material==REFRACTIVE){
			float dirn = dot(curr.dir, n);
			bool inside = dirn > 0;
			float refFactor = inside ? 0.5 : 2.0;
			float cosOut2 = 1 - refFactor * refFactor * (1 - dirn * dirn);
			if (cosOut2 < 0) {
				curr.dir = curr.dir - 2.0f * n * dirn;
				curr.origin = x + delta * curr.dir;
			} else {
				float cosOut = sqrt(cosOut2);
				float3 parallel = cross(n, cross(-n, curr.dir));

				curr.dir = refFactor*parallel - (inside ? -1 : 1) * n * cosOut;
				curr.origin = x + delta * curr.dir;
			}
			
		}

		
	}
	return radiance;
}

__kernel void main(__global float4* output, uint w, uint h, uint samples, struct Ray camera, __constant struct Sphere* spheres, uint sphereCount){
	int id = get_global_id(0);
	uint2 state = (uint2)(id,0);
	uint x = id%w;
	uint y = id/w;
	float3 cx=normalize((float3)(camera.dir.z,0.0,-camera.dir.x));
	float3 cy=normalize(cross(cx,camera.dir));
	float3 sum=(float3)(0,0,0);
	float invSamp = 1.0/samples;
	for(int sy = 0; sy<2; sy++){
		for(int sx = 0; sx<2; sx++){
			float3 subpix_color=(float3)(0,0,0);
			for(int s = 0; s<samples; s++){
				float3 d = camera.dir + cx*(float)(((rand(&state)+sx)/2.0+x-w/2)/h) + cy*(float)(((rand(&state)+sy)/2.0+y-h/2)/h);
				struct Ray r;
				r.origin=camera.origin+0.1f*d;
				r.dir=normalize(d);
				subpix_color += 5.0f*raycast(&r, spheres, sphereCount, &state)*invSamp;
			}
			
			sum+=subpix_color/(subpix_color+(float3)1.0)*.25f;
		}
	}
	output[id]=(float4)(sum,0.0);
}


