///
/// Project: VenomEngine
/// @file Mesh.cc
/// @date Aug, 25 2024
/// @brief 
/// @author Pruvost Kevin | pruvostkevin (pruvostkevin0@gmail.com)
///
#include <venom/common/plugin/graphics/Model.h>

#include <venom/common/VenomEngine.h>
#include <venom/common/Resources.h>
#include <venom/common/Log.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <iostream>
#include <assimp/DefaultLogger.hpp>
#include <filesystem>

namespace venom
{
namespace common
{

Model::Model()
    : GraphicsPluginObject()
{
}

Model* Model::Create(const std::string & path)
{
    auto realPath = Resources::GetModelsResourcePath(path);
    Model * model = dynamic_cast<Model *>(GetCachedObject(realPath));
    if (!model) {
        model = GraphicsPlugin::Get()->CreateModel();
        if (Error err = model->ImportModel(realPath); err != Error::Success) {
            model->Destroy();
            return nullptr;
        }
        _SetInCache(realPath, model);
    }
    return model;
}

Model::~Model()
{
}

static MaterialComponentType GetMaterialComponentTypeFromAiTextureType(const aiTextureType type)
{
    switch (type)
    {
        case aiTextureType_DIFFUSE: return MaterialComponentType::DIFFUSE;
        case aiTextureType_SPECULAR: return MaterialComponentType::SPECULAR;
        case aiTextureType_AMBIENT: return MaterialComponentType::AMBIENT;
        case aiTextureType_EMISSIVE: return MaterialComponentType::EMISSIVE;
        case aiTextureType_HEIGHT: return MaterialComponentType::HEIGHT;
        case aiTextureType_NORMALS: return MaterialComponentType::NORMAL;
        case aiTextureType_SHININESS: return MaterialComponentType::SHININESS;
        case aiTextureType_OPACITY: return MaterialComponentType::OPACITY;
        case aiTextureType_REFLECTION: return MaterialComponentType::REFLECTION;
        case aiTextureType_BASE_COLOR: return MaterialComponentType::BASE_COLOR;
        case aiTextureType_METALNESS: return MaterialComponentType::METALLIC;
        case aiTextureType_DIFFUSE_ROUGHNESS: return MaterialComponentType::ROUGHNESS;
        case aiTextureType_AMBIENT_OCCLUSION: return MaterialComponentType::AMBIENT_OCCLUSION;
        case aiTextureType_EMISSION_COLOR: return MaterialComponentType::EMISSION_COLOR;
        case aiTextureType_TRANSMISSION: return MaterialComponentType::TRANSMISSION;
        case aiTextureType_SHEEN: return MaterialComponentType::SHEEN;
        case aiTextureType_CLEARCOAT: return MaterialComponentType::CLEARCOAT;
        default: return MaterialComponentType::MAX_COMPONENT;
    }
}

static MaterialComponentType GetMaterialComponentTypeFromProperty(const std::string & name, const int semantic, const int index, const int dataLength, MaterialComponentValueType & type)
{
    // If name starts with "$mat." it's a value, "$clr." is a color, "$tex.file" is a texture
    if (strncmp(name.c_str(), "$mat.", 4) == 0) {
        type = MaterialComponentValueType::VALUE;
    } else if (strncmp(name.c_str(), "$clr.", 4) == 0) {
        type = MaterialComponentValueType::COLOR3D;
        if (dataLength == sizeof(float) * 4) type = MaterialComponentValueType::COLOR4D;
    } else if (strncmp(name.c_str(), "$tex.file", 9) == 0) {
        type = MaterialComponentValueType::TEXTURE;
        return GetMaterialComponentTypeFromAiTextureType(static_cast<aiTextureType>(semantic));
    } else {
        type = MaterialComponentValueType::NONE;
    }

    if (name == "$clr.diffuse") return MaterialComponentType::DIFFUSE;
    if (name == "$clr.ambient") return MaterialComponentType::AMBIENT;
    if (name == "$clr.specular") return MaterialComponentType::SPECULAR;
    if (name == "$clr.emissive") return MaterialComponentType::EMISSIVE;
    if (name == "$mat.shininess") return MaterialComponentType::SHININESS;
    if (name == "$mat.opacity") return MaterialComponentType::OPACITY;
    if (name == "$mat.anisotropyFactor") return MaterialComponentType::ANISOTROPY;
    if (name == "$clr.transparent") return MaterialComponentType::TRANSPARENT;
    if (name == "$clr.reflective") return MaterialComponentType::REFLECTION;
    if (name == "$mat.refracti") return MaterialComponentType::REFRACTION;
    if (name == "$mat.reflectivity") return MaterialComponentType::REFLECTIVITY;


    return MaterialComponentType::MAX_COMPONENT;
}

vc::Error Model::ImportModel(const std::string & path)
{
    // Get Parent folder for relative paths when we will load textures
    auto parentFolder = std::filesystem::path(path).parent_path();

    // Create Logger
    if (Assimp::DefaultLogger::isNullLogger())
        Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    if (!scene) {
        vc::Log::Error("Failed to load model: %s", path.c_str());
        return vc::Error::Failure;
    }

    // Load every material
    if (scene->HasMaterials()) {
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
            auto material = vc::Material::Create();
            __materials.push_back(material);

            const aiMaterial* aimaterial = scene->mMaterials[i];

            // Iterate over all properties of the material
            for (unsigned int p = 0; p < aimaterial->mNumProperties; ++p) {
                aiMaterialProperty* property = aimaterial->mProperties[p];

                // Property Key (name) and Type
                auto propName = property->mKey.C_Str();
                auto propType = property->mType;
                auto propIndex = property->mIndex;
                auto propSemantic = property->mSemantic;

                // If propName is "?mat.name", it's the material name
                if (strncmp(propName, "?mat.name", 9) == 0) {
                    aiString value;
                    memcpy(&value, property->mData, property->mDataLength);
                    material->SetName(value.C_Str());
                    continue;
                }

                MaterialComponentValueType valueType;
                MaterialComponentType matCompType = GetMaterialComponentTypeFromProperty(property->mKey.C_Str(), property->mSemantic, property->mIndex, property->mDataLength, valueType);

                if (matCompType == MaterialComponentType::MAX_COMPONENT) {
                    vc::Log::Error("Unknown material component type: %s", property->mKey.C_Str());
                    continue;
                }

                switch (valueType) {
                    case MaterialComponentValueType::VALUE: {
                        float value;
                        memcpy(&value, property->mData, sizeof(float));
                        material->SetComponent(matCompType, value);
                        break;
                    }
                    case MaterialComponentValueType::COLOR3D: {
                        aiColor3D value;
                        memcpy(&value, property->mData, sizeof(aiColor3D));
                        material->SetComponent(matCompType, vcm::Vec3(value.r, value.g, value.b));
                        break;
                    }
                    case MaterialComponentValueType::COLOR4D: {
                        aiColor4D value;
                        memcpy(&value, property->mData, sizeof(aiColor4D));
                        material->SetComponent(matCompType, vcm::Vec4(value.r, value.g, value.b, value.a));
                        break;
                    }
                    case MaterialComponentValueType::TEXTURE: {
                        aiString value;
                        memcpy(&value, property->mData, property->mDataLength);
                        // Tries to load from cache or path
                        std::string texturePath = parentFolder / value.C_Str();
                        Texture * texture = Texture::Create(texturePath.c_str());
                        material->SetComponent(matCompType, texture);
                        break;
                    }
                    default:
                        break;
                }

#ifdef VENOM_DEBUG
                Log::LogToFile("Property Name: %s", property->mKey.C_Str());
                Log::LogToFile("Property Semantic: %d", property->mSemantic);
                Log::LogToFile("Property Index: %d", property->mIndex);
                Log::LogToFile("Property Data Length: %d", property->mDataLength);
                Log::LogToFile("Property Type: %d", property->mType);

                // Check property type
                switch (property->mType) {
                case aiPTI_Float:
                        Log::LogToFile("Float\n");
                        break;
                case aiPTI_Integer:
                        Log::LogToFile("Integer\n");
                        break;
                case aiPTI_String:
                        Log::LogToFile("String\n");
                        break;
                case aiPTI_Buffer:
                        Log::LogToFile("Buffer\n");
                        break;
                default:
                        Log::LogToFile("Unknown\n");
                }

                // Handle different property types
                if (property->mType == aiPTI_Float && property->mDataLength == sizeof(float)) {
                    float value;
                    memcpy(&value, property->mData, sizeof(float));
                    Log::LogToFile("Float Value: %f\n", value);
                } else if (property->mType == aiPTI_Integer && property->mDataLength == sizeof(int)) {
                    int value;
                    memcpy(&value, property->mData, sizeof(int));
                    Log::LogToFile("Integer Value: %d\n", value);
                } else if (property->mType == aiPTI_String) {
                    aiString value;
                    memcpy(&value, property->mData, property->mDataLength);
                    Log::LogToFile("String Value: %s\n", value.C_Str());
                }

                Log::LogToFile("--------------------------------------------\n");
#endif
            }
        }
    }

