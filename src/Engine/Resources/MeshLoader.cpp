//
// Created by 2401Lucas on 2025-10-30.
//

#include "MeshLoader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cfloat>
#include <cmath>

MeshData MeshLoader::LoadFromFile(const std::string &path) {
    // Detect format from extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        throw std::runtime_error("Unknown mesh format (no extension): " + path);
    }

    std::string extension = path.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "obj") {
        return LoadOBJ(path);
    } else if (extension == "fbx") {
        return LoadFBX(path);
    } else if (extension == "gltf" || extension == "glb") {
        return LoadGLTF(path);
    } else {
        throw std::runtime_error("Unsupported mesh format: " + extension);
    }
}

MeshData MeshLoader::LoadOBJ(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file: " + path);
    }

    MeshData mesh;
    mesh.name = path;

    // Temporary storage for OBJ data
    std::vector<float> positions;
    std::vector<float> texCoords;
    std::vector<float> normals;

    // Vertex cache to avoid duplicates
    struct VertexKey {
        int posIndex, texIndex, normIndex;

        bool operator==(const VertexKey &other) const {
            return posIndex == other.posIndex &&
                   texIndex == other.texIndex &&
                   normIndex == other.normIndex;
        }
    };

    struct VertexKeyHash {
        size_t operator()(const VertexKey &k) const {
            return std::hash<int>()(k.posIndex) ^
                   (std::hash<int>()(k.texIndex) << 1) ^
                   (std::hash<int>()(k.normIndex) << 2);
        }
    };

    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexCache;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            // Vertex position
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        } else if (type == "vt") {
            // Texture coordinate
            float u, v;
            iss >> u >> v;
            texCoords.push_back(u);
            texCoords.push_back(1.0f - v); // Flip V coordinate
        } else if (type == "vn") {
            // Normal
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(x);
            normals.push_back(y);
            normals.push_back(z);
        } else if (type == "f") {
            // Face (triangle or quad)
            std::vector<uint32_t> faceIndices;
            std::string vertex;

            while (iss >> vertex) {
                VertexKey key = {0, 0, 0};

                // Parse vertex format: v/vt/vn or v//vn or v/vt or v
                size_t pos1 = vertex.find('/');
                if (pos1 != std::string::npos) {
                    key.posIndex = std::stoi(vertex.substr(0, pos1));

                    size_t pos2 = vertex.find('/', pos1 + 1);
                    if (pos2 != std::string::npos) {
                        if (pos2 > pos1 + 1) {
                            key.texIndex = std::stoi(vertex.substr(pos1 + 1, pos2 - pos1 - 1));
                        }
                        key.normIndex = std::stoi(vertex.substr(pos2 + 1));
                    } else if (pos1 + 1 < vertex.length()) {
                        key.texIndex = std::stoi(vertex.substr(pos1 + 1));
                    }
                } else {
                    key.posIndex = std::stoi(vertex);
                }

                // OBJ indices are 1-based, convert to 0-based
                if (key.posIndex > 0) key.posIndex--;
                else key.posIndex = static_cast<int>(positions.size()) / 3 + key.posIndex;

                if (key.texIndex > 0) key.texIndex--;
                else if (key.texIndex < 0) key.texIndex = static_cast<int>(texCoords.size()) / 2 + key.texIndex;

                if (key.normIndex > 0) key.normIndex--;
                else if (key.normIndex < 0) key.normIndex = static_cast<int>(normals.size()) / 3 + key.normIndex;

                // Check vertex cache
                auto it = vertexCache.find(key);
                if (it != vertexCache.end()) {
                    faceIndices.push_back(it->second);
                } else {
                    // Create new vertex
                    Vertex v = {};

                    if (key.posIndex >= 0 && key.posIndex * 3 + 2 < positions.size()) {
                        v.position[0] = positions[key.posIndex * 3 + 0];
                        v.position[1] = positions[key.posIndex * 3 + 1];
                        v.position[2] = positions[key.posIndex * 3 + 2];
                    }

                    if (key.texIndex >= 0 && key.texIndex * 2 + 1 < texCoords.size()) {
                        v.texCoord[0] = texCoords[key.texIndex * 2 + 0];
                        v.texCoord[1] = texCoords[key.texIndex * 2 + 1];
                    }

                    if (key.normIndex >= 0 && key.normIndex * 3 + 2 < normals.size()) {
                        v.normal[0] = normals[key.normIndex * 3 + 0];
                        v.normal[1] = normals[key.normIndex * 3 + 1];
                        v.normal[2] = normals[key.normIndex * 3 + 2];
                    }

                    uint32_t index = static_cast<uint32_t>(mesh.vertices.size());
                    mesh.vertices.push_back(v);
                    vertexCache[key] = index;
                    faceIndices.push_back(index);
                }
            }

            // Triangulate if quad
            if (faceIndices.size() == 3) {
                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[1]);
                mesh.indices.push_back(faceIndices[2]);
            } else if (faceIndices.size() == 4) {
                // Split quad into two triangles
                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[1]);
                mesh.indices.push_back(faceIndices[2]);

                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[2]);
                mesh.indices.push_back(faceIndices[3]);
            }
        }
    }

    // Calculate tangents if we have UVs
    if (!texCoords.empty()) {
        CalculateTangents(mesh);
    }

    // Calculate bounding box
    CalculateBounds(mesh);

    return mesh;
}

