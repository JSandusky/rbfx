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
        static auto vsFiller = [](Graphics* graphics, ea::unordered_map<StringHash, ShaderParameter>& vsParams, SharedPtr<ConstantBuffer>* vsConstantBuffers, ShaderVariation* inQuestion, const char* stageName) {
            if (inQuestion)
            {
                const unsigned* sizes = inQuestion->GetConstantBufferSizes();
                for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
                {
                    if (vsConstantBuffers[i] && vsConstantBuffers[i]->GetSize() != sizes[i])
                    {
                        URHO3D_LOGERROR("Constant buffer size mismatch between VS and {}, VS has {} while {} has {}", stageName, vsConstantBuffers[i]->GetSize(), stageName, sizes[i]);
                    }
                    else if (vsConstantBuffers[i].Null())
                        vsConstantBuffers[i] = graphics->GetOrCreateConstantBuffer(VS, i, sizes[i]);
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

        vsFiller(graphics, parameters_, vsConstantBuffers_, hullShader, "HS");
        vsFiller(graphics, parameters_, vsConstantBuffers_, domainShader, "DS");
        vsFiller(graphics, parameters_, vsConstantBuffers_, geometryShader, "GS");

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
