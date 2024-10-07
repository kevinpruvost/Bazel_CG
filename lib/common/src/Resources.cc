///
/// Project: VenomEngine
/// @file Resources.cc
/// @date Aug, 22 2024
/// @brief
/// @author Pruvost Kevin | pruvostkevin (pruvostkevin0@gmail.com)
///
#include <venom/common/Resources.h>

#include <venom/common/Log.h>

#include <filesystem>

namespace venom::common
{
// If using bazel
#ifdef __BAZEL__
#include "tools/cpp/runfiles/runfiles.h"

typedef bazel::tools::cpp::runfiles::Runfiles Runfiles;

static std::unique_ptr<Runfiles> s_runfiles;
void Resources::InitializeFilesystem(char** argv)
{
    s_runfiles.reset(bazel::tools::cpp::runfiles::Runfiles::Create(argv[0]));
}

void Resources::FreeFilesystem()
{
    s_runfiles.reset();
}

std::string Resources::GetResourcePath(const std::string& resourcePath)
{
    const std::string path = "VenomEngineWorkspace/resources/" + resourcePath;
    return s_runfiles->Rlocation(path);
}

#else

static std::string s_basePath;
void Resources::InitializeFilesystem(char** argv)
{
    (void)argv;
    s_basePath = "./resources/";
    // Verify if the path exists
    // If not, test at ../
    if (!std::filesystem::exists(s_basePath)) {
        s_basePath = "./../resources/";
        if (!std::filesystem::exists(s_basePath)) {
            Log::Error("Failed to find resources folder");
            exit(1);
        }
    }
}

void Resources::FreeFilesystem()
{
}

static bool validPath(const std::string& path, std::string & res)
{
    std::error_code ec;
    auto realPath = std::filesystem::canonical(path, ec);
    if (realPath.empty() || ec) {
        Log::LogToFile("Failed to find canonical path when looking cache for: %s", path.c_str());
        return false;
    }
    realPath = std::filesystem::relative(realPath, ec);
    if (realPath.empty() || ec) {
        Log::LogToFile("Failed to find relative path when looking cache for: %s", path.c_str());
        return false;
    }
    res = realPath;
    return true;
}

std::string Resources::GetResourcePath(const std::string& resourcePath)
{
    return s_basePath + resourcePath;
}

#endif

std::string Resources::GetTexturesResourcePath(const std::string& resourcePath)
{
    // If resource path already points to a valid pathm then returns it
    return __GetResourcePath(resourcePath, GetResourcePath("textures/"));
}

std::string Resources::GetShadersResourcePath(const std::string& resourcePath)
{
    return __GetResourcePath(resourcePath, GetResourcePath("shaders/"));
}

std::string Resources::GetModelsResourcePath(const std::string& resourcePath)
{
    return __GetResourcePath(resourcePath, GetResourcePath("models/"));
}

std::string Resources::__GetResourcePath(const std::string& resourcePath, const std::string& folder)
{
    std::string res;
    if (!validPath(resourcePath, res) && !validPath(folder + resourcePath, res))
        return "";
    return res;
}
}
