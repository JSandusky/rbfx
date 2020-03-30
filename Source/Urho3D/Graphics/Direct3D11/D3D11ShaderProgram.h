//
// Copyright (c) 2008-2020 the Urho3D project.
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

#pragma once

#include <EASTL/unordered_map.h>

#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/Graphics.h"
#include "../../IO/Log.h"
#include "../../Graphics/ShaderVariation.h"

namespace Urho3D
{

/// Combined information for specific vertex and pixel shaders.
class URHO3D_API ShaderProgram : public RefCounted
{
public:
    /// Construct.
    ShaderProgram(Graphics* graphics, ShaderVariation* vertexShader, ShaderVariation* pixelShader, ShaderVariation* hullShader, ShaderVariation* domainShader, ShaderVariation* geometryShader)
    {
        // Create needed constant buffers
        const unsigned* vsBufferSizes = vertexShader->GetConstantBufferSizes();
        unsigned vertexProcessingSizes[MAX_SHADER_PARAMETER_GROUPS] = {
            vsBufferSizes[0],
            vsBufferSizes[1],
            vsBufferSizes[2],
            vsBufferSizes[3],
            vsBufferSizes[4],
            vsBufferSizes[5],
            vsBufferSizes[6]
        };
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        {
            if (vsBufferSizes[i])
                vsConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(VS, i, vsBufferSizes[i]);
        }

        const unsigned* psBufferSizes = pixelShader->GetConstantBufferSizes();
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        {
            if (psBufferSizes[i])
                psConstantBuffers_[i] = graphics->GetOrCreateConstantBuffer(PS, i, psBufferSizes[i]);
        }

        // Copy parameters, add direct links to constant buffers
        const ea::unordered_map<StringHash, ShaderParameter>& vsParams = vertexShader->GetParameters();
        for (auto i = vsParams.begin(); i != vsParams.end(); ++i)
        {
            parameters_[i->first] = i->second;
            parameters_[i->first].bufferPtr_ = vsConstantBuffers_[i->second.buffer_].Get();
        }

        const ea::unordered_map<StringHash, ShaderParameter>& psParams = pixelShader->GetParameters();
        for (auto i = psParams.begin(); i != psParams.end(); ++i)
        {
            parameters_[i->first] = i->second;
            parameters_[i->first].bufferPtr_ = psConstantBuffers_[i->second.buffer_].Get();
        }

        // The other stages are all expected to use the VS buffers.
        static auto vsFiller = [](Graphics* graphics, ea::unordered_map<StringHash, ShaderParameter>& vsParams, SharedPtr<ConstantBuffer>* vsConstantBuffers, unsigned* vsBufferSizes, ShaderVariation* inQuestion, const char* stageName) {
            if (inQuestion)
            {
                const unsigned* sizes = inQuestion->GetConstantBufferSizes();
                for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
                {
                    if (sizes[i] != vsBufferSizes[i] && sizes[i] != 0)
                    {
                        if (vsBufferSizes[i] == 0)
                        {
                            vsConstantBuffers[i] = graphics->GetOrCreateConstantBuffer(VS, i, sizes[i]);
                            vsBufferSizes[i] = sizes[i];
                        }
                        else
                        {
                            switch (inQuestion->GetShaderType())
                            {
                            case HS:
                                URHO3D_LOGERRORF("Hull shader and vertex shader constant buffer mismatch: GS size {}, VS size {} at index %u", sizes[i], vsBufferSizes[i], i);
                                URHO3D_LOGINFO("Hull and vertex shaders must use matching constant buffers");
                                break;
                            case DS:
                                URHO3D_LOGERRORF("Domain shader and vertex shader constant buffer mismatch: GS size {}, VS size {} at index %u", sizes[i], vsBufferSizes[i], i);
                                URHO3D_LOGINFO("Domain and vertex shaders must use matching constant buffers");
                                break;
                            case GS:
                                URHO3D_LOGERRORF("Geometry shader and vertex shader constant buffer mismatch: GS size {}, VS size {} at index %u", sizes[i], vsBufferSizes[i], i);
                                URHO3D_LOGINFO("Geometry and vertex shaders must use matching constant buffers");
                                break;
                            }
                        }
                    }
                }

                const auto& qParams = inQuestion->GetParameters();
                for (auto i = qParams.begin(); i != qParams.end(); ++i)
                {
                    if (!vsParams.contains(i->first))
                    {
                        vsParams[i->first] = i->second;
                        vsParams[i->first].bufferPtr_ = vsConstantBuffers[i->second.buffer_].Get();
                    }
                }
            }
        };

        vsFiller(graphics, parameters_, vsConstantBuffers_, vertexProcessingSizes, hullShader, "HS");
        vsFiller(graphics, parameters_, vsConstantBuffers_, vertexProcessingSizes, domainShader, "DS");
        vsFiller(graphics, parameters_, vsConstantBuffers_, vertexProcessingSizes, geometryShader, "GS");

        // Optimize shader parameter lookup by rehashing to next power of two
        parameters_.rehash(Max(2u, NextPowerOfTwo(parameters_.size())));

    }

    /// Destruct.
    virtual ~ShaderProgram() override
    {
    }

    /// Combined parameters from the vertex and pixel shader.
    ea::unordered_map<StringHash, ShaderParameter> parameters_;
    /// Vertex shader constant buffers.
    SharedPtr<ConstantBuffer> vsConstantBuffers_[MAX_SHADER_PARAMETER_GROUPS];
    /// Pixel shader constant buffers.
    SharedPtr<ConstantBuffer> psConstantBuffers_[MAX_SHADER_PARAMETER_GROUPS];
};

}