MeshData MeshLoader::LoadFBX(const std::string &path) {
    // FBX loading would require FBX SDK or a library like Assimp
    throw std::runtime_error("FBX loading not implemented - use Assimp library");
}

MeshData MeshLoader::LoadGLTF(const std::string &path) {
    // GLTF loading would require tinygltf or similar library
    throw std::runtime_error("GLTF loading not implemented - use tinygltf library");
}

void MeshLoader::CalculateBounds(MeshData &mesh) {
    if (mesh.vertices.empty()) return;

    mesh.boundsMin[0] = mesh.boundsMin[1] = mesh.boundsMin[2] = FLT_MAX;
    mesh.boundsMax[0] = mesh.boundsMax[1] = mesh.boundsMax[2] = -FLT_MAX;

    for (const auto &vertex: mesh.vertices) {
        for (int i = 0; i < 3; i++) {
            mesh.boundsMin[i] = std::min(mesh.boundsMin[i], vertex.position[i]);
            mesh.boundsMax[i] = std::max(mesh.boundsMax[i], vertex.position[i]);
        }
    }
}

void MeshLoader::CalculateTangents(MeshData &mesh) {
    // Calculate tangents for normal mapping
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i + 0];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];

        Vertex &v0 = mesh.vertices[i0];
        Vertex &v1 = mesh.vertices[i1];
        Vertex &v2 = mesh.vertices[i2];

        float edge1[3] = {
            v1.position[0] - v0.position[0],
            v1.position[1] - v0.position[1],
            v1.position[2] - v0.position[2]
        };

        float edge2[3] = {
            v2.position[0] - v0.position[0],
            v2.position[1] - v0.position[1],
            v2.position[2] - v0.position[2]
        };

        float deltaUV1[2] = {
            v1.texCoord[0] - v0.texCoord[0],
            v1.texCoord[1] - v0.texCoord[1]
        };

        float deltaUV2[2] = {
            v2.texCoord[0] - v0.texCoord[0],
            v2.texCoord[1] - v0.texCoord[1]
        };

        float f = 1.0f / (deltaUV1[0] * deltaUV2[1] - deltaUV2[0] * deltaUV1[1]);

        float tangent[3] = {
            f * (deltaUV2[1] * edge1[0] - deltaUV1[1] * edge2[0]),
            f * (deltaUV2[1] * edge1[1] - deltaUV1[1] * edge2[1]),
            f * (deltaUV2[1] * edge1[2] - deltaUV1[1] * edge2[2])
        };

        // Add tangent to all three vertices
        for (int j = 0; j < 3; j++) {
            v0.tangent[j] += tangent[j];
            v1.tangent[j] += tangent[j];
            v2.tangent[j] += tangent[j];
        }
    }

    // Normalize tangents
    for (auto &vertex: mesh.vertices) {
        float len = sqrtf(vertex.tangent[0] * vertex.tangent[0] +
                          vertex.tangent[1] * vertex.tangent[1] +
                          vertex.tangent[2] * vertex.tangent[2]);
        if (len > 0.0f) {
            vertex.tangent[0] /= len;
            vertex.tangent[1] /= len;
            vertex.tangent[2] /= len;
        }
    }
}