    // Load every mesh
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        auto mesh = vc::Mesh::Create();
        __meshes.push_back(mesh);

        const aiMesh* aimesh = scene->mMeshes[i];

        // Assign material
        mesh->SetMaterial(__materials[aimesh->mMaterialIndex]);

        // Vertices & normals
        mesh->__positions.reserve(aimesh->mNumVertices);
        mesh->__normals.reserve(aimesh->mNumVertices);
        for (uint32_t x = 0; x < aimesh->mNumVertices; ++x) {
            mesh->__positions.emplace_back(aimesh->mVertices[x].x, aimesh->mVertices[x].y, aimesh->mVertices[x].z);
            mesh->__normals.emplace_back(aimesh->mNormals[x].x, aimesh->mNormals[x].y, aimesh->mNormals[x].z);
        }

        // Color sets
        for (int c = 0; c < AI_MAX_NUMBER_OF_COLOR_SETS; ++c) {
            if (!aimesh->HasVertexColors(c)) break;

            mesh->__colors[c].reserve(aimesh->mNumVertices);
            for (uint32_t x = 0; x < aimesh->mNumVertices; ++x) {
                mesh->__colors[c].emplace_back(aimesh->mColors[c][x].r, aimesh->mColors[c][x].g, aimesh->mColors[c][x].b, aimesh->mColors[c][x].a);
            }
        }

