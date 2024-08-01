#include "Skeleton.h"
#include <iostream>
#include <filesystem>
namespace py = pybind11;
// Initialize the Python interpreter only once
static bool is_python_initialized = false;
//#define PYBIND11_NUMPY_1_ONLY

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	

	return PF_Err_NONE;
}

static PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], PF_LayerDef* output) {
	out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION, BUG_VERSION, STAGE_VERSION, BUILD_VERSION);
	PF_Err err = PF_Err_NONE;
	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;  // Just 16bpc, not 32bpc
	return err;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{ 
    out_data->num_params = 1;
	return PF_Err_NONE;
}

std::string getCurrentDirectory() {
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(NULL, buffer, MAX_PATH)) {
        std::string path(buffer);
        size_t pos = path.find_last_of("\\/");
        return path.substr(0, pos);
    }
    return "";
}

py::dict convertParamsToDict(PF_ParamDef* params[], int numParams) {
    py::dict d;
    for (int i = 1; i < numParams; ++i) {
        if (params[i] == nullptr) {
            continue; // Skip null entries
        }
        switch (params[i]->param_type) {
            case PF_Param_ANGLE:
                d[params[i]->name] = params[i]->u.ad.value;
                break;
            case PF_Param_POPUP:
                d[params[i]->name] = params[i]->u.pd.value;
                break;
            case PF_Param_CHECKBOX:
                d[params[i]->name] = params[i]->u.bd.value;
                break;
            case PF_Param_COLOR:
                d[params[i]->name] = std::vector<uint8_t>{
                    params[i]->u.cd.value.red, 
                    params[i]->u.cd.value.green, 
                    params[i]->u.cd.value.blue
                };
                break;
            case PF_Param_POINT:
                d[params[i]->name] = std::vector<int>{
                    params[i]->u.td.x_value, 
                    params[i]->u.td.y_value
                };
                break;
            case PF_Param_POINT_3D:
                d[params[i]->name] = std::vector<double>{
                    params[i]->u.point3d_d.x_value, 
                    params[i]->u.point3d_d.y_value, 
                    params[i]->u.point3d_d.z_value
                };
                break;
            case PF_Param_FLOAT_SLIDER:
                d[params[i]->name] = params[i]->u.sd.value;
                break;
            default:
                // Handle or log unknown parameter types if needed
                break;
        }
    }
    return d;
}

// Utility function to convert a string to lowercase
std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

static PF_Err Render(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output)
{
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    try {
        py::scoped_interpreter guard{};  // Start the interpreter

        // Add the path to site-packages and capture environment information
        py::exec(R"(
        import sys
        import os
        from io import StringIO
        import sysconfig

        # Dynamically get site-packages path
        site_packages_path = sysconfig.get_paths()['purelib']

        # Add the site-packages directory to sys.path
        if site_packages_path not in sys.path:
            sys.path.insert(0, site_packages_path)

        # Collect environment information
        environment_info = {
            'sys.executable': sys.executable,
            'sys.version': sys.version,
            'sys.path': sys.path,
            'os.environ': dict(os.environ),
        }

        # Redirect stdout and stderr
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        sys.stdout = StringIO()
        sys.stderr = StringIO()

        try:
            import numpy as np
            print("NumPy version:", np.__version__)
        except Exception as e:
            import traceback
            traceback.print_exc()

        # Get the captured output
        stdout_output = sys.stdout.getvalue()
        stderr_output = sys.stderr.getvalue()

        # Restore stdout and stderr
        sys.stdout = old_stdout
        sys.stderr = old_stderr
        )");

        // Retrieve the captured output from the Python execution
        py::object main_module = py::module::import("__main__");
        py::object stdout_output = main_module.attr("stdout_output");
        py::object stderr_output = main_module.attr("stderr_output");
        py::object environment_info = main_module.attr("environment_info");

        // Convert the captured output and environment information to strings
        std::string stdout_str = stdout_output.cast<std::string>();
        std::string stderr_str = stderr_output.cast<std::string>();
        auto env_info_dict = environment_info.cast<py::dict>();

        std::string env_info_str;
        for (auto item : env_info_dict) {
            env_info_str += item.first.cast<std::string>() + ": ";
            if (py::isinstance<py::str>(item.second)) {
                env_info_str += item.second.cast<std::string>() + "\n";
            }
            else if (py::isinstance<py::list>(item.second)) {
                auto list = item.second.cast<py::list>();
                env_info_str += "[";
                for (auto elem : list) {
                    env_info_str += elem.cast<std::string>() + ", ";
                }
                env_info_str += "]\n";
            }
            else if (py::isinstance<py::dict>(item.second)) {
                auto dict = item.second.cast<py::dict>();
                env_info_str += "{";
                for (auto elem : dict) {
                    env_info_str += elem.first.cast<std::string>() + ": " + elem.second.cast<std::string>() + ", ";
                }
                env_info_str += "}\n";
            }
        }

        // Output the information as a single string for debugging
        std::string debug_info = "Captured stdout:\n" + stdout_str + "\nCaptured stderr:\n" + stderr_str + "\nPython Environment Information:\n" + env_info_str;
        std::cout << debug_info << std::endl; // You may want to store this or handle it according to your debugging setup

    }
    catch (const py::error_already_set& e) {
        std::string error_str = "Error in Python execution: " + std::string(e.what());
        std::cout << error_str << std::endl; // Handle this string for debugging
    }
    catch (const std::exception& e) {
        std::string error_str = "Error in Python execution: " + std::string(e.what());
        std::cout << error_str << std::endl; // Handle this string for debugging
    }
    catch (...) {
        std::string error_str = "Error in Python execution: Unknown exception";
        std::cout << error_str << std::endl; // Handle this string for debugging
    }

    return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction2(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB2 inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;
	result = PF_REGISTER_EFFECT_EXT2(
		inPtr,
		inPluginDataCallBackPtr,
		"Skeleton", // Name
		"ADBE Skeleton", // Match Name
		"Sample Plug-ins", // Category
		AE_RESERVED_INFO, // Reserved Info
		"EffectMain",	// Entry point
		"https://www.adobe.com");	// support URL

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;

			
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

