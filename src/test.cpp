#include <iostream>
#include <string>
#include <algorithm>
#include <CL/cl.hpp>

bool cmd_option_exists(char** begin, char** end, const std::string &option){
	return std::find(begin, end, option)!=end;
}


int main(int argc, char* argv[])
{
	
	bool no_selection = cmd_option_exists(argv, argv+argc, "--noselect");

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
		std::cerr<<"No OpenCL platforms found. Is OpenCL installed?"<<std::endl;
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

	std::cerr<<"Selected "<<selected_device.getInfo<CL_DEVICE_NAME>()<<std::endl;

}
