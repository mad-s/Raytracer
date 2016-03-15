#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <CL/cl.hpp>
#include <unistd.h>

#include "raycast.cl.h"

enum Material {
	DIFFUSE,
	REFLECTIVE,
	REFRACTIVE
};

struct Sphere {
	cl_float3 color;
	cl_float3 emisiion;
	cl_float3 pos;
	cl_float radius;
	enum Material material;
};

const float big = 1e5;
Sphere spheres[] = {
	{cl_float3{.75,.25,.25},	cl_float3{},		cl_float3{-big-10,0,0},	big,	DIFFUSE},
	{cl_float3{.75,.75,.75},	cl_float3{},		cl_float3{0,-big-7,0},	big,	DIFFUSE},
	{cl_float3{.75,.75,.75},	cl_float3{},		cl_float3{0,0,big+10},	big,	DIFFUSE},
	{cl_float3{.25,.25,.75},	cl_float3{},		cl_float3{big+10,0,0},	big,	DIFFUSE},
	{cl_float3{.75,.75,.75},	cl_float3{},		cl_float3{0,big+10,0},	big,	DIFFUSE},
	{cl_float3{.75,.75,.75},	cl_float3{},		cl_float3{0,0,-big-30},	big,	DIFFUSE},
	{cl_float3{1,1,1},		cl_float3{},		cl_float3{4,-4,4},	3,	REFLECTIVE},
	{cl_float3{},			cl_float3{12,12,12},	cl_float3{0,19,5},	10,	DIFFUSE}
};

struct Ray {
	cl_float3 origin;
	cl_float3 dir;
};

bool cmd_option_exists(char** begin, char** end, const std::string &option){
	return std::find(begin, end, option)!=end;
}

inline int toInt(float x){
	return (int)(x*255+.5);
}

int main(int argc, char* argv[])
{
	if(isatty(fileno(stdout))){
		std::cerr<<"The program will output in PPM format, please redirect stdout to image.ppm"<<std::endl;
		exit(1);
	}
	bool no_selection = !cmd_option_exists(argv, argv+argc, "--select");

	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);

	if(all_platforms.size()==0){
		std::cerr<<"No OpenCL platforms found. Is OpenCL installed?"<<std::endl;
		exit(1);
	}

	cl::Platform selected_platform;
	if(all_platforms.size()==1 || no_selection){
		selected_platform = all_platforms[0];
	}else{
		std::cerr<<"Select OpenCL platform (default: 1)"<<std::endl;
		for(int i=0;i<all_platforms.size();i++){
			std::cerr<<"["<<i+1<<"] "<<all_platforms[i].getInfo<CL_PLATFORM_NAME>()<<std::endl;
		}

		std::string input;
		std::getline(std::cin,input);
		int val;
		try{
			val = std::stoi(input,nullptr);
		}catch(const std::exception &ex){
			val=0;
		}
		if(val<=0||val>all_platforms.size()){
			selected_platform = all_platforms[0];
		}else{
			selected_platform = all_platforms[val-1];
		}
	}

	std::cerr<<"Selected "<<selected_platform.getInfo<CL_PLATFORM_NAME>()<<std::endl;

	std::vector<cl::Device> all_devices;
	selected_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);

	if(all_devices.size()==0){
		std::cerr<<"No OpenCL devices found. Is OpenCL installed?"<<std::endl;
		exit(1);
	}

	cl::Device selected_device;
	if(all_devices.size()==1 || no_selection){
		selected_device = all_devices[0];
	}else{

		std::cerr<<"Select OpenCL device (default: 1)"<<std::endl;
		for(int i=0;i<all_devices.size();i++){
			std::cerr<<"["<<i+1<<"] "<<all_devices[i].getInfo<CL_DEVICE_NAME>()<<std::endl;
		}

		std::string input;
		std::getline(std::cin,input);
		int val;
		try{
			val = std::stoi(input,nullptr);
		}catch(const std::exception &ex){
			val=0;
		}
		if(val<=0||val>all_devices.size()){
			selected_device = all_devices[0];
		}else{
			selected_device = all_devices[val-1];
		}
	}

	std::cerr<<"Selected "<<selected_device.getInfo<CL_DEVICE_NAME>()<<"("<<selected_device.getInfo<CL_DEVICE_VERSION>()<<")"<<std::endl;


	cl::Context context({selected_device});

	cl::Program::Sources sources;

	
	sources.push_back({(char*)raycast_cl, (unsigned long int)raycast_cl_len});

	cl::Program program(context,sources);
	if(program.build({selected_device})!=CL_SUCCESS){
		std::cerr<<"Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(selected_device)<<std::endl;
		exit(1);
	}

	cl::CommandQueue queue(context,selected_device, CL_QUEUE_PROFILING_ENABLE);

	int width=1366;
	int height=768;
	int samples=25;
	Ray camera = {cl_float3{0.5,1,-16},cl_float3{0.0014,0,1}};
	cl::Buffer screen_buffer(context,CL_MEM_WRITE_ONLY,width*height*sizeof(cl_float4));
	cl::Buffer sphere_buffer(context,CL_MEM_READ_WRITE,sizeof(spheres));
	queue.enqueueWriteBuffer(sphere_buffer,CL_FALSE,0,sizeof(spheres),spheres);
	cl::Kernel kernel(program,"main");
	kernel.setArg(0,screen_buffer);
	kernel.setArg(1,(cl_uint)(width));
	kernel.setArg(2,(cl_uint)(height));
	kernel.setArg(3,(cl_uint)(samples));
	kernel.setArg(4,camera);
	kernel.setArg(5,sphere_buffer);
	kernel.setArg(6,(cl_uint)(sizeof(spheres)/sizeof(Sphere)));
	std::cerr<<"Starting kernel\n";
	
	cl::Event evt;

	queue.enqueueNDRangeKernel(kernel,cl::NullRange, cl::NDRange(width*height), cl::NullRange, NULL, &evt);
	queue.flush();
	evt.wait();
	long long delta = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>()-evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
	std::cerr<<"Took "<<delta/1000000<<"ms to render\n";
	std::cerr<<"That is "<<delta/(4ll*width*height*samples)<<"ns per sample\n";
	
	cl_float4* buff = (cl_float4*)queue.enqueueMapBuffer(screen_buffer,CL_TRUE,CL_MAP_READ,0,width*height*sizeof(cl_float4), NULL, &evt);
	evt.wait();
	
	delta = evt.getProfilingInfo<CL_PROFILING_COMMAND_END>()-evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
	std::cerr<<"Took "<<delta/1000<<"µs to transfer "<< width*height*sizeof(cl_float4)/sizeof(char) <<" bytes\n";
	std::cerr<<"That is "<<(1000000000ll*width*height*sizeof(cl_float4)/sizeof(char)>>20)<<"MiB/s\n";
	std::cout<<"P3\n"<<width<<" "<<height<<"\n255\n";
	for(int i=0;i<width*height;i++){
		std::cout<<toInt(buff[i].s[0])<<" ";
		std::cout<<toInt(buff[i].s[1])<<" ";
		std::cout<<toInt(buff[i].s[2])<<"\n";
	}


}
