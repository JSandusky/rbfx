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

#include "../Precompiled.h"

#include <EASTL/sort.h>

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Shader.h"
#include "../Graphics/ShaderVariation.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"

#include "../DebugNew.h"

namespace Urho3D
{

void CommentOutFunction(ea::string& code, const ea::string& signature)
{
    unsigned startPos = code.find(signature);
    unsigned braceLevel = 0;
    if (startPos == ea::string::npos)
        return;

    code.insert(startPos, "/*");

    for (unsigned i = startPos + 2 + signature.length(); i < code.length(); ++i)
    {
        if (code[i] == '{')
            ++braceLevel;
        else if (code[i] == '}')
        {
            --braceLevel;
            if (braceLevel == 0)
            {
                code.insert(i + 1, "*/");
                return;
            }
        }
    }
}

Shader::Shader(Context* context) :
    Resource(context),
    timeStamp_(0),
    numVariations_(0)
{
    RefreshMemoryUse();
}

Shader::~Shader()
{
    if (!context_.Expired())
    {
        auto* cache = GetSubsystem<ResourceCache>();
        if (cache)
            cache->ResetDependencies(this);
    }
}

void Shader::RegisterObject(Context* context)
{
    context->RegisterFactory<Shader>();
}

bool Shader::BeginLoad(Deserializer& source)
{
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return false;

    // Load the shader source code and resolve any includes
    timeStamp_ = 0;
    ea::string shaderCode;
    if (!ProcessSource(shaderCode, source))
        return false;

    // Comment out the unneeded shader function
#define SETUP_SOURCE(WHO, A, B, C, D, E) WHO ## Data_.sourceCode_ = shaderCode; \
    CommentOutFunction( WHO ## Data_.sourceCode_, "void " #A "("); \
    CommentOutFunction( WHO ## Data_.sourceCode_, "void " #B "("); \
    CommentOutFunction( WHO ## Data_.sourceCode_, "void " #C "("); \
    CommentOutFunction( WHO ## Data_.sourceCode_, "void " #D "("); \
    CommentOutFunction( WHO ## Data_.sourceCode_, "void " #E "(");

    SETUP_SOURCE(vs, PS, HS, DS, GS, CS);
    SETUP_SOURCE(ps, VS, HS, DS, GS, CS);
    SETUP_SOURCE(hs, PS, VS, DS, GS, CS);
    SETUP_SOURCE(ds, PS, HS, VS, GS, CS);
    SETUP_SOURCE(gs, PS, HS, DS, VS, CS);
    SETUP_SOURCE(cs, PS, HS, DS, GS, VS);

    // OpenGL: rename either VS() or PS() to main()
#ifdef URHO3D_OPENGL
#define HANDLE_SHADER_STAGE(SHORTNAME, LCASESHORTNAME) \
    LCASESHORTNAME ## Data_.sourceCode_.replace("void " #SHORTNAME "(", "void main(");

    HANDLE_SHADER_STAGE(VS, vs);
    HANDLE_SHADER_STAGE(PS, ps);

    HANDLE_SHADER_STAGE(HS, hs);
    HANDLE_SHADER_STAGE(DS, ds);
    HANDLE_SHADER_STAGE(GS, gs);
    HANDLE_SHADER_STAGE(CS, cs);

#undef HANDLE_SHADER_STAGE
#endif

    RefreshMemoryUse();
    return true;
}

bool Shader::EndLoad()
{
#define FREE_VARIATIONS(STAGE) \
    for (auto i = STAGE ## Data_.variations_.begin(); i != STAGE ## Data_.variations_.end(); ++i) i->second->Release();

    // If variations had already been created, release them and require recompile
    FREE_VARIATIONS(vs);
    FREE_VARIATIONS(hs);
    FREE_VARIATIONS(ds);
    FREE_VARIATIONS(gs);
    FREE_VARIATIONS(ps);
    FREE_VARIATIONS(cs);

#undef FREE_VARIATIONS

    return true;
}

ShaderVariation* Shader::GetVariation(ShaderType type, const ea::string& defines)
{
    return GetVariation(type, defines.c_str());
}

ShaderVariation* Shader::GetVariation(ShaderType type, const char* defines)
{
    const unsigned definesHash = GetShaderDefinesHash(defines);

    ea::unordered_map<unsigned, SharedPtr<ShaderVariation> >* variations = nullptr;
    switch (type)
    {
    case VS:
        variations = &vsData_.variations_;
        break;
    case PS:
        variations = &psData_.variations_;
        break;
    case CS:
        variations = &csData_.variations_;
        break;
    case HS:
        variations = &hsData_.variations_;
        break;
    case DS:
        variations = &dsData_.variations_;
        break;
    case GS:
        variations = &gsData_.variations_;
        break;
    }

    if (variations == nullptr)
        return nullptr;
    
    auto i = variations->find(definesHash);
    if (i == variations->end())
    {
        // If shader not found, normalize the defines (to prevent duplicates) and check again. In that case make an alias
        // so that further queries are faster
        const ea::string normalizedDefines = NormalizeDefines(defines);
        const unsigned normalizedHash = GetShaderDefinesHash(normalizedDefines.c_str());

        i = variations->find(normalizedHash);
        if (i != variations->end())
            variations->insert(ea::make_pair(definesHash, i->second));
        else
        {
            // No shader variation found. Create new
            i = variations->insert(ea::make_pair(normalizedHash, SharedPtr<ShaderVariation>(new ShaderVariation(this, type)))).first;
            if (definesHash != normalizedHash)
                variations->insert(ea::make_pair(definesHash, i->second));

            Graphics* graphics = context_->GetGraphics();
            i->second->SetName(GetFileName(GetName()));
            i->second->SetDefines(graphics->GetGlobalShaderDefines() + " " + normalizedDefines);
            ++numVariations_;
            RefreshMemoryUse();
        }
    }

    return i->second;
}

const ea::string& Shader::GetSourceCode(ShaderType type) const
{
    switch (type)
    {
    case VS:
        return vsData_.sourceCode_;
    case HS:
        return hsData_.sourceCode_;
    case DS:
        return dsData_.sourceCode_;
    case GS:
        return gsData_.sourceCode_;
    case PS:
        return psData_.sourceCode_;
    case CS:
        return csData_.sourceCode_;
    }
    return vsData_.sourceCode_;
}

unsigned Shader::GetShaderDefinesHash(const char* defines) const
{
    Graphics* graphics = context_->GetGraphics();
    unsigned definesHash = StringHash(defines).Value();
    CombineHash(definesHash, graphics->GetGlobalShaderDefinesHash().Value());
    return definesHash;
}

bool Shader::ProcessSource(ea::string& code, Deserializer& source)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // If the source if a non-packaged file, store the timestamp
    auto* file = dynamic_cast<File*>(&source);
    if (file && !file->IsPackaged())
    {
        auto* fileSystem = GetSubsystem<FileSystem>();
        ea::string fullName = cache->GetResourceFileName(file->GetName());
        unsigned fileTimeStamp = fileSystem->GetLastModifiedTime(fullName);
        if (fileTimeStamp > timeStamp_)
            timeStamp_ = fileTimeStamp;
    }

    // Store resource dependencies for includes so that we know to reload if any of them changes
    if (source.GetName() != GetName())
        cache->StoreResourceDependency(this, source.GetName());

    while (!source.IsEof())
    {
        ea::string line = source.ReadLine();

        if (line.starts_with("#include"))
        {
            ea::string includeFileName = GetPath(source.GetName()) + line.substr(9).replaced("\"", "").trimmed();

            SharedPtr<File> includeFile = cache->GetFile(includeFileName);
            if (!includeFile)
                return false;

            // Add the include file into the current code recursively
            if (!ProcessSource(code, *includeFile))
                return false;
        }
        else
        {
            code += line;
            code += "\n";
        }
    }

    // Finally insert an empty line to mark the space between files
    code += "\n";

    return true;
}

ea::string Shader::NormalizeDefines(const ea::string& defines)
{
    ea::vector<ea::string> definesVec = defines.to_upper().split(' ');
    ea::quick_sort(definesVec.begin(), definesVec.end());
    return ea::string::joined(definesVec, " ");
}

void Shader::RefreshMemoryUse()
{
    SetMemoryUse(
        (unsigned)(sizeof(Shader) +
            vsData_.sourceCode_.length() +
            psData_.sourceCode_.length() +
            csData_.sourceCode_.length() +
            hsData_.sourceCode_.length() +
            dsData_.sourceCode_.length() +
            gsData_.sourceCode_.length() +
            numVariations_ * sizeof(ShaderVariation)));
}

}