        // UV Texture Coords
        for (int c = 0; c < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++c) {
            if (!aimesh->HasTextureCoords(c)) break;

            mesh->__uvs[c].reserve(aimesh->mNumVertices);
            for (uint32_t x = 0; x < aimesh->mNumVertices; ++x) {
                mesh->__uvs[c].emplace_back(aimesh->mTextureCoords[c][x].x, aimesh->mTextureCoords[c][x].y);
            }
        }

        // Tangents & Bitangents
        if (aimesh->HasTangentsAndBitangents()) {
            mesh->__tangents.reserve(aimesh->mNumVertices);
            mesh->__bitangents.reserve(aimesh->mNumVertices);
            for (uint32_t x = 0; x < aimesh->mNumVertices; ++x) {
                mesh->__tangents.emplace_back(aimesh->mTangents[x].x, aimesh->mTangents[x].y, aimesh->mTangents[x].z);
                mesh->__bitangents.emplace_back(aimesh->mBitangents[x].x, aimesh->mBitangents[x].y, aimesh->mBitangents[x].z);
            }
        }

        // Faces
        if (aimesh->HasFaces()) {
            mesh->__indices.reserve(aimesh->mNumFaces * 3);
            for (uint32_t x = 0; x < aimesh->mNumFaces; ++x) {
                mesh->__indices.push_back(aimesh->mFaces[x].mIndices[0]);
                mesh->__indices.push_back(aimesh->mFaces[x].mIndices[1]);
                mesh->__indices.push_back(aimesh->mFaces[x].mIndices[2]);
            }
        }

        // Load mesh into Graphics API
        if (auto err = mesh->__LoadMeshFromCurrentData(); err != vc::Error::Success) {
            vc::Log::Error("Failed to load mesh from current data");
            return err;
        }
    }
    return vc::Error::Success;
}

const std::vector<vc::Mesh*>& Model::GetMeshes() const
{
    return __meshes;
}
}
}
