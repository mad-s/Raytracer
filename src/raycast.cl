STRINGIFY(

#pragma OPENCL EXTENSION cl_intel_printf : enable

enum Material {
	DIFF
};

struct Sphere {
	float3 color;
	float3 emission;
	float3 pos;
	float radius;
};

struct Ray {
	float3 origin;
	float3 dir;
};

float rand(uint2* state){
	const float invMaxInt = 1.0f/4294967296.0f;
	uint x = state->x*17+state->y*13123;
	state->x = (x<<13)^x;
	state->y ^= (x<<7);
	x = x*(x*x*15731+74323)+871483;
	return convert_float(x)*invMaxInt;
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

float3 randHemisphere(float3 normal, uint2* state){
	float3 d;
	/*do{*/
		d=(float3)(rand(state)*2-1,rand(state)*2-1,rand(state)*2-1);
	/*}while(dot(d,d)>1);*/
	d=normalize(d);
	if(dot(normal,d)<0)d=-d;
	return d;
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


float3 raycast(struct Ray* r, __constant struct Sphere* spheres, uint sphereCount, uint2* randState){
	unsigned depth = 0;
	float3 radiance=(float3)(0,0,0);
	float3 throughput=(float3)(1,1,1); //how the color has been affected so far
	struct Ray curr = *r;
	__constant struct Sphere* hit;

	/*return throughput;*/
	for(;depth<=6;depth++){
		if(radiance.x>=1.0f&&radiance.y>=1.0f&&radiance.z>=1.0f)return radiance;
		__constant struct Sphere* hit;
		float t = intersect(&curr, spheres, sphereCount, &hit);
		if(t==INFINITY){
			return radiance;
		}
		float3 x = curr.origin+t*curr.dir;
		float3 n = normalize(x-hit->pos);
		radiance+=throughput*hit->emission;
		throughput*=hit->color;
		float3 d = randHemisphere(n,randState);
		curr.origin = x+0.001f*d;
		curr.dir = d;
	}
	return radiance;
}

__kernel void main(__global float4* output, uint w, uint h, uint samples, struct Ray camera, __constant struct Sphere* spheres, uint sphereCount){
	int id = get_global_id(0);
	uint2 state = (uint2)(id*17,id*71235);
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
				float3 d = normalize(camera.dir + cx*(float)(((rand(&state)+sx)/2.0+x-w/2)/h) + cy*(float)(((rand(&state)+sy)/2.0+y-h/2)/h));
				struct Ray r;
				r.origin=camera.origin;
				r.dir=d;
				subpix_color += 5.0f*raycast(&r, spheres, sphereCount, &state)*invSamp;
			}
			
			sum+=subpix_color/(subpix_color+(float3)1.0)*.25f;
		}
	}
	output[id]=(float4)(sum,0.0);
}


);
