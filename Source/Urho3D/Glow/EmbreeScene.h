//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Container/Ptr.h"
#include "../Glow/LightmapSettings.h"
#include "../Glow/EmbreeForward.h"
#include "../Math/BoundingBox.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Context;
class Node;

/// Embree geometry.
struct EmbreeGeometry
{
    /// Node.
    Node* node_{};
    /// Geometry index.
    unsigned geometryIndex_{};
    /// Geometry LOD.
    unsigned geometryLOD_{};
    /// Lightmap chart index.
    unsigned lightmapIndex_{};
    /// Internal geometry pointer.
    RTCGeometry embreeGeometry_{};
};

/// Calculate bounding box of nodes. Only StaticModel and TerrainPatch are processed.
URHO3D_API BoundingBox CalculateBoundingBoxOfNodes(const ea::vector<Node*>& nodes, bool padIfZero = false);

/// Embree scene.
class URHO3D_API EmbreeScene : public RefCounted
{
public:
    /// Construct.
    EmbreeScene(Context* context, RTCDevice embreeDevice, RTCScene embreeScene,
        ea::vector<EmbreeGeometry> geometries, float maxDistance)
        : context_(context)
        , device_(embreeDevice)
        , scene_(embreeScene)
        , geometries_(ea::move(geometries))
        , maxDistance_(maxDistance)
    {
    }
    /// Destruct.
    ~EmbreeScene() override;

    /// Get context.
    Context* GetContext() const { return context_; }
    /// Get Embree device.
    RTCDevice GetEmbreeDevice() const { return device_; }
    /// Get Embree scene.
    RTCScene GetEmbreeScene() const { return scene_; }
    /// Get Embree geometries.
    const ea::vector<EmbreeGeometry>& GetEmbreeGeometryIndex() const { return geometries_; }
    /// Get max distance between two points.
    float GetMaxDistance() const { return maxDistance_; }

private:
    /// Context.
    Context* context_{};
    /// Embree device.
    RTCDevice device_{};
    /// Embree scene.
    RTCScene scene_{};
    /// Embree geometries.
    ea::vector<EmbreeGeometry> geometries_;
    /// Max distance between two points.
    float maxDistance_{};
};

// Create Embree scene.
URHO3D_API SharedPtr<EmbreeScene> CreateEmbreeScene(Context* context, const ea::vector<Node*>& nodes);

}
